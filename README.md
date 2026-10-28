> **Play it with Asciiverse.** With [av](https://asciiverse.sh), the Asciiverse
> client:
>
> ```
> av install starlanes
> av play starlanes
> ```
>
> The original README follows.

# Star Lanes

This is a C version of the classic space trading game [_Star
Lanes_](https://github.com/beejjorgensen/starlanes-info) that I wrote
back in the mid 1990s based on the Osborne FOG disk source.

I was in college and my skills were poor. Lots of globals in this thing.
Could stand to be cleaned up a bit. It was one giant file, but I've
broken it up so we could all be less offended.

## Building

Build steps:

* Run `./conf` to configure.
* Run `make` to build.

Clean steps:

* `make clean` to remove intermediate build products.
* `make pristine` to remove all build products.

## Installation

You can set a prefix with:

```
./conf --prefix=/usr/local
```

And the man directory:

```
./conf --mandir=/usr/local/man
```

And you can install to a different root with `DESTDIR`:

```
make DESTDIR=fakeroot install
```

Or, last resort, as root you can just do this like it's 1999:

```
make install
```

## Version Info

David Barnsdale wrote an AI for this thing and released it as
[1.3.0](https://web.archive.org/web/20190701060458/https://www.barnsdle.demon.co.uk/game/starlanes.html).
But I'm having trouble getting that to run without crashing. So I've
forged ahead with modernizing the working one, and we'll see about
backporting the AI into it later.

This one is based on 1.2.2.

## TODO

* Fix resizing; at least make it not awful.
* ^C should work everywhere.
* Bot players

---

beej@beej.us

