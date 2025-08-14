IOP_SRC_DIR ?= src/
IOP_INC_DIR ?= include/

IOP_BIN = irx/mmcesio2.irx
IOP_OBJS = exports.o imports.o ioplib.o mmce_sio2.o sio2man_hook.o

IOP_IMPORT_INCS += \
	sio/mmcesio2 \
	system/dmacman \
	system/iomanx \
	system/intrman \
	system/loadcore \
	system/sio2man \
	system/stdio \
	system/sysclib \
	system/threadman


ifeq ($(DEBUG), 1)
  $(info -- MMCESIO2 Debug)
  IOP_CFLAGS += -DDEBUG
else
  $(info -- MMCESIO2)
endif


include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release