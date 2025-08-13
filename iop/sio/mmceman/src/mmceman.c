#include <intrman.h>
#include <stdio.h>
#include <string.h>
#include <sysclib.h>

#include "ioplib.h"
#include "irx_imports.h"

#include "mmce_cmds.h"
#include "mmce_fs.h"
#include "mmce_sio2.h"
#include "mmceman.h"

#include "module_debug.h"

#define MAJOR 2
#define MINOR 1

const char *mmce_product_ids[] = {"Unknown", "SD2PSX", "MemCard PRO2", "PicoMemcard+", "PicoMemcardZero"};

IRX_ID("mmceman", MAJOR, MINOR);

u8 mmce_port;

static void mmce_init(int port)
{
    int res = -1;
    int id = 0;

    mmce_port = port;

    for (int i = 0; i < 6; i++) {
        res = mmce_cmd_ping_quick();
        if (res != -1) {
            DPRINTF("Found card in port %i\n", port);

            id = (res & 0xFF00) >> 8;
            if (id < 5) {
                DPRINTF("Product ID: %i (%s)\n", id, mmce_product_ids[id]);
            } else {
                DPRINTF("Unknown Product ID: %i\n", id);
            }

            DPRINTF("Revision ID: %i\n", (res & 0xFF));
            DPRINTF("Protocol Version: %i\n", (res & 0xFF0000) >> 16);

            DPRINTF("Resetting MMCE FS...");
            res = mmce_cmd_reset();
            if (res != 0) 
                DPRINTF("Failed: %i\n", res);
            else
                DPRINTF("Okay\n");

            break;
        }
    }
}

int __start(int argc, char *argv[])
{
    printf("Multipurpose Memory Card Emulator Manager (MMCEMAN) v%d.%d by the MMCE team\n", MAJOR, MINOR);

    //Check for MMCESIO2
    iop_library_t * lib = ioplib_getByName("mmcesio2");
    if (lib == NULL) {
        DPRINTF("MMCESIO2 not loaded, aborting. Please load MMCESIO2 first.\n");
        return MODULE_NO_RESIDENT_END;
    }

    //Try to init MMCE's on both ports
    mmce_init(2);
    mmce_init(3);

    //Attach filesystem to iomanX
    mmce_fs_register();

    lib = ioplib_getByName("modload");
    if (lib != NULL) {
        DPRINTF("modload 0x%x detected\n", lib->version);
        if (lib->version > 0x102)           // IOP is running a MODLOAD version which supports unloading IRX Modules
            return MODULE_REMOVABLE_END;    // and we do support getting unloaded...
    } else {
        DPRINTF("modload not detected! this is serious!\n");
    }

    return MODULE_RESIDENT_END;
}

int __stop(int argc, char *argv[])
{
    DPRINTF("Unloading module\n");
    mmce_fs_unregister();

    return MODULE_NO_RESIDENT_END;
}

int _start(int argc, char *argv[])
{
    if (argc >= 0) 
        return __start(argc, argv);
    else
        return __stop(-argc, argv);
}
