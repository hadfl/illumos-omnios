#!/bin/ksh -p
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

#
# Copyright (c) 2012, 2016 by Delphix. All rights reserved.
#

. $STF_SUITE/include/libtest.shlib
. $STF_SUITE/tests/functional/cli_root/zpool_create/zpool_create.shlib

#
#
# DESCRIPTION:
# 'zpool create' will fail with ordinary file in swap
#
# STRATEGY:
# 1. Create a regular file on top of UFS-zvol filesystem
# 2. Try to create a new pool with regular file in swap
# 3. Verify the creation is failed.
#

verify_runnable "global"

function cleanup
{
	if datasetexists $vol_name; then
		swap -l | grep $TMP_FILE > /dev/null 2>&1
		if (( $? == 0 )); then
			log_must swap -d $TMP_FILE
		fi
		rm -f $TMP_FILE
		log_must umount $mntp
		zfs destroy $vol_name
	fi

	if poolexists $TESTPOOL; then
		destroy_pool $TESTPOOL
	fi
}

log_assert "'zpool create' should fail with regular file in swap."
log_onexit cleanup

if [[ -n $DISK ]]; then
        disk=$DISK
else
        disk=$DISK0
fi

typeset pool_dev=${disk}s${SLICE0}
typeset vol_name=$TESTPOOL/$TESTVOL
typeset mntp=/mnt
typeset TMP_FILE=$mntp/tmpfile.$$

create_pool $TESTPOOL $pool_dev
log_must zfs create -V 100m $vol_name
log_must eval "new_fs ${ZVOL_RDEVDIR}/$vol_name > /dev/null 2>&1"
log_must mount ${ZVOL_DEVDIR}/$vol_name $mntp

log_must mkfile 50m $TMP_FILE
log_must swap -a $TMP_FILE

for opt in "-n" "" "-f"; do
	log_mustnot zpool create $opt $TESTPOOL $TMP_FILE
done

log_pass "'zpool create' passed as expected with inapplicable scenario."
