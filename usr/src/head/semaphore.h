/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2014 Garrett D'Amore <garrett@damore.org>
 *
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SEMAPHORE_H
#define	_SEMAPHORE_H

#include <sys/feature_tests.h>

#include <sys/types.h>
#include <sys/fcntl.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Including <time.h> has an unfortunate XPG/POSIX history. In XPG6 and POSIX
 * 1999, sem_timedwait() was added which required making the struct timespec
 * declared in some form. In XPG7, this is permitted to make all of time.h
 * visible. In XPG8, the use of the clockid_t is required from time.h. For
 * better and worse, that is how the ifdef soup is here. We prefer including
 * time.h so that folks can get a full definition for the time struct timespec
 * rather than the forward declaration that most other OSes do here.
 */
#if !defined(_STRICT_POSIX) || defined(_XPG7)
#include <time.h>
#elif defined(_XPG6)
struct timespec;
#endif

typedef struct {
	/* this structure must be the same as sema_t in <synch.h> */
	uint32_t	sem_count;	/* semaphore count */
	uint16_t	sem_type;
	uint16_t	sem_magic;
	upad64_t	sem_pad1[3];	/* reserved for a mutex_t */
	upad64_t	sem_pad2[2];	/* reserved for a cond_t */
} sem_t;

#define	SEM_FAILED	((sem_t *)(-1))

/*
 * function prototypes
 */
int	sem_init(sem_t *, int, unsigned int);
int	sem_destroy(sem_t *);
sem_t	*sem_open(const char *, int, ...);
int	sem_close(sem_t *);
int	sem_unlink(const char *);
int	sem_wait(sem_t *);

#if !defined(_STRICT_POSIX) || defined(_XPG6)
int	sem_timedwait(sem_t *_RESTRICT_KYWD,
		const struct timespec *_RESTRICT_KYWD);
int	sem_reltimedwait_np(sem_t *_RESTRICT_KYWD,
		const struct timespec *_RESTRICT_KYWD);
#endif	/* !_STRICT_POSIX || _XPG6 */

#if !defined(_STRICT_POSIX) || defined(_XPG8)
int	sem_clockwait(sem_t *_RESTRICT_KYWD, clockid_t,
		const struct timespec *_RESTRICT_KYWD);
int	sem_relclockwait_np(sem_t *_RESTRICT_KYWD, clockid_t,
		const struct timespec *_RESTRICT_KYWD);
#endif	/* !_STRICT_POSIX || _XPG8 */
int	sem_trywait(sem_t *);
int	sem_post(sem_t *);
int	sem_getvalue(sem_t *_RESTRICT_KYWD, int *_RESTRICT_KYWD);

#ifdef	__cplusplus
}
#endif

#endif	/* _SEMAPHORE_H */
