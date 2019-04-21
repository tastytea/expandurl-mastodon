/*  This file is part of expandurl-mastodon.
 *  Copyright © 2018, 2019 tastytea <tastytea@tastytea.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <cstdlib>  // getenv()
#include <iostream>
#include <mutex>
#include <sstream>
#include <syslog.h>
#include <chrono>
#include "version.hpp"
#include "expandurl-mastodon.hpp"

using std::cout;
using std::string;
using std::uint8_t;

Listener::Listener()
: _instance("")
, _access_token("")
, _stream("")
, _ptr(nullptr)
, _running(false)
, _proxy("")
, _proxy_user("")
, _proxy_password("")
, _config(configfile.get_json())
{
    read_config();
    if (_config["access_token"].isNull())
    {
        syslog(LOG_INFO, "Attempting to register application and write config file.");
        if (register_app())
        {
            syslog(LOG_INFO, "Registration successful.");
            if (!configfile.write())
            {
                syslog(LOG_ERR, "Could not write %s.",
                       configfile.get_filepath().c_str());
                std::exit(1);
            }
        }
        else
        {
            syslog(LOG_ERR, "Could not register app.");
            std::exit(2);
        }
    }

    _masto = std::make_unique<Easy::API>(_instance, _access_token);
        _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                              global::version);
        set_proxy(*_masto);
}

Listener::~Listener()
{
}

void Listener::read_config()
{
    _instance = _config["account"].asString();
    _instance = _instance.substr(_instance.find('@') + 1);
    _access_token = _config["access_token"].asString();
    _proxy = _config["proxy"]["url"].asString();
    _proxy_user = _config["proxy"]["user"].asString();
    _proxy_password = _config["proxy"]["password"].asString();
}

void Listener::start()
{
    _running = true;
    _masto->get_stream(API::v1::streaming_user, _ptr, _stream);

    syslog(LOG_NOTICE, "Connecting to %s ...", _instance.c_str());
}

void Listener::stop()
{
    if (!configfile.write())
    {
        syslog(LOG_ERR, "Could not write %s.",
               configfile.get_filepath().c_str());
    }

    if (_ptr)
    {
        _ptr->cancel_stream();
        _thread.join();
        _ptr.reset();
        _ptr = nullptr;
        _stream = "";
    }
    else
    {
        syslog(LOG_DEBUG, "_ptr is false.");
    }
}

const std::vector<Easy::Notification> Listener::get_new_messages()
{
    using namespace std::chrono;

    std::vector<Easy::Notification> v;
    static system_clock::time_point lastping = system_clock::now();

    std::lock_guard<std::mutex> lock(_ptr->get_mutex());
    if (!_stream.empty())
    {
        for (const Easy::stream_event &event : Easy::parse_stream(_stream))
        {
            if (event.type == Easy::event_type::Notification)
            {
                Easy::Notification notif(event.data);
                if (notif.type() == Easy::notification_type::Mention)
                {
                    v.push_back(notif);
                }
            }
            else if (event.type == Easy::event_type::Error)
            {
                constexpr uint8_t delay_after_error = 120;
                syslog(LOG_DEBUG, "Connection lost.");
                const Json::Value err;
                syslog(LOG_ERR, "Connection terminated: Error %u",
                       err["error_code"].asUInt());
                syslog(LOG_INFO, "Waiting for %u seconds", delay_after_error);
                std::this_thread::sleep_for(std::chrono::seconds(delay_after_error));
                _running = false;
            }
        }
        _stream.clear();
        lastping = system_clock::now();
    }
    else
    {
        // If the last keep-alive packet was received 25 seconds or more ago
        if (duration_cast<seconds>(system_clock::now() - lastping).count() >= 25)
        {
            lastping = system_clock::now();
            syslog(LOG_NOTICE, "Detected broken connection.");
            _running = false;
        }
    }

    return v;
}

const std::vector<Easy::Notification> Listener::catchup()
{
    std::vector<Easy::Notification> v;
    const string last_id = _config["last_id"].asString();
    if (last_id != "")
    {
        syslog(LOG_DEBUG, "Catching up...");
        parameters parameter =
        {
            { "since_id", { last_id } },
            { "exclude_types", { "follow", "favourite", "reblog" } }
        };
        return_call ret;;

        ret = _masto->get(API::v1::notifications, parameter);

        if (ret)
        {
            for (const string str : Easy::json_array_to_vector(ret.answer))
            {
                v.push_back(Easy::Notification(str));
            }
        }
        else
        {
            syslog(LOG_ERR, "Could not catch up: Error %u", ret.error_code);
        }
    }

    return v;
}

Mastodon::Easy::Status Listener::get_status(const string &id)
{
    return_call ret;

    ret = _masto->get(API::v1::statuses_id, {{ "id", { id }}});
    if (ret)
    {
        return Easy::Status(ret.answer);
    }
    else
    {
        syslog(LOG_ERR, "Error %u in %s.", ret.error_code, __FUNCTION__);
        return Easy::Status();
    }
}

bool Listener::send_reply(const Easy::Status &to_status,
                          const string &message)
{
    Easy::return_entity<Easy::Status> ret;

    Easy::Status new_status;
    if (to_status.visibility() == Easy::visibility_type::Public)
    {
        new_status.visibility(Easy::visibility_type::Unlisted);
    }
    else
    {
        new_status.visibility(to_status.visibility());
    }
    new_status.in_reply_to_id(to_status.id());
    new_status.content('@' + to_status.account().acct() + ' ' + message);
    new_status.sensitive(to_status.sensitive());
    new_status.spoiler_text(to_status.spoiler_text());

    ret = _masto->send_post(new_status);

    if (ret)
    {
        syslog(LOG_DEBUG, "Sent reply");
        return true;
    }
    else
    {
        syslog(LOG_ERR, "Error %u in %s.", ret.error_code, __FUNCTION__);
        return false;
    }
}

const string Listener::get_parent_id(const Easy::Notification &notif)
{
    return_call ret;

    // Retry up to 2 times
    for (std::uint_fast8_t retries = 1; retries <= 2; ++retries)
    {
        // Fetch full status
        ret = _masto->get(API::v1::search, {{ "q", { notif.status().url() }}});
        if (!ret)
        {
            syslog(LOG_ERR, "Error %u: Could not fetch status (in %s).",
                   ret.error_code, __FUNCTION__);
            return 0;
        }

        ret = _masto->get(API::v1::statuses_id,
                          {{ "id", { notif.status().id() }}});

        if (!ret)
        {
            syslog(LOG_ERR, "Error %u: Could not get status (in %s).",
                   ret.error_code, __FUNCTION__);
            return 0;
        }
        else
        {
            _config["last_id"] = notif.id();
            const Easy::Status s(ret.answer);

            // If parent is found, return ID; else retry
            if (!s.in_reply_to_id().empty())
            {
                return s.in_reply_to_id();
            }
            else
            {
                syslog(LOG_WARNING, "Could not get ID of replied-to post");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }

    return 0;
}

bool Listener::stillrunning() const
{
    return _running;
}

bool Listener::register_app()
{
    cout << "Account (username@instance): ";
    std::cin >> _instance;
    _config["account"] = _instance;
    _instance = _instance.substr(_instance.find('@') + 1);

    _masto = std::make_unique<Easy::API>(_instance, "");
    _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                          global::version);

    return_call ret;
    string client_id, client_secret, url;
    ret = _masto->register_app1("expandurl-mastodon",
                                "urn:ietf:wg:oauth:2.0:oob",
                                "read write",
                                "https://schlomp.space/tastytea/expandurl-mastodon",
                                client_id,
                                client_secret,
                                url);
    if (ret)
    {
        string code;
        cout << "Visit " << url << " to authorize this application.\n";
        cout << "Paste the authorization code here: ";
        std::cin >> code;
        ret = _masto->register_app2(client_id,
                                    client_secret,
                                    "urn:ietf:wg:oauth:2.0:oob",
                                    code,
                                    _access_token);
        if (ret)
        {
            _config["access_token"] = _access_token;
            return true;
        }
        else
        {
            syslog(LOG_ERR, "register_app2(): %u", ret.error_code);
        }
    }
    else
    {
        syslog(LOG_ERR, "register_app1(): %u", ret.error_code);
    }

    return false;
}

void Listener::set_proxy(Easy::API &masto)
{
    if (!_proxy.empty())
    {
        if (!_proxy_user.empty())
        {
            masto.set_proxy(_proxy, _proxy_user + ':' + _proxy_password);
        }
        else
        {
            masto.set_proxy(_proxy);
        }
    }
}
