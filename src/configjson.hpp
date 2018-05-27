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

#ifndef CONFIGJSON_HPP
#define CONFIGJSON_HPP

#include <string>
#include <jsoncpp/json/json.h>

using std::string;

class ConfigJSON
{
public:
    /*!
     *  @brief  Checks if subdir is present, creates it if necessary
     *
     *          Example:
     *  @code
     *          ConfigJSON config("test.json", "subdirectory");
     *  @endcode
     *
     *  @param  filename  The name of the file, including extension
     *  @param  subdir    The subdir (optional)
     */
    explicit ConfigJSON(const string &filename, const string &subdir = "");

    /*!
     *  @brief  Read the file
     *
     *  @return `true` on success
     */
    const bool read();

    /*!
     *  @brief  Write the file
     *
     *  @return `true` on success
     */
    const bool write();

    /*!
     *  @brief  Returns a reference to the config as Json::Value
     *
     *          Example:
     *  @code
     *          Json::Value &json = config.get_json();
     *  @endcode
     */
    Json::Value &get_json();

    /*!
     *  @brief  Gets the complete filepath
     */
    const string get_filepath() const;

private:
    /*!
     *  Holds the contents of the JSON file
     */
    Json::Value _json;

    /*!
     *  Complete filepath
     */
    string _filepath;
};

#endif  // CONFIGJSON_HPP
