# Honey
  
Honey is intended to be a unix-like OS package manager,
archive structure format for packages and http/https remote
package database guideline. It is distributed under BSD 3-Clause
license, see [LICENSE.txt](https://github.com/ValentinDebon/Honey/blob/master/LICENSE.txt).

It is meant to be composed of an ansi C library and
a command line utility for now, plus a guideline
for future providers implementations.

It keeps in mind the desire to stay embeddable without
any compromise on its capabilities. However it isn't
meant to be used directly, you should build your
providers around it. The command line utility is meant for
shell script providers or advanced users.

## Dependencies

The Honey library only have one dependency:
- [libarchive](https://github.com/libarchive/libarchive), with xz and tar support

## Install

You should be able to execute it in any prefix,
so build it with:
```sh
make
```
and enjoy the certainly-full-of-bugs-pre-alpha!

