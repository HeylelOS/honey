# Honey Package format

The package is an archive which implements several mandatory files, and may implements others as features.

## Archive
The honey package file format is an _XZ stream with one lzma2 filter and crc32_ compressing an _odc cpio file archive_.
The choice for such a specific archive is to make honey packages as embeddable as possible without adding a huge backend to handle it.
Note concerning CPIO: Unlike other cpio, honey will not extract its files exactly where specified in the archive. If an absolute pathname is specified,
it will also be considered relative. And a pathname cannot contain a .. entry.

## Mandatory
- **hny/** : the directory containing all package related files.
- **hny/setup** : executable which sets up necessary files outside the prefix. Callable just after export and when already setup once (setup upgrade).
- **hny/clean** : executable which removes setup steps.
- **hny/eula** : file holding the license(s) of the package, may be displayed and agreed before the extract process.

## Optional
- **hny/reset** : executable reseting packages' configuration.
- **hny/check** : executable checking files from packages (not the one installed).
- **hny/purge** : executable which removes every user-related data.
- **hny/retail/** : A directory which is meant to contain platform specific package files.

