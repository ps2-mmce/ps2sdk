# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

IOP_SRC_DIR ?= src/
IOP_INC_DIR ?= include/

IOP_IMPORT_INCS += \
	sio/mmcesio2 \
	sio/mmceman \
	system/dmacman \
	system/iomanx \
	system/intrman \
	system/loadcore \
	system/sio2man \
	system/stdio \
	system/sysclib \
	system/threadman

#Build minimal MMCEDRV for use in-game
IOP_BIN = irx/mmcedrv.irx
IOP_OBJS = imports.o ioplib.o exports.o mmcedrv.o

ifeq ($(DEBUG), 1)
  $(info -- MMCEDRV Debug)
  IOP_CFLAGS += -DDEBUG
else
  $(info -- MMCEDRV)
endif

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
