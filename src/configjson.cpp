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
#include <experimental/filesystem>
#include <basedir.h>
#include <sstream>
#include "configjson.hpp"

using std::string;
namespace fs = std::experimental::filesystem;

ConfigJSON::ConfigJSON(const string &filename, const string &subdir)
: _json()
{
    xdgHandle xdg;
    xdgInitHandle(&xdg);
    _filepath = xdgConfigHome(&xdg);
    xdgWipeHandle(&xdg);

    if (!subdir.empty())
    {
        _filepath += '/' + subdir;
        if (!fs::exists(_filepath))
        {
            fs::create_directory(_filepath);
        }
    }
    _filepath += '/' + filename;
}

const bool ConfigJSON::read()
{
    std::ifstream file(_filepath);
    if (file.is_open())
    {
        std::stringstream config;
        config << file.rdbuf();
        file.close();
        config >> _json;

        return true;
    }
    else
    {
        return false;
    }
}

const bool ConfigJSON::write()
{
    std::ofstream file(_filepath);
    if (file.is_open())
    {
        const string config = _json.toStyledString();
        file.write(config.c_str(), config.length());
        file.close();

        return true;
    }
    else
    {
        return false;
    }
}

Json::Value &ConfigJSON::get_json()
{
    return _json;
}

const string ConfigJSON::get_filepath() const
{
    return _filepath;
}
