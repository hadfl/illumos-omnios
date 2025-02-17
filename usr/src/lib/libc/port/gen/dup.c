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

/* Copyright 2013, OmniTI Computer Consulting, Inc. All rights reserved. */

/*
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * Copyright 2024 Oxide Computer Company
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/

#include	"lint.h"
#include	<sys/types.h>
#include	<fcntl.h>
#include	<errno.h>

#pragma weak _dup = dup
int
dup(int fildes)
{
	return (fcntl(fildes, F_DUPFD, 0));
}

#pragma weak _dup2 = dup2
int
dup2(int fildes, int fildes2)
{
	return (fcntl(fildes, F_DUP2FD, fildes2));
}

int
dup3(int fildes, int fildes2, int flags)
{
	int dflags = 0;

	/*
	 * dup3() only supports O_ open flags that translate into file
	 * descriptor flags in the F_GETFD sense.
	 */
	if (flags & ~(O_CLOEXEC | O_CLOFORK)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * This call differs from dup2 such that it is an error when
	 * fildes == fildes2
	 */
	if (fildes == fildes2) {
		errno = EINVAL;
		return (-1);
	}

	if ((flags & O_CLOEXEC) != 0)
		dflags |= FD_CLOEXEC;
	if ((flags & O_CLOFORK) != 0)
		dflags |= FD_CLOFORK;

	return (fcntl(fildes, F_DUP3FD, fildes2, dflags));
}
