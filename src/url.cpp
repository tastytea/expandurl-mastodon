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
#include <sstream>
#include <regex>
#include <array>
#include <utility>
#include <syslog.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include "version.hpp"
#include "expandurl-mastodon.hpp"

using std::string;
namespace curlopts = curlpp::options;

const std::vector<string> get_urls(const string &html)
{
    const std::regex re_url("href=\\\\?\"([^\"\\\\]+)\\\\?\"([^>]+)");
    std::smatch match;
    string buffer = html;
    std::vector<string> v;

    while (std::regex_search(buffer, match, re_url))
    {
        // Add URL to vector if it is not a mention.#
        if (match[2].str().find("mention") == std::string::npos)
        {
            string url = Easy::unescape_html(match[1].str());
            v.push_back(strip(expand(url)));
            buffer = match.suffix().str();
        }
    }

    return v;
}

const string expand(const string &url)
{
    curlpp::Easy request;
    std::stringstream ss;

    request.setOpt(curlopts::WriteStream(&ss));
    request.setOpt<curlopts::CustomRequest>("HEAD");
    request.setOpt<curlopts::Url>(url);
    request.setOpt<curlopts::UserAgent>
        (static_cast<const string>("expandurl-mastodon/") + global::version);
    request.setOpt<curlopts::HttpHeader>(
    {
        "Connection: close",
    });
    request.setOpt<curlopts::FollowLocation>(true);

    try
    {
        request.perform();
    }
    catch (const std::exception &e)
    {
        syslog(LOG_ERR, "%s", e.what());
        // TODO: Do something when: "Couldn't resolve host …"
        syslog(LOG_NOTICE, "The previous error is ignored.");
    }

    return curlpp::infos::EffectiveUrl::get(request);
}

const string strip(const string &url)
{
    using namespace std::regex_constants;
    Json::Value &config = configfile.get_json();
    string newurl = url;

    for (auto it = config["replace"].begin(); it != config["replace"].end();
         ++it)
    {
        newurl = std::regex_replace(newurl,
                                    std::regex(it.name(), icase),
                                    (*it).asString());
    }

    // If '&' is found in the new URL, but no '?'
    if (newurl.find('&') != std::string::npos &&
        newurl.find('?') == std::string::npos)
    {
        size_t pos = newurl.find('&');
        newurl.replace(pos, 1, "?");
    }

    return newurl;
}

const void init_replacements()
{
    using replace_pair = std::pair<const std::string, const std::string>;
    Json::Value &config = configfile.get_json();
    if (config["replace"].isNull())
    {
        const std::array<const replace_pair, 5> replace_array =
        {{
            { "[\\?&]utm_[^&]+", "" },                      // Google
            { "[\\?&]wt_?[^&]+", "" },                      // Twitter?
            { "[\\?&]__twitter_impression=[^&]+", "" },     // Twitter?
            { "//amp\\.", "//" }                            // AMP
        }};

        for (const replace_pair &pair : replace_array)
        {
            config["replace"][pair.first] = pair.second;
        }
    }
}
