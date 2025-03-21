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
# Copyright 2015 Toomas Soome <tsoome@me.com>
#

include $(SRC)/Makefile.master
include $(SRC)/boot/Makefile.version
include $(SRC)/boot/Makefile.inc

PROG= isoboot
FILEMODE=0444

BOOT_COMCONSOLE_PORT= 0x3f8
BOOT_COMCONSOLE_SPEED= 9600
B2SIOFMT= 0x3

ORG1= 0x7c00
ORG2= 0x0

ISOBOOTSIZE= 30720

CPPFLAGS += -DBOOTPROG=\"isoboot\" \
	-DSIOPRT=$(BOOT_COMCONSOLE_PORT) \
	-DSIOFMT=$(B2SIOFMT) \
	-DSIOSPD=$(BOOT_COMCONSOLE_SPEED) \
	-I../../include \
	-I../../libsa \
	-I. \
	-I../../sys \
	-I../common \
	-I../btx/lib \
	-I../../common \
	-I../gptzfsboot
AS_CPPFLAGS += $(CPPFLAGS)

LDSCRIPT=	../boot.ldscript
LD_FLAGS=	-static -N --gc-sections
LIBSTAND=	../../libsa/$(MACH)/libsa.a

gptldr.out := LD_FLAGS += $(GLDTARGET)

isoboot.o := SMOFF += unreachable
cons.o := CFLAGS += -_gcc14=--param=min-pagesize=0
isoboot.o := CFLAGS += -_gcc14=--param=min-pagesize=0

include ../Makefile.inc

all: $(PROG)

install: all $(ROOTBOOTPROG)

OBJS= mb_header.o isoboot.o sio.o drv.o cons.o gptldr.o

CLEANFILES += isoboot

isoboot: gptldr.bin isoboot.bin $(BTXKERN)
	$(BTXLD) -v -E $(ORG2) -f bin -b $(BTXKERN) -V $(BOOT_VERSION) -l \
		gptldr.bin -o $@ isoboot.bin
	@set -- `ls -l $@`; x=$$(($(ISOBOOTSIZE)-$$5)); \
	    echo "$$x bytes available"; test $$x -ge 0

CLEANFILES += gptldr.bin gptldr.out gptldr.o

gptldr.bin: gptldr.out
	$(OBJCOPY) -S -O binary gptldr.out $@

gptldr.out:	gptldr.o
	$(GLD) $(LD_FLAGS) -e start -Ttext $(ORG1) -o $@ gptldr.o

CLEANFILES += isoboot.bin isoboot.out $(OBJS)

isoboot.bin: isoboot.out
	$(OBJCOPY) -S -O binary isoboot.out $@

isoboot.out: $(BTXCRT) $(OBJS)
	$(GLD) $(LD_FLAGS) -T $(LDSCRIPT) -o $@ $(BTXCRT) $(OBJS) $(LIBSTAND)

machine:
	$(RM) machine
	$(SYMLINK) ../../sys/i386/include machine

x86:
	$(RM) x86
	$(SYMLINK) ../../sys/x86/include x86

$(OBJS): machine x86

%.o:	../gptzfsboot/%.S
	$(COMPILE.S) $<

%.o:	../../common/%.S
	$(COMPILE.S) $<

%.o:	../common/%.c
	$(COMPILE.c) $<

%.o:	../../common/%.c
	$(COMPILE.c) $<

clobber: clean

clean:
	$(RM) $(CLEANFILES) machine x86
