# NAME
hny - Command line utility to repair or access honey prefixes.

# SYNOPSIS
**hny** [-hb] [-p \<prefix\>] extract [\<geist\>] \<file\>

**hny** [-h] [-p \<prefix\>] list [packages|geister]

**hny** [-hb] [-p \<prefix\>] remove [\<entry\>...]

**hny** [-hb] [-p \<prefix\>] shift \<geist\> \<target\>

**hny** [-h] [-p \<prefix\>] status [\<geist\>...]

**hny** [-h] [-p \<prefix\>] \<command\> [\<entry\>...]

# DESCRIPTION
This command line interface is only meant to be used in shell scripts or by advanced users, either for fun or to repair a broken prefix.

# OPTIONS
-h : Prints usage and exits.

-b : Sets the honey blocking behavior to blocking, as it is non-blocking by default.

-p \<prefix\> : To specify a prefix manually, overrides the value in **HNY_PREFIX**.

extract [\<geist\>] \<file\> : Unpacks **file** in the prefix, with the specified **geist**, or its basename else.

list [packages|geister] : Lists respectively directories, or symlinks in the prefix.

remove [\<entry\>...] : Removes **entry**, unlinks it if a symlink, removes files if a package.

shift \<geist\> \<target\> : Associates **target** to **geist**.

status [\<geist\>...] : Returns which package **geist** points too.

setup [\<entry\>...] : Executes the **pkg/setup** routine for the specified **entry**.

clean [\<entry\>...] : Executes the **pkg/clean** routine for the specified **entry**.

reset [\<entry\>...] : Executes the **pkg/reset** routine for the specified **entry**.

check [\<entry\>...] : Executes the **pkg/check** routine for the specified **entry**.

purge [\<entry\>...] : Executes the **pkg/purge** routine for the specified **entry**.

# ENVIRONMENT
HNY\_PREFIX : Indicates the defaut honey prefix, else **/hub** is used.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
cpio(5), xz(5).

