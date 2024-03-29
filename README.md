# Honey
  
honey is intended to be a unix-like OS package manager and archive structure
format for packages. It is distributed under BSD 3-Clause license, see LICENSE.

It is meant to be composed of a C library and a command line utility.

It keeps in mind the desire to stay embeddable without
any compromise on its scalability. However it isn't
meant to be used directly, you should build your
providers around it. The command line utility is meant for
shell script providers, recovery or advanced users.

## How to make and extract a package?

The format is documented in the repository. Supposing the archive name is in _$PACKAGE_
you may respectively create and extract honey packages with the following commands:

```sh
cpio -c -o | xz -C crc32 --lzma2 > "$PACKAGE"
unxz -C crc32 --lzma2 < "$PACKAGE" | cpio -c -i
```

Note: You can also replace `crc32` with `none`.

## Configure, build and install

Meson is used to configure, build and install binaires and documentations:

```sh
meson setup build
meson compile -C build
meson install -C build
```

## Documentation

HTML documentation for the library is built using [Doxygen](https://github.com/doxygen/doxygen).
The documentation is built through the `doc` target.

```sh
meson compile -C build doc
```

## Tests

Meson is also used to manage the test suite:

```sh
meson test -C build
```
