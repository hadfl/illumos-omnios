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
# Copyright 2023 Oxide Computer Company
#

UTSBASE	= ../..

MODULE		= stream_ktest
OBJECTS		= $(STREAM_KTEST_OBJS:%=$(OBJS_DIR)/%)
ROOTMODULE	= $(USR_KTEST_DIR)/$(MODULE)

include $(UTSBASE)/intel/Makefile.intel

ALL_TARGET	= $(BINARY)
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

LDFLAGS		+= -N drv/ktest

.KEEP_STATE:

def:           $(DEF_DEPS)

all:           $(ALL_DEPS)

clean:         $(CLEAN_DEPS)

clobber:       $(CLOBBER_DEPS)

install:       $(INSTALL_DEPS)

include $(UTSBASE)/intel/Makefile.targ
