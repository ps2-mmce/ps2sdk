#include <intrman.h>
#include <stdio.h>
#include <string.h>
#include <sysclib.h>

#include "ioplib.h"
#include "irx_imports.h"

#include "mmce_cmds.h"
#include "mmce_sio2.h"

#include "module_debug.h"

#define MAJOR 1
#define MINOR 1

IRX_ID("mmcedrv", MAJOR, MINOR);

extern struct irx_export_table _exp_mmcedrv;

#define MMCEDRV_SETTING_PORT 0x0

static u8 mmce_port;

int mmcedrv_read_sector(int fd, u32 sector, u32 count, void *buffer)
{
    int res;
    int sectors_read = 0;

    u8 wrbuf[0xB];
    u8 rdbuf[0xB];

    DPRINTF("%s fd: %i, starting sector: %i, count: %i\n", __func__, fd, sector, count);

    wrbuf[0x0] = MMCE_ID;                   //Identifier
    wrbuf[0x1] = MMCE_CMD_FS_READ_SECTOR;   //Command
    wrbuf[0x2] = MMCE_RESERVED;             //Reserved
    wrbuf[0x3] = fd;                        //File Descriptor

    wrbuf[0x4] = (sector & 0x00FF0000) >> 16;
    wrbuf[0x5] = (sector & 0x0000FF00) >> 8;
    wrbuf[0x6] = (sector & 0x000000FF);

    wrbuf[0x7] = (count & 0x00FF0000) >> 16;
    wrbuf[0x8] = (count & 0x0000FF00) >> 8;
    wrbuf[0x9] = (count & 0x000000FF);

    wrbuf[0xA] = 0xff;

    mmce_sio2_lock();

    //Packet #1: Command, file descriptor, and size
    res = mmce_sio2_tx_rx_pio(mmce_port, 0xB, 0xB, wrbuf, rdbuf, TIMEOUT_ALARM_2S);
    if (res == -1) {
        DPRINTF("%s ERROR: P1 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x1] != MMCE_REPLY_CONST) {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], MMCE_REPLY_CONST);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x9] != 0x0) {
        DPRINTF("%s ERROR: P1 - Got bad return value from card, res %i\n", __func__, rdbuf[0x9]);
        mmce_sio2_unlock();
        return -1;
    }

    //Packet #2 - n: Read data
    res = mmce_sio2_rx(mmce_port, buffer, (count * 2048), TIMEOUT_ALARM_2S);
    if (res == -1) {
        DPRINTF("%s ERROR: P2 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    //Packet #n + 1: Sectors read
    res = mmce_sio2_tx_rx_pio(mmce_port, 0x0, 0x5, wrbuf, rdbuf, TIMEOUT_ALARM_1S);
    if (res == -1) {
        DPRINTF("%s ERROR: P3 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    mmce_sio2_unlock();

    sectors_read  = rdbuf[0x1] << 16;
    sectors_read |= rdbuf[0x2] << 8;
    sectors_read |= rdbuf[0x3];

    if (sectors_read != count) {
        DPRINTF("%s ERROR: Sectors read: %i, expected: %i\n", __func__, sectors_read, count);
    }

    return sectors_read;
}

int mmcedrv_read(int fd, int size, void *ptr)
{
    int res;
    int bytes_read;

    DPRINTF("%s: fd: %i, size: %i\n", __func__, fd, size);

    u8 wrbuf[0xA];
    u8 rdbuf[0xA];

    wrbuf[0x0] = MMCE_ID;                   //Identifier
    wrbuf[0x1] = MMCE_CMD_FS_READ;          //Command
    wrbuf[0x2] = MMCE_RESERVED;             //Reserved
    wrbuf[0x3] = 0x0;                       //Transfer mode (unused)
    wrbuf[0x4] = fd;                        //File Descriptor 
    wrbuf[0x5] = (size & 0xFF000000) >> 24; //Size
    wrbuf[0x6] = (size & 0x00FF0000) >> 16;
    wrbuf[0x7] = (size & 0x0000FF00) >> 8;
    wrbuf[0x8] = (size & 0x000000FF);
    wrbuf[0x9] = 0xff;

    mmce_sio2_lock();

    //Packet #1: Command, file descriptor, and size
    res = mmce_sio2_tx_rx_pio(mmce_port, 0xA, 0xA, wrbuf, rdbuf, TIMEOUT_ALARM_2S);
    if (res == -1) {
        DPRINTF("%s ERROR: P1 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x1] != MMCE_REPLY_CONST) {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], MMCE_REPLY_CONST);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x9] != 0x0) {
        DPRINTF("%s ERROR: P1 - Got bad return value from card, res %i\n", __func__, rdbuf[0x9]);
        mmce_sio2_unlock();
        return -1;
    }

    //Packet #2 - n: Raw read data
    res = mmce_sio2_rx(mmce_port, ptr, size, TIMEOUT_ALARM_2S);
    if (res == -1) {
        DPRINTF("%s ERROR: P2 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    //Packet #n + 1: Bytes read
    res = mmce_sio2_tx_rx_pio(mmce_port, 0x0, 0x6, wrbuf, rdbuf, TIMEOUT_ALARM_1S);
    if (res == -1) {
        DPRINTF("%s ERROR: P3 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    mmce_sio2_unlock();
    
    bytes_read  = rdbuf[0x1] << 24;
    bytes_read |= rdbuf[0x2] << 16;
    bytes_read |= rdbuf[0x3] << 8;
    bytes_read |= rdbuf[0x4];

    if (bytes_read != size) {
        DPRINTF("%s WARN: bytes read: %i, expected: %i\n", __func__, bytes_read, size);

        if (bytes_read > size)
            bytes_read = size;

    }

    return bytes_read;
}

int mmcedrv_write(int fd, int size, u8 *ptr)
{
    int res;
    int bytes_written;

    u32 bytes_done = 0;
    u32 transfer_size = 0;

    DPRINTF("%s: fd: %i, size: %i\n", __func__, fd, size);

    u8 wrbuf[0xA];
    u8 rdbuf[0xA];

    wrbuf[0x0] = MMCE_ID;                   //Identifier
    wrbuf[0x1] = MMCE_CMD_FS_WRITE;         //Command
    wrbuf[0x2] = MMCE_RESERVED;             //Reserved
    wrbuf[0x3] = 0x0;                       //Transfer mode (unimplemented)
    wrbuf[0x4] = fd;                        //File Descriptor 
    wrbuf[0x5] = (size & 0xFF000000) >> 24;
    wrbuf[0x6] = (size & 0x00FF0000) >> 16;
    wrbuf[0x7] = (size & 0x0000FF00) >> 8;
    wrbuf[0x8] = (size & 0x000000FF);
    wrbuf[0x9] = 0xff;

    mmce_sio2_lock();

    //Packet #1: Command, file descriptor, and size
    res = mmce_sio2_tx_rx_pio(mmce_port, 0xA, 0xA, wrbuf, rdbuf, TIMEOUT_ALARM_1S);
    if (res == -1) {
        DPRINTF("%s ERROR: P1 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x1] != MMCE_REPLY_CONST) {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], MMCE_REPLY_CONST);
        mmce_sio2_unlock();
        return -1;
    }

    if (rdbuf[0x9] != 0x0) {
        DPRINTF("%s ERROR: P1 - Got bad return value from card 0x%x\n", __func__, rdbuf[0x9]);
        mmce_sio2_unlock();
        return -1;
    }

    while (1) {
        //Waiting stage
        res = mmce_sio2_tx_rx_pio(mmce_port, 0, 2, NULL, rdbuf, TIMEOUT_ALARM_2S);
        if (res == -1) {
            DPRINTF("%s ERROR: Timed out waiting for ready\n", __func__);
            mmce_sio2_unlock();
            return -1;
        }

        if (bytes_done >= size) {
            DPRINTF("transfer loop finished\n");
            break;
        }

        /* mmce_sio2_tx can send an unlimited amount of bytes 
         * at a time but MMCE(s) only have a 4KB write buffer and 
         * move to a waiting state when full, this is to give the MMCE 
         * time to flush the data to the sdcard */
        transfer_size = size - bytes_done;
        if (transfer_size > 4096)
            transfer_size = 4096;

        //Transfer stage
        res = mmce_sio2_tx(mmce_port, &ptr[bytes_done], transfer_size, TIMEOUT_ALARM_2S);
        if (res == -1) {
            DPRINTF("%s ERROR: Timed out during transfer\n", __func__);
            mmce_sio2_unlock();
            return -1;
        }

        bytes_done += res;
    }

    //Packets #n + 1: Bytes written
    res = mmce_sio2_tx_rx_pio(mmce_port, 0x0, 0x6, wrbuf, rdbuf, TIMEOUT_ALARM_1S);
    if (res == -1) {
        DPRINTF("%s ERROR: P3 - Timedout waiting for /ACK\n", __func__);
        mmce_sio2_unlock();
        return -1;
    }

    mmce_sio2_unlock();

    bytes_written  = rdbuf[0x1] << 24;    
    bytes_written |= rdbuf[0x2] << 16;   
    bytes_written |= rdbuf[0x3] << 8;   
    bytes_written |= rdbuf[0x4];

    if (bytes_written != size) {
        DPRINTF("%s bytes written: %i, expected: %i\n", __func__, bytes_written, size);
    }

    return bytes_written;
}

s64 mmcedrv_lseek64(int fd, s64 offset, int whence)
{
    int res;
    s64 position = -1;

    DPRINTF("%s: fd: %i, offset: %i, whence: %i\n", __func__, fd, offset, whence);

    u8 wrbuf[0xd];
    u8 rdbuf[0x16];

    wrbuf[0x0] = MMCE_ID;                       //Identifier
    wrbuf[0x1] = MMCE_CMD_FS_LSEEK64;           //Command
    wrbuf[0x2] = MMCE_RESERVED;                 //Reserved
    wrbuf[0x3] = fd;                            //File descriptor

    wrbuf[0x4] = (offset & 0xFF00000000000000) >> 56;   //Offset
    wrbuf[0x5] = (offset & 0x00FF000000000000) >> 48;
    wrbuf[0x6] = (offset & 0x0000FF0000000000) >> 40;
    wrbuf[0x7] = (offset & 0x000000FF00000000) >> 32;
    wrbuf[0x8] = (offset & 0x00000000FF000000) >> 24;
    wrbuf[0x9] = (offset & 0x0000000000FF0000) >> 16;
    wrbuf[0xa] = (offset & 0x000000000000FF00) >> 8;
    wrbuf[0xb] = (offset & 0x00000000000000FF);
    wrbuf[0xc] = (u8)(whence);  //Whence

    //Packet #1: Command, file descriptor, offset, and whence
    mmce_sio2_lock();
    res = mmce_sio2_tx_rx_pio(mmce_port, 0xd, 0x16, wrbuf, rdbuf, TIMEOUT_ALARM_1S);
    mmce_sio2_unlock();

    if (res == -1) {
        DPRINTF("%s ERROR: P1 - Timedout waiting for /ACK\n", __func__);
        return -1;
    }

    if (rdbuf[0x1] != MMCE_REPLY_CONST) {
        DPRINTF("%s ERROR: Invalid response from card. Got 0x%x, Expected 0x%x\n", __func__, rdbuf[0x1], MMCE_REPLY_CONST);
        return -1;
    }

    position  = (s64)rdbuf[0xd] << 56;
    position |= (s64)rdbuf[0xe] << 48;
    position |= (s64)rdbuf[0xf] << 40;
    position |= (s64)rdbuf[0x10] << 32;
    position |= (s64)rdbuf[0x11] << 24;
    position |= (s64)rdbuf[0x12] << 16;
    position |= (s64)rdbuf[0x13] << 8;
    position |= (s64)rdbuf[0x14];

    return position;
}

//For OPL, called through CDVDMAN Device
void mmcedrv_config_set(int setting, int value)
{
    switch (setting) {
        case MMCEDRV_SETTING_PORT:
            if (value == 2 || value == 3)
                mmce_port = value;
            else
                DPRINTF("Invalid port setting: %i\n", value);
        break;

        default:
        break;
    }
}

int __start(int argc, char *argv[])
{
    DPRINTF("Multipurpose Memory Card Emulator Driver (MMCEDRV) v%d.%d by the MMCE team\n", MAJOR, MINOR);

    //Check for MMCESIO2
    iop_library_t * lib = ioplib_getByName("mmcesio2");
    if (lib == NULL) {
        DPRINTF("MMCESIO2 not loaded, aborting. Please load MMCESIO2 first.\n");
        return MODULE_NO_RESIDENT_END;
    }

    //Register exports
    if (RegisterLibraryEntries(&_exp_mmcedrv) != 0) {
        DPRINTF("ERROR: library already registered\n");
        return MODULE_NO_RESIDENT_END;
    }

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

    return MODULE_NO_RESIDENT_END;
}

int _start(int argc, char *argv[])
{
    if (argc >= 0) 
        return __start(argc, argv);
    else
        return __stop(-argc, argv);
}
