**expandurl-mastodon** is a Mastodon bot that expands a shortened URL.


# Install

## Dependencies

 * Tested OS: Linux
 * C++ compiler (tested: gcc 6.4, clang 5.0)
 * [cmake](https://cmake.org/) (tested: 3.9.6)
 * [curlpp](http://www.curlpp.org/) (tested: 0.8.4)
 * [mastodon-cpp](https://github.com/tastytea/mastodon-cpp) (at least: 0.12.0)

## Get sourcecode

### Development version

    git clone https://github.com/tastytea/expandurl-mastodon.git

## Compile

    mkdir build
    cd build/
    cmake ..
    make

Install with `make install`.

# Usage

## Error codes

Same as [mastodon-cpp](https://github.com/tastytea/mastodon-cpp/blob/master/README.md#error-codes)

# Copyright

    Copyright © 2018 tastytea <tastytea@tastytea.de>.
    License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
    This program comes with ABSOLUTELY NO WARRANTY. This is free software,
    and you are welcome to redistribute it under certain conditions.
