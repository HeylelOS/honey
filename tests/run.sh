#!/bin/sh

export HNY_PREFIX="`pwd`/tests/prefix"

if [ -d "$HNY_PREFIX" ]
then
	rm -rf "$HNY_PREFIX"
fi

mkdir "$HNY_PREFIX"

echo "Testing prefix $HNY_PREFIX"

echo "Verifying test archives"
./build/bin/hny verify "tests/test1.hny" "tests/test2.hny"

echo "Exporting test archives"
./build/bin/hny export "tests/test1.hny" "test1-0.0.1"
./build/bin/hny export "tests/test2.hny" "test2-0.0.1"

echo "Shifting test archives"
./build/bin/hny shift "test1" "test1-0.0.1"
./build/bin/hny shift "test2" "test2-0.0.1"

echo "Statuses"
./build/bin/hny status "test1" "test2"

echo "Setups"
./build/bin/hny setup "test1" "test2"

echo "Testing links above links"
./build/bin/hny shift "test" "test1"
./build/bin/hny status "test"
./build/bin/hny erase "test"

echo "Listing active"
./build/bin/hny list active
echo "Listing packages"
./build/bin/hny list packages

echo "Testing links above links"
./build/bin/hny shift "test" "test2"
./build/bin/hny status "test"
./build/bin/hny erase "test"

echo "Cleans"
./build/bin/hny clean "test1" "test2"

echo "Erasing tests"
./build/bin/hny erase "test1-0.0.1" "test2-0.0.1"

