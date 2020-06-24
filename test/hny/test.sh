#!/bin/sh

test_hny() {
	set -e

	printf 'Testing extract\n'
	hny extract "$1"
	hny extract "$2"
	printf 'Testing shift (creation)\n'
	hny shift test_archive1 test_archive-1
	hny shift test_archive2 test_archive-2
	printf 'Testing status (one level)\n'
	hny status test_archive1 test_archive2
	printf 'Testing setup\n'
	hny setup test_archive1 test_archive2
	printf 'Testing shift (two levels)\n'
	hny shift test_archive test_archive1
	printf 'Testing status (two levels)\n'
	hny status test_archive
	printf 'Testing list (geister)\n'
	hny list geister
	printf 'Testing list (packages)\n'
	hny list packages
	printf 'Testing list (all)\n'
	hny list
	printf 'Testing remove (geister)\n'
	hny remove test_archive test_archive1 test_archive2
	printf 'Testing clean\n'
	hny clean test_archive-1 test_archive-2
	printf 'Testing remove (packages)\n'
	hny remove test_archive-1 test_archive-2
}

mktest_archive() {
	cd "${testdir}/$1"
	cpio -co < ../name-list | xz -C crc32 --lzma2 > "../$1.hny"
}

testdir="$PWD/`dirname $0`"
export HNY_PREFIX="${testdir}/hub"
mkdir -p "${HNY_PREFIX}"

printf 'Honey test at %s\n' "${testdir}"
date

printf 'Creating test archives\n'

(mktest_archive 'test_archive-1')
(mktest_archive 'test_archive-2')

printf 'Running tests\n'
(test_hny "${testdir}/test_archive-1.hny" "${testdir}/test_archive-2.hny")

rm -r "${testdir}/hub" "${testdir}"/*.hny

