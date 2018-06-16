/*  This file is part of expandurl-mastodon.
 *  Copyright © 2018 tastytea <tastytea@tastytea.de>
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

        _masto = std::make_unique<Easy>(_instance, _access_token);
        _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                              global::version);
        set_proxy(*_masto);
}

Listener::~Listener()
{
}

const void Listener::read_config()
{
    _instance = _config["account"].asString();
    _instance = _instance.substr(_instance.find('@') + 1);
    _access_token = _config["access_token"].asString();
    _proxy = _config["proxy"]["url"].asString();
    _proxy_user = _config["proxy"]["user"].asString();
    _proxy_password = _config["proxy"]["password"].asString();
}

const void Listener::start()
{
    constexpr uint_fast8_t delay_after_error = 120;
    static std::uint_fast16_t ret;
    _thread = std::thread([=]
    {
        ret = 0;
        _running = true;
        Easy masto(_instance, _access_token);
        masto.set_useragent(static_cast<const string>("expandurl-mastodon/") +
                            global::version);
        set_proxy(masto);
        ret = masto.get_stream(Mastodon::API::v1::streaming_user, _stream, _ptr);
        syslog(LOG_DEBUG, "Connection lost.");
        if (ret != 0 && ret != 14)  // 14 means canceled by user
        {
            syslog(LOG_ERR, "Connection terminated: Error %u", ret);
            syslog(LOG_INFO, "Waiting for %u seconds", delay_after_error);
            std::this_thread::sleep_for(std::chrono::seconds(delay_after_error));
        }
        _running = false;
    });
    while (!_ptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (ret == 0)
    {
        syslog(LOG_NOTICE, "Connected to %s", _instance.c_str());
    }
    else if (ret != 14)
    {   // If the stream thread sleeps, the main thread should sleep too
        std::this_thread::sleep_for(std::chrono::seconds(delay_after_error));
    }
}

const void Listener::stop()
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
            if (event.first == Easy::event_type::Notification)
            {
                Easy::Notification notif(event.second);
                if (notif.type() == Easy::notification_type::Mention)
                {
                    v.push_back(notif);
                }
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
        API::parametermap parameter =
        {
            { "since_id", { last_id } },
            { "exclude_types", { "follow", "favourite", "reblog" } }
        };
        string answer;
        std::uint_fast16_t ret;

        ret = _masto->get(API::v1::notifications, parameter, answer);

        if (ret == 0)
        {
            for (const string str : Easy::json_array_to_vector(answer))
            {
                v.push_back(Easy::Notification(str));
            }
        }
        else
        {
            syslog(LOG_ERR, "Could not catch up: Error %u", ret);
        }
    }

    return v;
}

Mastodon::Easy::Status Listener::get_status(const std::uint_fast64_t &id)
{
    std::uint_fast16_t ret;
    string answer;

    ret = _masto->get(API::v1::statuses_id, {{ "id", { std::to_string(id) }}},
                      answer);
    if (ret == 0)
    {
        return Easy::Status(answer);
    }
    else
    {
        syslog(LOG_ERR, "Error %u in %s.", ret, __FUNCTION__);
        return Easy::Status();
    }
}

const bool Listener::send_reply(const Easy::Status &to_status,
                                const string &message)
{
    std::uint_fast16_t ret = 0;

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

    _masto->send_toot(new_status, ret);

    if (ret == 0)
    {
        syslog(LOG_DEBUG, "Sent reply");
        return true;
    }
    else
    {
        syslog(LOG_ERR, "Error %u in %s.", ret, __FUNCTION__);
        return false;
    }
}

const std::uint_fast64_t Listener::get_parent_id(const Easy::Notification &notif)
{
    string answer;
    std::uint_fast16_t ret;

    // Retry up to 2 times
    for (std::uint_fast8_t retries = 1; retries <= 2; ++retries)
    {
        // Fetch full status
        ret = _masto->get(API::v1::search, {{ "q", { notif.status().url() }}},
                          answer);
        if (ret > 0 || !Easy::Status(answer).valid())
        {
            syslog(LOG_ERR, "Error %u: Could not fetch status (in %s).",
                   ret, __FUNCTION__);
            return 0;
        }

        ret = _masto->get(API::v1::statuses_id,
                          {{ "id", { std::to_string(notif.status().id()) }}},
                          answer);

        if (ret > 0)
        {
            syslog(LOG_ERR, "Error %u: Could not get status (in %s).",
                   ret, __FUNCTION__);
            return 0;
        }
        else
        {
            _config["last_id"] = std::to_string(notif.id());
            const Easy::Status s(answer);

            // If parent is found, return ID; else retry
            if (s.in_reply_to_id() != 0)
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

const bool Listener::stillrunning() const
{
    return _running;
}

const bool Listener::register_app()
{
    cout << "Account (username@instance): ";
    std::cin >> _instance;
    _config["account"] = _instance;
    _instance = _instance.substr(_instance.find('@') + 1);

    _masto = std::make_unique<Easy>(_instance, "");
    _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                          global::version);

    std::uint_fast16_t ret;
    string client_id, client_secret, url;
    ret = _masto->register_app1("expandurl-mastodon",
                                "urn:ietf:wg:oauth:2.0:oob",
                                "read write",
                                "https://schlomp.space/tastytea/expandurl-mastodon",
                                client_id,
                                client_secret,
                                url);
    if (ret == 0)
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
        if (ret == 0)
        {
            _config["access_token"] = _access_token;
            return true;
        }
        else
        {
            syslog(LOG_ERR, "register_app2(): %u", ret);
        }
    }
    else
    {
        syslog(LOG_ERR, "register_app1(): %u", ret);
    }

    return false;
}

const void Listener::set_proxy(Mastodon::Easy &masto)
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
