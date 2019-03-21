# Honey Package format

The honey package file format consists of a simple .tar.xz archive which implements the following files:

## Mandatory:
hny/ : the directory containing all package related files.
hny/setup : executable which sets up necessary files outside the prefix. Callable just after export and when already setup once (setup upgrade).
hny/clean : executable which removes setup steps.
hny/eula : file holding the license(s) of the package, may be displayed and agreed before the extract process.

## Optional:
hny/reset : executable reseting packages' configuration.
hny/check : executable checking files from packages (not the one installed).
hny/purge : executable which removes every user-related data.
hny/retail/ : A directory which is meant to contain platform specific package files.
