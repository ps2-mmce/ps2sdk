#ifndef MMCEMAN_H
#define MMCEMAN_H

extern u8 mmce_port;

//Called through devctl
int mmce_cmd_ping_quick(void);
int mmce_cmd_ping(void);
int mmce_cmd_get_status(void);
int mmce_cmd_get_card(void);
int mmce_cmd_set_card(u8 type, u8 mode, u16 num);
int mmce_cmd_get_channel(void);
int mmce_cmd_set_channel(u8 mode, u16 num);
int mmce_cmd_get_gameid(void *ptr);
int mmce_cmd_set_gameid(void *ptr);
int mmce_cmd_reset(void);

#endif