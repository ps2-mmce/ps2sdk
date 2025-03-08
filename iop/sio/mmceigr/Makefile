IOP_SRC_DIR ?= ../mmceman/src/
IOP_INC_DIR ?= ../mmceman/include/

$(info -- MMCEIGR)
#Build single shot module to issue switch to bootcard on IGR
IOP_BIN = irx/mmceigr.irx
IOP_OBJS = mmceigr.o
IOP_CFLAGS += -DMMCEIGR
all: $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS) $(IOP_BIN)

#include $(PS2SDK)/Defs.make
#include $(PS2SDK)/samples/Makefile.pref
#include $(PS2SDK)/samples/Makefile.iopglobal

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release