$(info -- MMCEMAN)
#Build full MMCEMAN for use with loader / homebrew
IOP_OBJS = ioplib.o sio2man_hook.o mmce_sio2.o mmce_cmds.o mmce_fs.o imports.o mmceman.o
#Needed for iomanX
IOP_CFLAGS += -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_OLD_NAME_ADDDELDRV=0

ifeq ($(DEBUG), 1)
  $(info -- MMCEMAN debug enabled)
  IOP_CFLAGS += -DDEBUG
  IOP_CFLAGS += -DDEBUG=1
endif

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
