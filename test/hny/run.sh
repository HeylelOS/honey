#!/bin/sh

# This test script must be run at projects's root
export HNY_PREFIX="$PWD/test/hny/prefix"

hnytest() {
	$@
	if [ "$?" -ne 0 ]
	then
		echo "[X] Test failure in command '$@'"
		rm -rf "$HNY_PREFIX"
		exit 1
	fi
}

echo "- Testing prefix $HNY_PREFIX"
hnytest mkdir -pv "$HNY_PREFIX"

echo "- Verifying test archives"
hnytest ./build/bin/hny -a verify "$PWD/test/hny/test1.hny" "$PWD/test/hny/test2.hny"

echo "- Exporting test archives"
hnytest ./build/bin/hny export "$PWD/test/hny/test1.hny" "test1-0.0.1"
hnytest ./build/bin/hny export "$PWD/test/hny/test2.hny" "test2-0.0.1"

echo "- Shifting test archives"
hnytest ./build/bin/hny shift "test1" "test1-0.0.1"
hnytest ./build/bin/hny shift "test2" "test2-0.0.1"

echo "- Statuses"
hnytest ./build/bin/hny status "test1" "test2"

echo "- Setups"
hnytest ./build/bin/hny setup "test1" "test2"

echo "- Testing links above links"
hnytest ./build/bin/hny shift "test" "test1"
hnytest ./build/bin/hny status "test"

echo "- Listing active"
hnytest ./build/bin/hny list active

echo "- Listing packages"
hnytest ./build/bin/hny list packages

echo "- Testing links above links"
hnytest ./build/bin/hny shift "test" "test2"
hnytest ./build/bin/hny status "test"
hnytest ./build/bin/hny erase "test"

echo "- Cleans"
hnytest ./build/bin/hny clean "test1" "test2"

echo "- Erasing"
hnytest ./build/bin/hny erase "test1" "test2"
hnytest ./build/bin/hny erase "test1-0.0.1" "test2-0.0.1"

echo "[O] All test passed"
rm -rf "$HNY_PREFIX"
exit 0
