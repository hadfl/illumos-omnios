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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2015 Joyent, Inc.
 * Copyright 2025 Oxide Computer Company
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/

#include "lint.h"
#include "_libc_gettext.h"
#include "syserr.h"
#include <string.h>
#include <sys/types.h>
#include <errno.h>

char *
strerror_l(int errnum, locale_t loc)
{
	if (errnum < _sys_num_nerr && errnum >= 0)
		return (_libc_gettext_l(&_sys_nerrs[_sys_nindex[errnum]],
		    loc));

	errno = EINVAL;
	return (_libc_gettext_l("Unknown error", loc));
}

char *
strerror(int errnum)
{
	return (strerror_l(errnum, uselocale(NULL)));
}

/*
 * A version of sterror() that always operates in the C locale. It returns NULL
 * rather than the "Unknown error" string.
 */
const char *
strerrordesc_np(int errnum)
{
	if (errnum < _sys_num_nerr && errnum >= 0)
		return (&_sys_nerrs[_sys_nindex[errnum]]);

	errno = EINVAL;
	return (NULL);
}

const char *
strerrorname_np(int errnum)
{
	if (errnum >= 0 && errnum < _sys_num_nerr &&
	    _sys_err_names[errnum] != NULL) {
		return (_sys_err_names[errnum]);
	}

	errno = EINVAL;
	return (NULL);
}

/*
 * Implemented strerror_r in Solaris 10 to comply with SUSv3 2001.
 */
int
strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
	char *buf;
	int ret = 0;

	if (errnum < _sys_num_nerr && errnum >= 0) {
		buf = _libc_gettext((char *)&_sys_nerrs[_sys_nindex[errnum]]);
	} else {
		buf = _libc_gettext("Unknown error");
		ret = errno = EINVAL;
	}

	/*
	 * At compile time, there is no way to determine the max size of
	 * language-dependent error message.
	 */
	if (buflen < (strlen(buf) + 1)) {
		ret = errno = ERANGE;
	} else {
		(void) strcpy(strerrbuf, buf);
	}

	return (ret);
}
