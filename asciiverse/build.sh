#!/bin/sh
# Builds Star Lanes into out/: the twelve sources in one compiler run (one
# process, so no parallel jobs to limit), with the conf.h upstream's ./conf
# would write. The compiler, flags and curses library come from the
# environment, so one script serves every target (see
# .github/workflows/release.yml):
#
#   asciiverse/build.sh                                       # Linux
#   LDFLAGS="-static -s" asciiverse/build.sh                  # Alpine, static
#   CURSES_LIBS=-lncurses CFLAGS="-O2 -arch arm64" \
#     LDFLAGS="-arch arm64" asciiverse/build.sh               # macOS
#   CC=x86_64-w64-mingw32-gcc PDCURSES=/path/to/PDCurses-3.9 \
#     EXE=.exe LDFLAGS="-static -s" asciiverse/build.sh       # Windows
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
EXE="${EXE:-}"
CFLAGS="${CFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
CURSES_LIBS="${CURSES_LIBS:--lncursesw}"
CURSES_HEADER="${CURSES_HEADER:-curses.h}"
INCLUDES="-I."
if [ -n "${PDCURSES:-}" ]; then
  INCLUDES="$INCLUDES -I$PDCURSES"
  CURSES_LIBS="$PDCURSES/wincon/pdcurses.a"
fi
cat > conf.h <<CONF
#ifndef CONF_H
#define CONF_H

/* Written by asciiverse/build.sh. */

#define CURSES_HEADER <$CURSES_HEADER>

#endif
CONF
mkdir -p out
rm -f "out/starlanes$EXE"
# shellcheck disable=SC2086
nice -n 10 $CC -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra $CFLAGS $INCLUDES -o "out/starlanes$EXE" \
  announce.c company.c globals.c map.c merge.c move.c player.c quit.c \
  standings.c starlanes.c trade.c ui.c $CURSES_LIBS $LDFLAGS
cp starlanes.6 out/starlanes.6
echo "built starlanes$EXE"
