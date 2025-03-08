$(info -- MMCEMAN)
#Build full MMCEMAN for use with loader / homebrew
IOP_OBJS = ioplib.o sio2man_hook.o mmce_sio2.o mmce_cmds.o mmce_fs.o imports.o mmceman.o
#Needed for iomanX
IOP_CFLAGS += -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_OLD_NAME_ADDDELDRV=0
IOP_INCS += \
	-I$(PS2SDKSRC)/iop/system/intrman/include \
	-I$(PS2SDKSRC)/iop/system/loadcore/include \
	-I$(PS2SDKSRC)/iop/system/sio2man/include \
	-I$(PS2SDKSRC)/iop/system/stdio/include \
	-I$(PS2SDKSRC)/iop/system/sysclib/include \
	-I$(PS2SDKSRC)/iop/system/threadman/include

ifeq ($(DEBUG), 1)
  $(info -- MMCEMAN debug enabled)
  IOP_CFLAGS += -DDEBUG
  IOP_CFLAGS += -DDEBUG=1
endif

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
