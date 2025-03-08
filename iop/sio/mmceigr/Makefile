IOP_SRC_DIR ?= ../mmceman/src/
IOP_INC_DIR ?= ../mmceman/include/

$(info -- MMCEIGR)
#Build single shot module to issue switch to bootcard on IGR

IOP_OBJS = mmceigr.o
IOP_CFLAGS += -DMMCEIGR

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release