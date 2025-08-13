#ifndef MMCE_SIO2_H
#define MMCE_SIO2_H

#include <tamtypes.h>
#include <thbase.h>

#define TIMEOUT_NONE 0            //No alarms, no SIO2 ACK timeout, if a transfer freezes, everything's freezing
#define TIMEOUT_USE_ACK_TIMEOUT 1 //Use SIO2 ACK timeout value set by PCTRL1_ACK_TIMEOUT_AFTER
#define TIMEOUT_ALARM_200MS 2     //Use 200MS IOP alarm
#define TIMEOUT_ALARM_1S 3        //Use 1S IOP alarm
#define TIMEOUT_ALARM_2S 4        //Use 2S IOP alarm

extern u8 mmce_sio2_use_alarm;

//Replace SIO2MAN intr handler and clear CTRL reg (called within mmce_sio2_lock)
extern void mmce_sio2_set_intr_handler();

//Restore SIO2MAN intr handler and restore CTRL reg (called within mmce_sio2_unlock)
extern void mmce_sio2_clear_intr_handler();

//Lock SIO2 for exclusive access
extern void mmce_sio2_lock();

//Unlock SIO2
extern void mmce_sio2_unlock();

//RX TX PIO single transfer (1-256 bytes)
extern int mmce_sio2_tx_rx_pio(u8 port, u8 tx_size, u8 rx_size, const u8 *tx_buf, u8 *rx_buf, u8 timeout);

//RX DMA n * 256, PIO remainder
extern int mmce_sio2_rx(u8 port, u8 *buffer, u32 size, u8 timeout);

//TX DMA n * 256, PIO remainder
extern int mmce_sio2_tx(u8 port, const u8 *buffer, u32 size, u8 timeout);

#define mmcesio2_IMPORTS_start DECLARE_IMPORT_TABLE(mmcesio2, 1, 1)
#define mmcesio2_IMPORTS_end   END_IMPORT_TABLE
#define I_mmce_sio2_set_intr_handler    DECLARE_IMPORT(4, mmce_sio2_set_intr_handler)
#define I_mmce_sio2_clear_intr_handler  DECLARE_IMPORT(5, mmce_sio2_clear_intr_handler)
#define I_mmce_sio2_lock                DECLARE_IMPORT(6, mmce_sio2_lock)
#define I_mmce_sio2_unlock              DECLARE_IMPORT(7, mmce_sio2_unlock)
#define I_mmce_sio2_tx_rx_pio           DECLARE_IMPORT(8, mmce_sio2_tx_rx_pio)
#define I_mmce_sio2_rx                  DECLARE_IMPORT(9, mmce_sio2_rx)
#define I_mmce_sio2_tx                  DECLARE_IMPORT(10, mmce_sio2_tx)
#endif
