
#include <dmacman.h>
#include <sio2regs.h>
#include <thbase.h>
#include <thevent.h>
#include <intrman.h>

#include "ioplib.h"
#include "irx_imports.h"

#include "mmce_sio2.h"
#include "sio2man_hook.h"

#include "module_debug.h"

#define EF_SIO2_INTR_COMPLETE	    0x00000200
#define EF_SIO2_TRANSFER_TIMEOUT    0x00000400

#define MAJOR 1
#define MINOR 1

IRX_ID("mmcesio2", MAJOR, MINOR);

//Port ctrl settings used for all transfers
static u32 mmce_sio2_port_ctrl1;
static u32 mmce_sio2_port_ctrl2;

static u32 sio2_save_ctrl;
static int event_flag = -1;

//SIO2MAN's intr handler
int (*sio2man_intr_handler_ptr)(void *arg);
void *sio2man_intr_arg_ptr;

//Replacement intr handler
int (*mmce_sio2_intr_handler_ptr)(void *arg) = NULL;
void *mmce_sio2_intr_arg_ptr = NULL;

iop_sys_clock_t timeout_alarm_200ms;
iop_sys_clock_t timeout_alarm_1s;
iop_sys_clock_t timeout_alarm_2s;

extern struct irx_export_table _exp_mmcesio2;

int mmce_sio2_intr_handler(void *arg)
{
	int ef = *(int *)arg;
	inl_sio2_stat_set(0x3);
	iSetEventFlag(ef, EF_SIO2_INTR_COMPLETE);
	return 1;
}

unsigned int mmce_sio2_timeout_handler(void *arg)
{
    int ef = *(int *)arg;
    iSetEventFlag(ef, EF_SIO2_TRANSFER_TIMEOUT);
    return 0;
}

void mmce_sio2_set_intr_handler()
{
    int state;
    sio2_save_ctrl = inl_sio2_ctrl_get();

    //Swap SIO2MAN's intr handler with ours
    CpuSuspendIntr(&state);
    mmce_sio2_intrman_internals_ptr->interrupt_handler_table[17].handler = mmce_sio2_intr_handler_ptr; 
    mmce_sio2_intrman_internals_ptr->interrupt_handler_table[17].userdata = mmce_sio2_intr_arg_ptr;
    CpuResumeIntr(state);

    //Copy port ctrl settings to SIO2 registers
    inl_sio2_portN_ctrl1_set(2, mmce_sio2_port_ctrl1);
    inl_sio2_portN_ctrl2_set(2, mmce_sio2_port_ctrl2);

    inl_sio2_portN_ctrl1_set(3, mmce_sio2_port_ctrl1);
    inl_sio2_portN_ctrl2_set(3, mmce_sio2_port_ctrl2);
}

void mmce_sio2_clear_intr_handler()
{
    int state;
    //Swap our intr handler with SIO2MAN's intr handler
    CpuSuspendIntr(&state);
    mmce_sio2_intrman_internals_ptr->interrupt_handler_table[17].handler = sio2man_intr_handler_ptr; 
    mmce_sio2_intrman_internals_ptr->interrupt_handler_table[17].userdata = sio2man_intr_arg_ptr;
    CpuResumeIntr(state);

    //Restore ctrl state, and reset STATE + FIFOS
    inl_sio2_ctrl_set(
        sio2_save_ctrl      |
        CTRL_RESET_STATE(1) |
        CTRL_RESET_FIFOS(1)
    );
}

void mmce_sio2_lock()
{
    //Lock sio2man driver so we can use it exclusively
    sio2man_hook_sio2_lock();

    //Set out intr handler
    mmce_sio2_set_intr_handler();
}

void mmce_sio2_unlock()
{
    //Restore original intr handler and ctrl state
    mmce_sio2_clear_intr_handler();

    //Unlock sio2man driver
    sio2man_hook_sio2_unlock();
}

int mmce_sio2_tx_rx_pio(u8 port, u8 tx_size, u8 rx_size, const u8 *tx_buf, u8 *rx_buf, u8 timeout)
{
    u32 resbits;
    u8 use_ack_timeout = 0;
    iop_sys_clock_t *timeout_value = NULL;

    if (timeout == TIMEOUT_USE_ACK_TIMEOUT)
        use_ack_timeout = 1;
    else if (timeout == TIMEOUT_ALARM_200MS)
        timeout_value = &timeout_alarm_200ms;
    else if (timeout == TIMEOUT_ALARM_1S)
        timeout_value = &timeout_alarm_1s;
    else if (timeout == TIMEOUT_ALARM_2S)
        timeout_value = &timeout_alarm_2s;

    //Reset SIO2 + FIFO pointers, enable interrupts
    inl_sio2_ctrl_set(
        CTRL_RESET_STATE(1)                         |
        CTRL_RESET_FIFOS(1)                         |
        CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
        CTRL_NO_MISSING_ACK_ERR(1)                  |
        CTRL_UNK07(1)                               |
        CTRL_ERROR_INTR_EN(1)                       |
        CTRL_TR_COMP_INTR_EN(1)
    );

    //Add transfer to queue
    inl_sio2_regN_set(0,
                        TR_CTRL_PORT_NR(port)       |
                        TR_CTRL_PAUSE(0)            |
                        TR_CTRL_TX_MODE_PIO_DMA(0)  |
                        TR_CTRL_RX_MODE_PIO_DMA(0)  |
                        TR_CTRL_NORMAL_TR(1)        |
                        TR_CTRL_SPECIAL_TR(0)       |
                        TR_CTRL_BAUD_DIV(0)         |
                        TR_CTRL_WAIT_ACK_FOREVER(0) |
                        TR_CTRL_TX_DATA_SZ(tx_size) |
                        TR_CTRL_RX_DATA_SZ(rx_size)
    );

    //Copy data to TX FIFO
    for (int i = 0; i < tx_size; i++) {
        inl_sio2_data_out(tx_buf[i]);
    }

    //Start transfer
    inl_sio2_ctrl_set(
        CTRL_TR_START(1)                            |
        CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
        CTRL_NO_MISSING_ACK_ERR(1)                  |
        CTRL_UNK07(1)                               |
        CTRL_ERROR_INTR_EN(1)                       |
        CTRL_TR_COMP_INTR_EN(1)
    );

    //Set timeout alarm
    if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
        SetAlarm(timeout_value, mmce_sio2_timeout_handler, &event_flag);

    //Wait for completion or timeout
    WaitEventFlag(event_flag, EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE, 1, &resbits);
    if (resbits & EF_SIO2_TRANSFER_TIMEOUT) {
        DPRINTF("Transfer timed out, resetting SIO2\n");
        //Reset SIO2
        inl_sio2_ctrl_set(
            CTRL_RESET_STATE(1)                         |
            CTRL_RESET_FIFOS(1)                         |
            CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
            CTRL_NO_MISSING_ACK_ERR(1)                  |
            CTRL_UNK07(1)                               |
            CTRL_ERROR_INTR_EN(1)                       |
            CTRL_TR_COMP_INTR_EN(1)
        );
        ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));
        return -1;
    }

    //Cancel timeout alarm
    if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
        CancelAlarm(mmce_sio2_timeout_handler,  &event_flag);
    
    //Clear flags
    ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));

    //Copy data out of RX FIFO
    for (int i = 0; i < rx_size; i++) {
        rx_buf[i] = inl_sio2_data_in();
    }

    return 0;
}

//Mixed DMA / PIO RX transfer
int mmce_sio2_rx(u8 port, u8 *buffer, u32 size, u8 timeout)
{
    u32 resbits;
    u8 use_ack_timeout = 0;
    iop_sys_clock_t *timeout_value = &timeout_alarm_2s;

    if (timeout == TIMEOUT_USE_ACK_TIMEOUT)
        use_ack_timeout = 1;
    else if (timeout == TIMEOUT_ALARM_200MS)
        timeout_value = &timeout_alarm_200ms;
    else if (timeout == TIMEOUT_ALARM_1S)
        timeout_value = &timeout_alarm_1s;
    else if (timeout == TIMEOUT_ALARM_2S)
        timeout_value = &timeout_alarm_2s;

    //dma element count (round down)
    u32 elements = size / 256;

    //remainder < 256
    u32 pio_size = size - (elements * 256);

    u32 bytes_done = 0;
    u32 elements_do = 0;

    u32 offset = 0;
    u32 offset_pio = 0;

    //used for all elements 256 bytes in size
    u32 dma_element = TR_CTRL_PORT_NR(port)           |
                      TR_CTRL_PAUSE(0)                |
                      TR_CTRL_TX_MODE_PIO_DMA(0)      |
                      TR_CTRL_RX_MODE_PIO_DMA(1)      |
                      TR_CTRL_NORMAL_TR(1)            |
                      TR_CTRL_SPECIAL_TR(0)           |
                      TR_CTRL_BAUD_DIV(0)             |
                      TR_CTRL_WAIT_ACK_FOREVER(0)     |
                      TR_CTRL_TX_DATA_SZ(0)           |
                      TR_CTRL_RX_DATA_SZ(256);

    //used for all elements less than 256 bytes in size
    u32 pio_element = TR_CTRL_PORT_NR(port)           |
                      TR_CTRL_PAUSE(0)                |
                      TR_CTRL_TX_MODE_PIO_DMA(0)      |
                      TR_CTRL_RX_MODE_PIO_DMA(0)      |
                      TR_CTRL_NORMAL_TR(1)            |
                      TR_CTRL_SPECIAL_TR(0)           |
                      TR_CTRL_BAUD_DIV(0)             |
                      TR_CTRL_WAIT_ACK_FOREVER(0)     |
                      TR_CTRL_TX_DATA_SZ(0)           |
                      TR_CTRL_RX_DATA_SZ(pio_size);

    while (bytes_done < size) {
        //Reset SIO2 + FIFO pointers, enable interrupts
        inl_sio2_ctrl_set(
            CTRL_RESET_STATE(1)                         |
            CTRL_RESET_FIFOS(1)                         |
            CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
            CTRL_NO_MISSING_ACK_ERR(1)                  |
            CTRL_UNK07(1)                               |
            CTRL_ERROR_INTR_EN(1)                       |
            CTRL_TR_COMP_INTR_EN(1)
        );

        //Used for DMA
        offset = bytes_done;
        elements_do = elements > 16 ? 16 : elements;

        //Copy DMA transfer elements to SIO2 registers
        for (int i = 0; i < elements_do; i++) {
            inl_sio2_regN_set(i, dma_element);
        }

        bytes_done += elements_do * 256;

        //Copy PIO element
        if (elements_do < 16) {
            if (pio_size != 0x0) {
                inl_sio2_regN_set(elements_do, pio_element);

                offset_pio = bytes_done;
                bytes_done += pio_size;
            }
        }

        //Start DMA transfer if needed
        if (elements_do != 0) {
            sceSetSliceDMA(IOP_DMAC_SIO2out, &buffer[offset], 0x100 >> 2, elements_do, DMAC_TO_MEM);
            sceStartDMA(IOP_DMAC_SIO2out);
        }

        //Start the transfer
        inl_sio2_ctrl_set(
            CTRL_TR_START(1)                            |
            CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
            CTRL_NO_MISSING_ACK_ERR(1)                  |
            CTRL_UNK07(1)                               |
            CTRL_ERROR_INTR_EN(1)                       |
            CTRL_TR_COMP_INTR_EN(1)
        );

        //Set timeout alarm
        if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
            SetAlarm(timeout_value, mmce_sio2_timeout_handler, &event_flag);

        //Wait for completion or timeout
        WaitEventFlag(event_flag, EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE, 1, &resbits);
        if (resbits & EF_SIO2_TRANSFER_TIMEOUT) {
            DPRINTF("Detected transfer timeout, attempting to reset SIO2\n");
            //Reset SIO2
            inl_sio2_ctrl_set(
                CTRL_RESET_STATE(1)                         |
                CTRL_RESET_FIFOS(1)                         |
                CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
                CTRL_NO_MISSING_ACK_ERR(1)                  |
                CTRL_UNK07(1)                               |
                CTRL_ERROR_INTR_EN(1)                       |
                CTRL_TR_COMP_INTR_EN(1)
            );
            ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));
            return -1;
        }

        //Cancel timeout alarm
        if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
            CancelAlarm(mmce_sio2_timeout_handler,  &event_flag);

        //Clear flags
        ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));

        elements -= elements_do;
    }

    //Copy data from RX FIFO
    if (pio_size != 0x0) {
        for (int i = 0; i < pio_size; i++) {
            buffer[offset_pio + i] = inl_sio2_data_in();
        }
    }

    return 0;
}

int mmce_sio2_tx(u8 port, const u8 *buffer, u32 size, u8 timeout)
{
    u32 resbits;
    u8 use_ack_timeout = 0;
    iop_sys_clock_t *timeout_value = NULL;

    if (timeout == TIMEOUT_USE_ACK_TIMEOUT)
        use_ack_timeout = 1;
    else if (timeout == TIMEOUT_ALARM_200MS)
        timeout_value = &timeout_alarm_200ms;
    else if (timeout == TIMEOUT_ALARM_1S)
        timeout_value = &timeout_alarm_1s;
    else if (timeout == TIMEOUT_ALARM_2S)
        timeout_value = &timeout_alarm_2s;

    //dma element count (round down)
    u32 elements = size / 256;

    //remainder < 256
    u32 pio_size = size - (elements * 256);

    u32 bytes_done = 0;
    u32 elements_do = 0;

    //used for all elements 256 bytes in size
    u32 dma_element = TR_CTRL_PORT_NR(port)           |
                      TR_CTRL_PAUSE(0)                |
                      TR_CTRL_TX_MODE_PIO_DMA(1)      |
                      TR_CTRL_RX_MODE_PIO_DMA(0)      |
                      TR_CTRL_NORMAL_TR(1)            |
                      TR_CTRL_SPECIAL_TR(0)           |
                      TR_CTRL_BAUD_DIV(0)             |
                      TR_CTRL_WAIT_ACK_FOREVER(0)     |
                      TR_CTRL_TX_DATA_SZ(256)         |
                      TR_CTRL_RX_DATA_SZ(0);

    //used for all elements less than 256 bytes in size
    u32 pio_element = TR_CTRL_PORT_NR(port)           |
                      TR_CTRL_PAUSE(0)                |
                      TR_CTRL_TX_MODE_PIO_DMA(0)      |
                      TR_CTRL_RX_MODE_PIO_DMA(0)      |
                      TR_CTRL_NORMAL_TR(1)            |
                      TR_CTRL_SPECIAL_TR(0)           |
                      TR_CTRL_BAUD_DIV(0)             |
                      TR_CTRL_WAIT_ACK_FOREVER(0)     |
                      TR_CTRL_TX_DATA_SZ(pio_size)    |
                      TR_CTRL_RX_DATA_SZ(0);

    while(bytes_done < size) {
        //Reset SIO2 + FIFO pointers, enable interrupts
        inl_sio2_ctrl_set(
            CTRL_RESET_STATE(1)                         |
            CTRL_RESET_FIFOS(1)                         |
            CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
            CTRL_NO_MISSING_ACK_ERR(1)                  |
            CTRL_UNK07(1)                               |
            CTRL_ERROR_INTR_EN(1)                       |
            CTRL_TR_COMP_INTR_EN(1)
        );

        elements_do = elements > 16 ? 16 : elements;

        //DMA elements
        if (elements_do != 0) {
            for (int i = 0; i < elements_do; i++) {
                inl_sio2_regN_set(i, dma_element);
            }

            sceSetSliceDMA(IOP_DMAC_SIO2in, (u8 *)&buffer[bytes_done], 0x100 >> 2, elements_do, DMAC_FROM_MEM);
            sceStartDMA(IOP_DMAC_SIO2in);

            bytes_done += elements_do * 256;
            elements -= elements_do;
        //PIO element
        } else if (elements_do == 0 && pio_size != 0) {
            inl_sio2_regN_set(0, pio_element);

            //Place data TX FIFO
            for (int i = 0; i < pio_size; i++) {
                inl_sio2_data_out(buffer[bytes_done + i]);
            }

            bytes_done += pio_size;
            pio_size = 0;
        }

        //Start the transfer
        inl_sio2_ctrl_set(
            CTRL_TR_START(1)                            |
            CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
            CTRL_NO_MISSING_ACK_ERR(1)                  |
            CTRL_UNK07(1)                               |
            CTRL_ERROR_INTR_EN(1)                       |
            CTRL_TR_COMP_INTR_EN(1)
        );

        //Set timeout alarm
        if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
            SetAlarm(timeout_value, mmce_sio2_timeout_handler, &event_flag);

        //Wait for completion or timeout
        WaitEventFlag(event_flag, EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE, 1, &resbits);
        if (resbits & EF_SIO2_TRANSFER_TIMEOUT) {
            DPRINTF("Detected transfer timeout, attempting to reset SIO2\n");
            //Reset SIO2
            inl_sio2_ctrl_set(
                CTRL_RESET_STATE(1)                         |
                CTRL_RESET_FIFOS(1)                         |
                CTRL_USE_ACK_WAIT_TIMEOUT(use_ack_timeout)  |
                CTRL_NO_MISSING_ACK_ERR(1)                  |
                CTRL_UNK07(1)                               |
                CTRL_ERROR_INTR_EN(1)                       |
                CTRL_TR_COMP_INTR_EN(1)
            );
            ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));
            return -1;
        }

        //Cancel timeout alarm
        if (timeout != TIMEOUT_NONE && timeout != TIMEOUT_USE_ACK_TIMEOUT)
            CancelAlarm(mmce_sio2_timeout_handler,  &event_flag);

        //Clear flags
        ClearEventFlag(event_flag, ~(EF_SIO2_TRANSFER_TIMEOUT | EF_SIO2_INTR_COMPLETE));
    }

    return bytes_done;
}

int _start(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int rv;
 
    DPRINTF("MMCESIO2 v%d.%d starting...\n", MAJOR, MINOR);

    //Install necessary hooks
    rv = sio2man_hook_init();
    if (rv == -1) {
        DPRINTF("Failed to init sio2man hook\n");
        return MODULE_NO_RESIDENT_END;
    }

    //Create event flag for intr
    iop_event_t event;
    event.attr = 2;
    event.option = 0;
    event.bits = 0;

    event_flag = CreateEventFlag(&event);
    if (event_flag < 0) {
        DPRINTF("Failed to create event flag\n");
        sio2man_hook_deinit();
        return MODULE_NO_RESIDENT_END;
    }

    //Assign intr pointers
    mmce_sio2_intr_handler_ptr = &mmce_sio2_intr_handler;
    mmce_sio2_intr_arg_ptr = &event_flag;

    //Ensure IOP DMA channels are enabled
    sceSetDMAPriority(IOP_DMAC_SIO2in, 3);
    sceSetDMAPriority(IOP_DMAC_SIO2out, 3);
    sceEnableDMAChannel(IOP_DMAC_SIO2in);
    sceEnableDMAChannel(IOP_DMAC_SIO2out);

    //Ensure SIO2 intrs are enabled
    EnableIntr(IOP_IRQ_SIO2);

    //200ms
    timeout_alarm_200ms.hi = 0x0;
    timeout_alarm_200ms.lo = 0x708000;

    //1s
    timeout_alarm_1s.hi = 0x0;
    timeout_alarm_1s.lo = 0x2328000;

    //2s
    timeout_alarm_2s.hi = 0x0;
    timeout_alarm_2s.lo = 0x4650000;

    mmce_sio2_port_ctrl1 =
        PCTRL0_ATT_LOW_PER(0x5)      |
        PCTRL0_ATT_MIN_HIGH_PER(0x0) |
        PCTRL0_BAUD0_DIV(0x2)        |
        PCTRL0_BAUD1_DIV(0xff);

    mmce_sio2_port_ctrl2 =
        PCTRL1_ACK_TIMEOUT_AFTER(0x3bf)         | //~40us timeout w/ 24MHz baud
        PCTRL1_WAIT_CYCLES_AFTER_ACK_LOW(0x0)   |
        PCTRL1_UNK24(0x0)                       |
        PCTRL1_IF_MODE_SPI_DIFF(0x0);

    RegisterLibraryEntries(&_exp_mmcesio2);

    DPRINTF("MMCESIO2 v%d.%d started\n", MAJOR, MINOR);

    return MODULE_RESIDENT_END;
}

int _stop(int argc, char *argv[])
{
    DPRINTF("Unloading module\n");
    ReleaseLibraryEntries(&_exp_mmcesio2);
    sio2man_hook_deinit();
    DeleteEventFlag(event_flag);

    return MODULE_NO_RESIDENT_END;
}
