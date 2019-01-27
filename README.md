**expandurl-mastodon** is a Mastodon bot that expands shortened URLs.

If you want the bot to expand an URL, reply to the post with the URL in it and
mention the bot account (`@expandurl@botsin.space` for example).

![Example screenshot](https://doc.schlomp.space/expandurl-mastodon/expandurl_screenshot.jpg)

This bot uses the same visibility as you, but posts unlisted instead of public.
It retains the sensitive flag and spoiler warnings.

Some tracking parameters, like those beginning with
[utm_](https://en.wikipedia.org/wiki/UTM_parameters) are stripped. It also tries
to rewrite [AMP](https://en.wikipedia.org/wiki/Accelerated_Mobile_Pages) URLs to
point at the real webpages.

Please report any bugs via the
[issue tracker on schlomp.space](https://schlomp.space/tastytea/expandurl-mastodon/issues)
or to [@tastytea@soc.ialis.me](https://soc.ialis.me/@tastytea).

# Install

## Dependencies

 * Tested OS: Linux
 * C++ compiler (tested: gcc 6/7/8)
 * [cmake](https://cmake.org/) (tested: 3.9 / 3.11)
 * [curlpp](http://www.curlpp.org/) (tested: 0.8)
 * [mastodon-cpp](https://schlomp.space/tastytea/mastodon-cpp) (at least: 0.30)
 * [jsoncpp](https://github.com/open-source-parsers/jsoncpp) (tested: 1.8 / 1.7)
 * [libxdg-basedir](http://repo.or.cz/w/libxdg-basedir.git) (tested: 1.2)

## Get sourcecode

### Latest release

https://schlomp.space/tastytea/expandurl-mastodon/releases

### Development version

```SH
git clone https://schlomp.space/tastytea/expandurl-mastodon.git
```

## Compile

```SH
mkdir build
cd build/
cmake ..
make
```

Install with `make install`.

# Usage

**The config file has changed from cfg to JSON in 0.4.0.**

Start expandurl-mastodon without parameters.

If no config file is found, you will be asked to provide your account address
and an access token is generated. The config file can be found in
`${HOME}/.config/expandurl-mastodon.json` and looks like this:

```JSON
{
    "account": "expandurl@example.social",
    "access_token": "abc123",
    "proxy":
    {
        "url": "socks5h://[::1]:1080/",
        "user": "user23",
        "password": "supersecure"
    },
    "replace" :
    {
            "//amp\\." : "//",
            "[\\?&]__twitter_impression=[^&]+" : "",
            "[\\?&]utm_[^&]+" : "",
            "[\\?&]wt_zmc=[^&]+" : "",
            "[\\?&]wtmc=[^&]+" : ""
    }
}
```

If you want to use a proxy or define your own replacements, you have to edit the
configuration file manually. After the configuration file is generated, you can
start expandurl-mastodon as
daemon.

# Copyright

```PLAIN
Copyright © 2018 tastytea <tastytea@tastytea.de>.
License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
This program comes with ABSOLUTELY NO WARRANTY. This is free software,
and you are welcome to redistribute it under certain conditions.
```
