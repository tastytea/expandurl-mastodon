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
{
    const string filepath = static_cast<const string>(getenv("HOME")) +
                            "/.config/expandurl-mastodon.cfg";
    std::ifstream file(filepath);

    if (file.is_open())
    {
        std::getline(file, _instance);
        _instance = _instance.substr(_instance.find('@') + 1);
        std::getline(file, _access_token);
        file.close();

        _masto = std::make_unique<Easy>(_instance, _access_token);
        _masto->set_useragent(static_cast<const string>("expandurl-mastodon/") +
                              global::version);
    }
    else
    {
        cerr << "ERROR: Could not open " << filepath << '\n';
        exit(1);
    }
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
}

const void Listener::stop()
{
    if (_ptr)
    {
        _ptr->abort_stream();
        _thread.join();
    }
}

std::vector<Easy::Notification> Listener::get_new_messages()
{
    const string buffer = _stream;
    _stream.clear();

    std::vector<Easy::Notification> v;
    for (const Easy::stream_event &event : Easy::parse_stream(buffer))
    {
        if (event.first == Easy::event_type::Notification)
        {
            v.push_back(Easy::Notification(event.second));
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
        cout << "DEBUG: Sent reply: " << message << '\n';
        return true;
    }
    else
    {
        cerr << "ERROR: " << ret << " (in " << __FUNCTION__ << ")\n";
        return false;
    }
}

const std::uint_fast64_t Listener::get_parent_id(Easy::Notification &notif)
{
    string answer;
    std::uint_fast16_t ret;

    ret = _masto->get(API::v1::statuses_id,
                      {{ "id", { std::to_string(notif.status().id()) }}},
                      answer);

    if (ret > 0)
    {
        cerr << "ERROR: " << ret << " (in " << __FUNCTION__ << ")\n";
        return 0;
    }
    else
    {
        Easy::Status s(answer);
        return s.in_reply_to_id();
    }
    return 0;
}

const bool Listener::stillrunning() const
{
    return _running;
}
