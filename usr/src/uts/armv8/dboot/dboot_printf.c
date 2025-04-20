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
 * Copyright (c) 2012 Gary Mills
 * Copyright 2020 Joyent, Inc.
 * Copyright 2025 Michael van der Westhuizen
 *
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/machparam.h>
#include <sys/archsystm.h>
#include <sys/boot_console.h>
#include <sys/varargs.h>
#include <sys/stdbool.h>
#include "dboot_printf.h"

/*
 * This file provides simple output formatting via dboot_printf()
 */

static void dboot_vprintf(const char *fmt, va_list args);

static char digits[] = "0123456789abcdef";

/*
 * Primitive version of panic, prints a message then resets the system
 */
void
dboot_panic(const char *fmt, ...)
{
	extern void __NORETURN _reset(bool poff);
	va_list	args;

	va_start(args, fmt);
	dboot_vprintf(fmt, args);
	va_end(args);

	if (boot_console_type(NULL) == CONS_SCREEN_TEXT ||
	    boot_console_type(NULL) == CONS_FRAMEBUFFER) {
		dboot_printf("Press any key to reboot\n");
		(void) bcons_getchar();
	}

	_reset(false);
}

void
panic(const char *fmt, ...)
    __attribute__((alias("dboot_panic"), noreturn));
void
prom_panic(const char *fmt, ...)
    __attribute__((alias("dboot_panic"), noreturn));

/*
 * printf for boot code
 */
void
dboot_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	dboot_vprintf(fmt, args);
	va_end(args);
}

void prom_printf(const char *fmt, ...) __attribute__((alias("dboot_printf")));
void printf(const char *fmt, ...) __attribute__((alias("dboot_printf")));

/*
 * output a string
 */
static void
dboot_puts(const char *s)
{
	while (*s != 0) {
		bcons_putchar(*s);
		++s;
	}
}

void
prom_writestr(const char *buf, size_t len)
{
	size_t written = 0;

	while (written < len)  {
		bcons_putchar(buf[written]);
		++written;
	}
}

static void
dboot_putnum(uintmax_t x, boolean_t is_signed, uint8_t base)
{
	char buffer[64];	/* digits in reverse order */
	int i;

	if (is_signed && (intmax_t)x < 0) {
		bcons_putchar('-');
		x = -x;
	}

	for (i  = -1; x != 0 && i <= 63; x /= base)
		buffer[++i] = digits[x - ((x / base) * base)];

	if (i < 0)
		buffer[++i] = '0';

	while (i >= 0)
		bcons_putchar(buffer[i--]);
}

/*
 * Very primitive printf - only does a subset of the standard format characters.
 */
static void
dboot_vprintf(const char *fmt, va_list args)
{
	char *s;
	uintmax_t x;
	uint8_t base;
	uint8_t size;

	if (fmt == NULL) {
		dboot_puts("dboot_printf(): 1st arg is NULL\n");
		return;
	}
	for (; *fmt; ++fmt) {
		if (*fmt != '%') {
			bcons_putchar(*fmt);
			continue;
		}

		size = 0;
again:
		++fmt;
		switch (*fmt) {

		case '%':
			bcons_putchar(*fmt);
			break;

		case 'c':
			x = va_arg(args, int);
			bcons_putchar(x);
			break;

		case 'j':
			size = sizeof (uintmax_t);
			goto again;

		case 's':
			s = va_arg(args, char *);
			if (s == NULL)
				dboot_puts("*NULL*");
			else
				dboot_puts(s);
			break;

		case 'p':
			x = va_arg(args, ulong_t);
			dboot_putnum(x, B_FALSE, 16);
			break;

		case 'l':
			if (size == 0)
				size = sizeof (long);
			else if (size == sizeof (long))
				size = sizeof (long long);
			goto again;

		case 'd':
			if (size == 0)
				x = va_arg(args, int);
			else if (size == sizeof (long))
				x = va_arg(args, long);
			else
				x = va_arg(args, long long);
			dboot_putnum(x, B_TRUE, 10);
			break;

		case 'u':
			base = 10;
			goto unsigned_num;

		case 'b':
			base = 2;
			goto unsigned_num;

		case 'o':
			base = 8;
			goto unsigned_num;

		case 'x':
			base = 16;
unsigned_num:
			if (size == 0)
				x = va_arg(args, uint_t);
			else if (size == sizeof (ulong_t))
				x = va_arg(args, ulong_t);
			else
				x = va_arg(args, unsigned long long);
			dboot_putnum(x, B_FALSE, base);
			break;

		case 'z':
			size = sizeof (size_t);
			goto again;

		default:
			dboot_puts("dboot_printf(): unknown % escape\n");
		}
	}
}

void
prom_vprintf(const char *fmt, va_list adx)
    __attribute__((alias("dboot_vprintf")));
