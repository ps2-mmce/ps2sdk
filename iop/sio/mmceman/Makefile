# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

IOP_SRC_DIR ?= src/
IOP_INC_DIR ?= include/

IOP_INC_DIR += -I../mmceman/include/

#Build full MMCEMAN for use with loader / homebrew
IOP_BIN = irx/mmceman.irx
IOP_OBJS = imports.o ioplib.o mmce_cmds.o mmceman.o mmce_fs.o

#Needed for iomanX
IOP_CFLAGS += -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_OLD_NAME_ADDDELDRV=0
IOP_IMPORT_INCS += \
	system/dmacman \
	system/intrman \
	system/loadcore \
	system/sio2man \
	system/stdio \
	system/sysclib \
	system/threadman \
	system/iomanX \
	sio/mmcesio2

ifeq ($(DEBUG), 1)
  $(info -- MMCEMAN Debug)
  IOP_CFLAGS += -DDEBUG
else
  $(info -- MMCEMAN)
endif

include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
