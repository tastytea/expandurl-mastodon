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
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include "version.hpp"
#include "expandurl-mastodon.hpp"

using std::cerr;
using std::string;
namespace curlopts = curlpp::options;

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
    const std::regex re("[\\?&]utm_[^&]+");
    return std::regex_replace(url, re, "");
}
