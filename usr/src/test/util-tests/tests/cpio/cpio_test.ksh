#!/usr/bin/ksh
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright 2022 OmniOS Community Edition (OmniOSce) Association.
#

set -e
set -o pipefail
export LC_ALL=C

CPIO=${CPIO:-/usr/bin/cpio}
SRCDIR=$(dirname $0)
FILES=$SRCDIR/files

typeset -i failures=0

function errexit {
	echo "$@" >&2
	exit 1
}

function fail {
	echo "FAIL: $@" >&2
	((failures++))
	true
}

function pass {
	echo "PASS: $@"
}

function find_cmd {
	typeset cmd="$1"
	typeset var=$(echo $cmd | tr '[:lower:]' '[:upper:]')
	typeset -n path="$var"
	path=$(whence -fp "$cmd")
	if (($? != 0)) || [ ! -x "$path" ]; then
		errexit "Cannot find executable '$cmd' in PATH"
	fi
}

# This script uses a few commands which are not part of illumos and are
# expected to be available in the path.
find_cmd gtar
find_cmd stat

# Test cpio's handling of device nodes across different formats.
# To do this, we need a device file to include in the archive.

typeset -i maj
typeset -i min

# To allow this test to run without root privileges, and in a non-global zone,
# we look for a suitable device for each one. Such a device must not have a
# zero minor number and both major and minor must be small enough to fit within
# the old SVR3 types, so we restrict both to 0x7f.
if [[ $(zonename) == global ]]; then
	DEVPATH=/devices/pseudo
else
	DEVPATH=/dev
fi
DEVICE=
for device in $DEVPATH/*; do
	[[ -c "$device" ]] || continue
	set -- $($STAT -c '%Hr %Lr' $device)
	maj=$1; min=$2
	((maj == 0 || min == 0)) && continue
	((maj > 0x7f || min > 0x7f)) && continue
	DEVICE="$device"
	break
done
[[ -z $DEVICE ]] && errexit "No suitable device node found for test"

typeset expect_cpio=$(printf "%d,%3d" $maj $min)
typeset expect_gtar=$(printf "%d,%d" $maj $min)

echo "Using $DEVICE ($maj/$min)"

# Create archives using GNU tar and check that cpio correctly extracts the
# nodes.
for f in posix ustar; do
	set -- $($GTAR --format=$f -cf - $DEVICE 2>/dev/null | \
	    $CPIO -H ustar -ivt 2>/dev/null | grep ${DEVICE#/})
	if echo "$@" | egrep -s "$expect_cpio"; then
		pass "gtar->cpio($f)"
	else
		fail "gtar->cpio($f) $@"
	fi
done

# Now the inverse, create the archives using cpio and confirm that GNU tar
# extracts them properly.

for f in tar ustar; do
	set -- $(echo $DEVICE | $CPIO -H $f -o 2>/dev/null | \
	    $GTAR tvf - 2>/dev/null)
	if echo "$@" | egrep -s "$expect_gtar"; then
		pass "cpio->gtar($f)"
	else
		fail "cpio->gtar($f) $@"
	fi
done

# Now cpio-generated archives passed into cpio for extraction

for f in crc odc odc_sparse ascii_sparse ustar; do
	set -- $(echo $DEVICE | $CPIO -H $f -o 2>/dev/null | \
	    $CPIO -H $f -ivt 2>/dev/null | grep ${DEVICE#/})
	if echo "$@" | egrep -s "$expect_cpio"; then
		pass "cpio->cpio($f)"
	else
		fail "cpio->cpio($f) $@"
	fi
done

# And a cpio archive with no format specified.
set -- $(echo $DEVICE | $CPIO -o 2>/dev/null | \
    $CPIO -ivt 2>/dev/null | grep ${DEVICE#/})

if echo "$@" | egrep -s "$expect_cpio"; then
	pass "cpio->cpio(native)"
else
	fail "cpio->cpio(native) $@"
fi

# Test extracting cpio samples created on FreeBSD.
# These all have maj/min 13/17 in them.
expect_cpio=$(printf "%d,%3d" 13 17)
for f in $FILES/freebsd.*.cpio; do
	format=${f%.*}
	format=${format##*.}
	[[ $format = pax || $format == ustar ]] && flags="-H ustar" || flags=
	set -- $($CPIO $flags -ivt < $f 2>/dev/null | grep node | grep -v Pax)
	if echo "$@" | egrep -s "$expect_cpio"; then
		pass "freebsd->cpio($format)"
	else
		fail "freebsd->cpio($format) $@"
	fi
done

# This is a 'bar' file created on a SunOS 4.x system. It contains a
# /dev/zero device node with major/minor 3/12
expect_cpio=$(printf "%d,%3d" 3 12)

set -- $($CPIO -H bar -ivt < $FILES/zero.bar 2>/dev/null | \
    grep test/zero | head -1)
if echo "$@" | egrep -s "$expect_cpio"; then
	pass "sunos->cpio(bar)"
else
	fail "sunos->cpio(bar) $@"
fi

exit $FAILURES

