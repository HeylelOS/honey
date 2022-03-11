# Honey Package format

The package is a simple archive which may follow the suggested hierarchy locations.

## Archive
The honey package file format is an _XZ stream with one lzma2 filter and crc32 or no check_ compressing an _odc cpio file archive_.
The choice for such a specific archive is to make honey packages as embeddable as possible without adding a huge backend to handle it.
Notes concerning CPIO:
- Paths from the archive are 'normalized', removing `.` and `..` entries, and prefix `/`. An empty entry or one resolving to `/` is considered invalid.
- Every directory must be explicitly declared and precede in declaration any file/directory it contains, to allow a continuous streamable extraction.

## Hierarchy suggested locations
- **bin/** : Binary/Script executables.
- **lib/** : Static/shared libraries.
- **man/** : Manpages documentation.
- **doc/** : Other documentations (eg. HTML).
- **conf/** : Embedded/default configurations.
- **src/** : The package's source files.
- **include/** : The package's developement headers.
- **pkg/** : Package-lifetime related files.
- **pkg/setup** : Executable which sets up files outside the prefix.
- **pkg/clean** : Executable which undoes setup steps.
- **pkg/eula** : File holding the license(s) of the package, may be displayed and agreed before the extract process.
- **pkg/reset** : Executable resetting packages' configuration.
- **pkg/check** : Executable checking integrity of the setup (and users config dirs and data).
- **pkg/purge** : Executable which removes every user-related data.

