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
#include "version.hpp"
#include "expandurl-mastodon.hpp"

using std::cerr;
using std::cout;
using std::string;

Listener::Listener()
: _instance("")
, _access_token("")
, _stream("")
, _ptr(nullptr)
, _running(false)
, _configfilepath(static_cast<const string>(getenv("HOME")) +
                  "/.config/expandurl-mastodon.json")
{

    if (read_config())
    {
        _masto = std::make_unique<Easy>(_instance, _access_token);
        _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                              global::version);
    }
    else
    {
        cerr << "WARNING: Could not open " << _configfilepath << ".\n";
        cout << "Attempting to register application and write config file.\n";
        if (register_app())
        {
            cout << "DEBUG: registration successful.\n";
            if (!write_config())
            {
                cerr << "ERROR: Could not write " << _configfilepath << ".\n";
                std::exit(1);
            }
        }
        else
        {
            cerr << "ERROR: Could not register app.\n";
            std::exit(2);
        }
    }
}

Listener::~Listener()
{
    if (!write_config())
    {
        cerr << "ERROR: Could not write " << _configfilepath << ".\n";
    }
}

const bool Listener::read_config()
{
    std::ifstream file(_configfilepath);

    if (file.is_open())
    {
        std::stringstream json;
        json << file.rdbuf();
        file.close();
        json >> _config;

        _instance = _config["account"].asString();
        _instance = _instance.substr(_instance.find('@') + 1);
        _access_token = _config["access_token"].asString();
        return true;
    }

    return false;
}

const bool Listener::write_config()
{
    std::ofstream outfile(_configfilepath);
    if (outfile.is_open())
    {
        outfile.write(_config.toStyledString().c_str(),
                      _config.toStyledString().length());
        outfile.close();

        return true;
    }

    return false;
}

const void Listener::start()
{
    _thread = std::thread([=]
    {
        _running = true;
        Easy masto(_instance, _access_token);
        masto.set_useragent(static_cast<const string>("expandurl-mastodon/") +
                            global::version);
        masto.get_stream(Mastodon::API::v1::streaming_user, _stream, _ptr);
        cout << "DEBUG: Connection lost.\n";
        _running = false;
    });
    while (_ptr == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

const void Listener::stop()
{
    if (_ptr)
    {
        _ptr->cancel_stream();
        _thread.join();
        _ptr.reset();
        _stream = "";
    }
    else
    {
        cout << "DEBUG: _ptr is false.\n";
    }
}

const std::vector<Easy::Notification> Listener::get_new_messages()
{
    std::vector<Easy::Notification> v;
    static std::uint_fast8_t count_empty = 0;

    std::lock_guard<std::mutex> lock(_ptr->get_mutex());
    if (!_stream.empty())
    {
        const string buffer = _stream;
        _stream.clear();

        for (const Easy::stream_event &event : Easy::parse_stream(buffer))
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
        count_empty = 0;
    }
    else
    {
        // If we got an empty message 5 times, set state to not running
        ++count_empty;
        if (count_empty > 5)
        {
            count_empty = 0;
            cout << "DEBUG: Detected broken connection.\n";
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
        cout << "DEBUG: catching up...\n";
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
        cerr << "ERROR: " << ret << " (in " << __FUNCTION__ << ")\n";
        return Easy::Status();
    }
}

const bool Listener::send_reply(const Easy::Status &to_status,
                                const string &message)
{
    std::uint_fast16_t ret;
    string answer;
    const string id = std::to_string(to_status.id());
    string strvisibility;

    switch (to_status.visibility())
    {
        case Easy::visibility_type::Private:
            strvisibility = "private";
            break;
        case Easy::visibility_type::Direct:
            strvisibility = "direct";
            break;
        default:
            strvisibility = "unlisted";
            break;
    }

    Easy::parametermap parameters =
    {
        { "in_reply_to_id", { id } },
        { "visibility", { strvisibility } },
        { "status", { '@' + to_status.account().acct() + ' ' + message } }
    };

    if (to_status.sensitive())
    {
        parameters.insert({ "sensitive", { "true" } });
    }

    if (!to_status.spoiler_text().empty())
    {
        parameters.insert({ "spoiler_text", { to_status.spoiler_text() } });
    }

    ret = _masto->post(API::v1::statuses, parameters, answer);

    if (ret == 0)
    {
        cout << "DEBUG: Sent reply.\n";
        return true;
    }
    else
    {
        cerr << "ERROR: " << ret << " (in " << __FUNCTION__ << ")\n";
        return false;
    }
}

const std::uint_fast64_t Listener::get_parent_id(const Easy::Notification &notif)
{
    string answer;
    std::uint_fast16_t ret;

    // Fetch full status
    ret = _masto->get(API::v1::search, {{ "q", { notif.status().url() }}},
                      answer);
    if (ret > 0)
    {
        cerr << "ERROR: " << ret <<
                "Could not fetch status (in " << __FUNCTION__ << ")\n";
        return 0;
    }

    ret = _masto->get(API::v1::statuses_id,
                      {{ "id", { std::to_string(notif.status().id()) }}},
                      answer);

    if (ret > 0)
    {
        cerr << "ERROR: " << ret <<
                "Could not get status (in " << __FUNCTION__ << ")\n";
        return 0;
    }
    else
    {
        _config["last_id"] = std::to_string(notif.id());
        Easy::Status s(answer);
        return s.in_reply_to_id();
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
                                "https://github.com/tastytea/expandurl-mastodon",
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
            cerr << "ERROR: register_app2(): " << ret << '\n';
        }
    }
    else
    {
        cerr << "ERROR: register_app1(): " << ret << '\n';
    }

    return false;
}
