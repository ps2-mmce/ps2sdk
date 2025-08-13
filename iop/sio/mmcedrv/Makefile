IOP_SRC_DIR ?= src/
IOP_INC_DIR ?= include/

IOP_INC_DIR += -I../mmceman/include/
IOP_INC_DIR += -I../mmcesio2/include/

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
