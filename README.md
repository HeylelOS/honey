# honey
  
honey is intended to be a unix-like OS package manager and
archive structure format for packages. It is distributed under BSD 3-Clause
license, see [LICENSE.txt](https://github.com/ValentinDebon/honey/blob/master/LICENSE.txt).

It is meant to be composed of an ansi C library and
a command line utility.

It keeps in mind the desire to stay embeddable without
any compromise on its scalability. However it isn't
meant to be used directly, you should build your
providers around it. The command line utility is meant for
shell script providers, recovery or advanced users.

## Dependencies

The honey library only have one non-standard dependency:
- [libarchive](https://github.com/libarchive/libarchive), with xz and tar support

## Build

You should be able to configure and build it with:
```sh
./configure
make
```
Note this is an ansi C library/program,
with a few exceptions on some features. So you
shouldn't have any problem building it on most systems.
It has been built on Debian 9, Ubuntu 18.04 and void linux successfully.
macOS only have a linking problem due to not having the latest
[libarchive](https://github.com/libarchive/libarchive) on the system.

## Documentation

HTML and man documentations for the library are built using
[Doxygen](https://github.com/doxygen/doxygen):
```sh
make doc
```

## Tests

You should be able to run tests by executing:
```sh
make tests
```
