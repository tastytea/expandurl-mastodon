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

using std::string;

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
 *          Currently remoces all arguments beginning with `utm_`
 *
 *  @param  url     URL to filter
 *
 *  @return Filtered URL
 */
const string strip(const string &url);

#endif  // EXPANDURL_MASTODON_HPP
