#!/bin/sh

export HNY_PREFIX="`pwd`/tests/prefix"

if [ -d "$HNY_PREFIX" ]
then
	rm -rf "$HNY_PREFIX"
fi

mkdir "$HNY_PREFIX"

echo "Testing prefix $HNY_PREFIX"

./build/bin/hny verify "tests/test1.hny" "tests/test2.hny"

./build/bin/hny export "tests/test1.hny" "test1-0.0.1"
./build/bin/hny export "tests/test2.hny" "test2-0.0.1"

./build/bin/hny shift "test1" "test1-0.0.1"
./build/bin/hny shift "test2" "test2-0.0.1"

./build/bin/hny status "test1" "test2"

./build/bin/hny setup "test1" "test2"

./build/bin/hny shift "test" "test1"
./build/bin/hny status "test"
./build/bin/hny erase "test"

echo "Listing active"
./build/bin/hny list active
echo "Listing packages"
./build/bin/hny list packages

./build/bin/hny shift "test" "test2"
./build/bin/hny status "test"
./build/bin/hny erase "test"

./build/bin/hny clean "test1" "test2"

./build/bin/hny erase "test1-0.0.1" "test2-0.0.1"

