#!/bin/sh
# Packs the build into dist/starlanes-<target>.tar.gz (.zip for Windows)
# for av: the binary at the top, with the man page, LICENSE.md, README.md
# and PATCHES.md.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
case "$target" in *windows*) exe=.exe ;; *) exe= ;; esac
stage=stage/starlanes
rm -rf "$stage" && mkdir -p "$stage" dist
cp "out/starlanes$exe" out/starlanes.6 LICENSE.md README.md PATCHES.md "$stage/"
if [ -n "$exe" ]; then
  (cd "$stage" && zip -qr "../../dist/starlanes-$target.zip" .)
else
  tar -czf "dist/starlanes-$target.tar.gz" -C "$stage" .
fi
ls dist
