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
#include <syslog.h>
#include <unistd.h> // getuid()
#include <curlpp/cURLpp.hpp>
#include "expandurl-mastodon.hpp"

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
                syslog(LOG_NOTICE, "Forced close.");
                exit(signum);
            }
            running = false;
            syslog(LOG_NOTICE, "Received signal %d, closing...", signum);
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
    openlog("expandurl-mastodon", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_LOCAL0);
    syslog(LOG_NOTICE, "Program started by User %d", getuid());

    Listener listener;
    listener.start();
    std::vector<Easy::Notification> new_messages = listener.catchup();

    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (!listener.stillrunning())
        {
            listener.stop();
            syslog(LOG_DEBUG, "Reestablishing connection...");
            listener.start();
            syslog(LOG_NOTICE, "Reestablished connection.");
            new_messages = listener.catchup();
        }

        // Only get new messages if we don't have to catchup
        if (new_messages.size() == 0)
        {
            new_messages = listener.get_new_messages();
        }

        for (Easy::Notification &notif : new_messages)
        {
            syslog(LOG_DEBUG, "new message");
            const std::uint_fast64_t id = listener.get_parent_id(notif);
            syslog(LOG_DEBUG, "in_reply_to_id: %lu", id);
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
                            syslog(LOG_ERR, "could not send reply to %lu", id);
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
                    "I couldn't find the message you replied to. 😞 \n"
                    "Maybe the federation is a bit wonky at the moment.");
            }
        }
        new_messages.clear();
    }

    listener.stop();
    closelog();
    curlpp::terminate();

    return 0;
}
