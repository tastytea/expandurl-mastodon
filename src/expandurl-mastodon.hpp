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

#ifndef EXPANDURL_MASTODON_HPP
#define EXPANDURL_MASTODON_HPP

#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <cstdint>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include <mastodon-cpp/easy/all.hpp>
#include <jsoncpp/json/json.h>
#include "configjson.hpp"

using namespace Mastodon;

using std::string;
using Mastodon::API;

extern ConfigJSON configfile;

void signal_handler(int signum);

/*!
 *  @brief  Extract URLs from HTML
 *
 *  @return vector of URLs
 */
const std::vector<string> get_urls(const string &html);

/*!
 *  @brief  Expands shortened URLs
 *
 *  @param  url     URL to expand
 *
 *  @return Expanded URL
 */
const string expand(const string &url);

/*!
 *  @brief  Filters out tracking stuff
 *
 *          Currently removes all arguments beginning with `utm_`
 *
 *  @param  url     URL to filter
 *
 *  @return Filtered URL
 */
const string strip(const string &url);

/*!
 *  @brief  Initialize replacements for URLs
 *
 *          If no replacements are found in the config file, a default list is
 *          inserted.
 *
 */
void init_replacements();


class Listener
{
public:
    Listener();
    ~Listener();

    /*!
     *  @brief  Starts listening on Mastodon
     */
    void start();
    /*!
     *  @brief  Stops listening on Mastodon
     */
    void stop();

    const std::vector<Easy::Notification> get_new_messages();
    const std::vector<Easy::Notification> catchup();
    Easy::Status get_status(const string &id);
    bool send_reply(const Easy::Status &to_status, const string &message);
    const string get_parent_id(const Easy::Notification &notif);

    bool stillrunning() const;

private:
    string _instance;
    string _access_token;
    std::unique_ptr<Easy::API> _masto;
    string _stream;
    std::unique_ptr<API::http> _ptr;
    std::thread _thread;
    bool _running;
    string _proxy;
    string _proxy_user;
    string _proxy_password;
    Json::Value &_config;

    void read_config();
    bool write_config();
    bool register_app();
    void set_proxy(Easy::API &masto);
};

#endif  // EXPANDURL_MASTODON_HPP
