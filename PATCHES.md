# Changes from upstream

This is the Asciiverse packaging of Star Lanes, Brian "Beej" Hall's curses
version of the classic space trading game (Steven Faber's Star Lanes,
which Beej knew from a First Osborne Group disk). Beej wrote it in
1995 to 1997 as one big C file; 1.2.2 (1997-03-29) was its last release
for almost thirty years. In February 2026 he picked it up again at
github.com/beejjorgensen/starlanes-c (the `upstream` remote): the code
broken into modules, modern compiler fixes, a small `./conf` script, and
release 1.4.0.

Branch `asciiverse` starts at upstream `main` `2c4b2bf` (2026-03-05,
four commits after the v1.4.0 tag; one of them fixes a name clash with
curses' `LINES`), with full history.

## The 1.2.2 release tarball

Upstream's first commit (`73ca63a`, tagged `orig-starlanes-1.2.2` and
`v1.2.2`) is the 1.2.2 release. It was checked file by file against the
original tarball, starlanes-1.2.2.tar.gz from ibiblio
(https://www.ibiblio.org/pub/Linux/games/strategy/, sha256
0ce2db991e8a707c65eeb7799cbe4d6c39c623b86c1a9999b0146076194d612a, the
same sha256 FreeBSD's games/starlanes port pins): all seven source and
document files are identical; only the 1997 prebuilt `starlanes` binary
in the tarball is left out of the commit.

## Distribution patches

Repology (2026-10-18) lists three packages: FreeBSD games/starlanes and
SlackBuilds games/starlanes (both 1.2.2), and os4depot's AmigaOS 4 build of
1.3.0. Debian, Fedora and Arch do not carry it (the AUR's was archived in
2015).

- FreeBSD's port has no patch files; it renames the identifier `restrict`
  (a C99 keyword) when building, and the SlackBuild does the same with
  sed. Upstream's 2026 code no longer has that identifier, so nothing is
  taken.

## Licence

The 1.2.2 source says "GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version". In February 2026 upstream's README said for
a few hours that Beej had changed it to the Unlicense ("this one (v1.2.2)
was entirely written by me"), then the paragraph was dropped and
LICENSE.md became the GPL version 2 text, which is what upstream ships
now; the 2026 source files carry no notice. The catalog says
GPL-2.0-only, the terms upstream's LICENSE.md states, which is also safe
under the 1.2.2 grant.

## Ours

- **Centred in the terminal, also after a resize.** The 80x24 screen was
  drawn at the top left, and the man page says "everything gets hosed on
  a window resize". Windows are now made through `sl_newwin` (ui.c),
  which places them centred and remembers them, and keys are read through
  `sl_getch`, which on KEY_RESIZE centres everything again. A window
  ncurses cut down for a smaller terminal is grown back and refilled from
  a pad copy taken before each key, so nothing is lost when the terminal
  grows again. The title screen is one row shorter (24 instead of 25) so
  the game fits an 80x24 terminal, and a terminal smaller than that gets a
  message instead of a crash.
- **Windows.** `resize_term(0, 0)` after KEY_RESIZE under PDCurses, which
  leaves that call to the program; otherwise the code builds with MinGW
  against PDCurses as it is.

Not changed: there are still no computer players (upstream's TODO; David
Barnsdale's 1.3.0 added one, which Beej could not get to run without
crashing), and money is a C `int`, so a runaway single-player fortune
past $2 billion overflows, as the man page warns.

## Packaging (new files)

- `asciiverse/build.sh`, `asciiverse/package.sh`: one compiler call (with
  the conf.h upstream's `./conf` would write), and one archive per target
  with the binary, its man page, LICENSE.md, README.md and this file.
- `asciiverse/record-preview.py`, `asciiverse/starlanes.cast`: the catalog
  preview (100x30, 53 s) and the player that recorded it, reading the
  screen through pyte: a two-player game that takes the offered move next
  to a company, a new company marker or a star, buys what it can afford,
  and quits into the final standings.
- `.github/workflows/release.yml`: static Linux in Alpine (x86_64,
  aarch64), macOS (x86_64, arm64) and Windows x86_64 (MinGW, PDCurses),
  plus SHA256SUMS, on a tag.
