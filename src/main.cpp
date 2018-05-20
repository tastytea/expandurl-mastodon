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

#include <iostream>
#include <chrono>
#include <csignal>
#include <regex>
#include <curlpp/cURLpp.hpp>
#include "expandurl-mastodon.hpp"

using std::cout;
using std::cerr;
using std::string;
using Mastodon::Easy;

bool running = true;

void signal_handler(int signum)
{
    switch (signum)
    {
        case SIGINT:
        case SIGTERM:
            if (!running)
            {
                cout << "Forced close.\n";
                exit(signum);
            }
            running = false;
            cout << "Closing, this could take a few seconds...\n";
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    curlpp::initialize();

    Listener listener;
    listener.start();

    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (!listener.stillrunning())
        {
            cout << "DEBUG: Reestablishing connection...\n";
            listener.stop();
            listener.start();
            cout << "DEBUG: Reestablished connection.\n";
        }

        for (Easy::Notification &notif : listener.get_new_messages())
        {
            cout << "DEBUG: new messages\n";
            const std::uint_fast64_t id = listener.get_parent_id(notif);
            cout << "DEBUG: in_reply_to_id: " << id << '\n';
            Easy::Status status;

            if (id > 0)
            {
                status = listener.get_status(id);
                if (status.valid())
                {
                    string message = "";
                    for (const string &url : get_urls(status.content()))
                    {
                        message += url + " \n";
                    }
                    if (!message.empty())
                    {
                        if (!listener.send_reply(notif.status(), message))
                        {
                            cerr << "ERROR: could not send reply to " << id << '\n';
                        }
                    }
                    else
                    {
                        listener.send_reply(notif.status(),
                            "I couldn't find an URL in the message you "
                            "replied to. 😞");
                    }
                }
                else
                {
                    listener.send_reply(notif.status(),
                        "I couldn't get the message you replied to. 😞");
                }
            }
            else
            {
                listener.send_reply(notif.status(),
                    "It seems that you didn't reply to a message?");
            }
        }
    }

    listener.stop();
    curlpp::terminate();

    return 0;
}
