#!/bin/sh

name="expandurl-mastodon"

if [ -n "${1}" ]; then
    dir="$(dirname ${0})"
    cp -vf "${dir}/${name}.1.adoc" .
    sed -Ei "s/(Revision: +)[0-9]+\.[0-9]\.[0-9]/\1${1}/" "${name}.1.adoc"
    a2x --doctype manpage --format manpage --no-xmllint "${name}.1.adoc"
else
    echo "usage: ${0} VERSION" >&2
fi
