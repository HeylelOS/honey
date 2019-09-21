# Prefix Format

A prefix directory must strictly follow these rules:

It must be only composed of entries in following types :
- Directory entry (a package)
- A symbolic link (a geist)

## Entries name format

No entry in the prefix can be hidden, therefore a name beginning with a '.' is forbidden.
And, as they are also directory entries, '/' is also forbidden.

### Package name form

A directory entry/package name must follow this regular expression:
	[^\./][^-/]+-[^/]+

### Geist name form

A symbolic link/geist must reference another entry in the prefix, which means it holds
either a directory name or an other geist's name, it must follow this regular expression:
	[^\./][^-/]+
which suppose every version-differentiating packages of one
geist are compatible one another.

