**expandurl-mastodon** is a Mastodon bot that expands shortened URLs.

If you want the bot to expand an URL, reply to the post with the URL in it and
mention the bot account (`@expandurl@botsin.space` for example).

![Example screenshot](https://user-images.githubusercontent.com/3681516/39963736-908e3eea-5663-11e8-9a9c-55ca74279235.jpg)

This bot uses the same visibility as you, but posts unlisted instead of public.
It retains the sensitive flag and spoiler warnings.

Some tracking parameters, like those beginning with
[utm_](https://en.wikipedia.org/wiki/UTM_parameters) are stripped. It also tries
to rewrite [AMP](https://en.wikipedia.org/wiki/Accelerated_Mobile_Pages) URLs to
point at the real webpages.

Please report any bugs via the
[issue tracker on GitHub](https://github.com/tastytea/expandurl-mastodon/issues)
or to [@tastytea@soc.ialis.me](https://soc.ialis.me/@tastytea).

# Install

## Dependencies

 * Tested OS: Linux
 * C++ compiler (tested: gcc 6.4)
 * [cmake](https://cmake.org/) (tested: 3.9.6)
 * [curlpp](http://www.curlpp.org/) (tested: 0.8.1)
 * [mastodon-cpp](https://github.com/tastytea/mastodon-cpp) (at least: 0.13.1)
 * [jsoncpp](https://github.com/open-source-parsers/jsoncpp) (tested: 1.8.4)

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

**The config file has changed from cfg to JSON in 0.4.0.**

You will need to generate an access token yourself at the moment. Create a
config file with your account and access token in
`${HOME}/.config/expandurl-mastodon.json`:

    {
        "account": "expandurl@example.social",
        "access_tocken": "abc123"
    }

Now start expandurl-mastodon without parameters.

# Copyright

    Copyright © 2018 tastytea <tastytea@tastytea.de>.
    License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
    This program comes with ABSOLUTELY NO WARRANTY. This is free software,
    and you are welcome to redistribute it under certain conditions.
