#!/bin/sh

export HNY_PREFIX="`pwd`/tests/prefix"

if [ -d "$HNY_PREFIX" ]
then
	rm -rf "$HNY_PREFIX"
fi

mkdir "$HNY_PREFIX"

echo "Testing prefix $HNY_PREFIX"

echo "Verifying test archives"
hny verify "tests/test1.hny" "tests/test2.hny"

echo "Exporting test archives"
hny export "tests/test1.hny" "test1-0.0.1"
hny export "tests/test2.hny" "test2-0.0.1"

echo "Shifting test archives"
hny shift "test1" "test1-0.0.1"
hny shift "test2" "test2-0.0.1"

echo "Statuses"
hny status "test1" "test2"

echo "Setups"
hny setup "test1" "test2"

echo "Testing links above links"
hny shift "test" "test1"
hny status "test"
hny erase "test"

echo "Listing active"
hny list active
echo "Listing packages"
hny list packages

echo "Testing links above links"
hny shift "test" "test2"
hny status "test"
hny erase "test"

echo "Cleans"
hny clean "test1" "test2"

echo "Erasing tests"
hny erase "test1-0.0.1" "test2-0.0.1"

