#!/bin/sh

# This test script must be run at projects's root
export HNY_PREFIX="$1"

hnytest() {
	$@
	[ "$?" -ne 0 ] && exit 1
}

echo "Testing prefix $HNY_PREFIX"

echo "Verifying test archives"
hnytest hny verify -a "tests/clitests/test1.hny" "tests/clitests/test2.hny"

echo "Exporting test archives"
hnytest hny export "tests/clitests/test1.hny" "test1-0.0.1"
hnytest hny export "tests/clitests/test2.hny" "test2-0.0.1"

echo "Shifting test archives"
hnytest hny shift "test1" "test1-0.0.1"
hnytest hny shift "test2" "test2-0.0.1"

echo "Statuses"
hnytest hny status "test1" "test2"

echo "Setups"
hnytest hny setup "test1" "test2"

echo "Testing links above links"
hnytest hny shift "test" "test1"
hnytest hny status "test"
hnytest hny erase "test"

echo "Listing active"
hnytest hny list active
echo "Listing packages"
hnytest hny list packages

echo "Testing links above links"
hnytest hny shift "test" "test2"
hnytest hny status "test"
hnytest hny erase "test"

echo "Cleans"
hnytest hny clean "test1" "test2"

echo "Erasing"
hnytest hny erase "test1-0.0.1" "test2-0.0.1"

return 0
