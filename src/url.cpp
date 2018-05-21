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
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include "version.hpp"
#include "expandurl-mastodon.hpp"

using std::cerr;
using std::string;
namespace curlopts = curlpp::options;

const std::vector<string> get_urls(const string &html)
{
    const std::regex re_url("href=\"([^\"]+)\" rel");
    std::smatch match;
    string buffer = html;
    std::vector<string> v;

    while (std::regex_search(buffer, match, re_url))
    {
        string url = Easy::unescape_html(match[1].str());
        v.push_back(strip(expand(url)));
        buffer = match.suffix().str();
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
        cerr << "ERROR: " << e.what() << '\n';
        cerr << "The previous error is ignored.\n";
    }

    return curlpp::infos::EffectiveUrl::get(request);
}

const string strip(const string &url)
{
    using replace_pair = std::pair<const std::regex, const std::string>;
    using namespace std::regex_constants;
    string newurl = url;

    const std::array<const replace_pair, 4> replace_array =
    {{
        { std::regex("[\\?&]utm_[^&]+", icase), "" },                   // Google
        { std::regex("[\\?&]wtmc=[^&]+", icase), "" },                  // Twitter
        { std::regex("[\\?&]__twitter_impression=[^&]+", icase), "" },  // Twitter
        { std::regex("//amp\\.", icase), "//" },                        // AMP
    }};

    for (const replace_pair &pair : replace_array)
    {
        newurl = std::regex_replace(newurl, pair.first, pair.second);
    }

    return newurl;
}
