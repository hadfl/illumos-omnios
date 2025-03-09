/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2017 Hayashi Naoyuki
 * Copyright 2023 Oxide Computer Co.
 * Copyright 2025 OmniOS Community Edition (OmniOSce) Association.
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/archsystm.h>
#include <sys/lockstat.h>

#include <sys/brand.h>
#include <sys/clock.h>
#include <sys/door.h>
#include <sys/debug.h>
#include <sys/smp_impldefs.h>
#include <sys/rtc.h>
#include <sys/zone.h>

#define	DOOR_PATH	"var/run/utmpd_door"
#define	ONE_DAY		(24 * 3600)

/*
 * This file contains all generic part of clock and timer handling.
 * Specifics are now in separate files and may be overridden by TOD
 * modules.
 */
tod_ops_t tod_ops;
char *tod_module_name;		/* Settable in /etc/system */

extern void tod_set_prev(timestruc_t);

static int
process_zone_cb(zone_t *zone, void *arg)
{
	door_handle_t door;
	int ret;
	time_t boot_ts;
	size_t dplen;
	char *door_path;

	/* ignore non-native zone brands */
	if (ZONE_IS_BRANDED(zone))
		return (0);

	boot_ts = *(time_t *)arg;
	dplen = zone->zone_rootpathlen + strlen(DOOR_PATH);

	door_path = kmem_alloc(dplen, KM_NOSLEEP);
	if (door_path == NULL)
		return (0);

	if (snprintf(door_path, dplen, "%s%s", zone->zone_rootpath,
	    DOOR_PATH) >= dplen) {
		goto out;
	}

	ret = door_ki_open(door_path, &door);
	if (ret != 0) {
		cmn_err(CE_WARN, "Time has stepped forwards; "
		    "failed to open door to utmpd for zone %s, err %d",
		    zone->zone_name, ret);

		goto out;
	}

	door_arg_t darg = {
		.data_ptr = (void *)&boot_ts,
		.data_size = sizeof (boot_ts),
	};

	ret = door_ki_upcall_limited(door, &darg, NULL, 0, 0);
	if (ret == 0) {
		cmn_err(CE_CONT, "?Time has stepped forwards; "
		    "successfully notified utmpd for zone %s.\n",
		    zone->zone_name);
	} else {
		cmn_err(CE_WARN, "Time has stepped forwards; "
		    "failed upcall to utmpd for zone %s, err %d",
		    zone->zone_name, ret);
	}

	door_ki_rele(door);

out:
	kmem_free(door_path, dplen);
	/*
	 * Since we want to keep going if a zone is not running utmpd
	 * for any reason, always return 0 here.
	 */
	return (0);
}

void
tod_set(timestruc_t ts)
{
	ASSERT(MUTEX_HELD(&tod_lock));

	if (tod_ops.tod_set) {
		tod_set_prev(ts);		/* for tod_validate() */
		TODOP_SET(tod_ops, ts);
		tod_status_set(TOD_SET_DONE);	/* TOD was modified */
	} else {
		/*
		 * There is no TOD unit, so there's nothing to do regarding
		 * that.
		 *
		 * However we take this opportunity to spot when the clock is
		 * stepped significantly forward, and use that as a cue that
		 * the system clock has been set initially after time
		 * synchronisation. When this happens we go through and update
		 * the global `boot_time` variable, and the `zone_boot_time`
		 * stored in each active zone (including the GZ) to correct the
		 * kstats and so that userland software can use this to obtain
		 * a more correct notion of the time that the system, and each
		 * zone, booted.
		 */
		time_t adj = ts.tv_sec - gethrestime_sec();
		extern time_t boot_time;

		if (adj < ONE_DAY)
			return;

		if (boot_time < INT64_MAX - adj)
			boot_time += adj;

		zone_boottime_adjust(adj);

		/*
		 * Call up to each zone's utmpd process and ask it to rewrite
		 * the utmpx and wtmpx database since the time has stepped
		 * forward. We drop tod_lock as this can take a while.
		 */
		time_t boot_ts = boot_time;
		mutex_exit(&tod_lock);
		(void) zone_walk(process_zone_cb, &boot_ts);
		mutex_enter(&tod_lock);
	}
}

timestruc_t
tod_get(void)
{
	timestruc_t ts;

	ASSERT(MUTEX_HELD(&tod_lock));

	if (tod_ops.tod_get) {
		ts = TODOP_GET(tod_ops);
	} else {
		gethrestime(&ts);
	}
	ts.tv_sec = tod_validate(ts.tv_sec);
	return (ts);
}

int
hr_clock_lock(void)
{
	ushort_t s;

	CLOCK_LOCK(&s);
	return (s);
}

void
hr_clock_unlock(int s)
{
	CLOCK_UNLOCK(s);
}

/*
 * Support routines for horrid GMT lag handling
 */

static time_t gmt_lag;		/* offset in seconds of gmt to local time */

void
sgmtl(time_t arg)
{
	gmt_lag = arg;
}

time_t
ggmtl(void)
{
	return (gmt_lag);
}
