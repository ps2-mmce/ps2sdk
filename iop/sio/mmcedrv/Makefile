IOP_SRC_DIR ?= ../mmceman/src/
IOP_INC_DIR ?= ../mmceman/include/

$(info -- MMCEDRV)
#Build minimal MMCE driver for use in-game
IOP_BIN = irx/mmcedrv.irx
IOP_OBJS = ioplib.o sio2man_hook.o mmce_sio2.o exports.o imports.o mmcedrv.o
IOP_CFLAGS += -DMMCEDRV

ifeq ($(DEBUG), 1)
  $(info -- MMCEMAN debug enabled)
  IOP_CFLAGS += -DDEBUG
  IOP_CFLAGS += -DDEBUG=1
endif

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
