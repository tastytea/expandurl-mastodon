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
* Optional:
    * Manpage: [asciidoc](http://asciidoc.org/) (tested: 8.6)

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

cmake options:
* `-DCMAKE_BUILD_TYPE=Debug` for a debug build
* `-DWITH_MAN=NO` to not compile the manpage

Install with `make install`.

# Usage

Have a look at the [manpage](https://schlomp.space/tastytea/expandurl-mastodon/src/branch/master/expandurl-mastodon.1.adoc).

# Copyright

```PLAIN
Copyright © 2018, 2019 tastytea <tastytea@tastytea.de>.
License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
This program comes with ABSOLUTELY NO WARRANTY. This is free software,
and you are welcome to redistribute it under certain conditions.
```
