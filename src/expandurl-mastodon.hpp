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

using std::string;
using Mastodon::API;
using Mastodon::Easy;

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
const void init_replacements();


class Listener
{
public:
    Listener();
    ~Listener();

    /*!
     *  @brief  Starts listening on Mastodon
     */
    const void start();
    /*!
     *  @brief  Stops listening on Mastodon
     */
    const void stop();

    const std::vector<Easy::Notification> get_new_messages();
    const std::vector<Easy::Notification> catchup();
    Easy::Status get_status(const std::uint_fast64_t &id);
    const bool send_reply(const Easy::Status &to_status, const string &message);
    const std::uint_fast64_t get_parent_id(const Easy::Notification &notif);

    const bool stillrunning() const;

private:
    string _instance;
    string _access_token;
    std::unique_ptr<Easy> _masto;
    string _stream;
    std::unique_ptr<API::http> _ptr;
    std::thread _thread;
    bool _running;
    string _proxy;
    string _proxy_user;
    string _proxy_password;
    Json::Value &_config;

    const void read_config();
    const bool write_config();
    const bool register_app();
    const void set_proxy(Easy &masto);
};

#endif  // EXPANDURL_MASTODON_HPP
