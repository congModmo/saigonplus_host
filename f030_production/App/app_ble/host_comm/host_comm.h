/*
    Host communication protocol & stack

    27/11/2020
    HieuNT
*/

#ifndef __HOST_COMM_H_
#define __HOST_COMM_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
typedef enum
{
    HOST_COMM_BLE_MSG=0,
    HOST_COMM_UI_MSG=1,
}host_comm_msg_type_t;

typedef enum
{
    BLE_STATE_CONNECTED=0,
    BLE_STATE_DISCONNECTED,
    BLE_STATE_CONN_PARAMS_UPDATE,
    //
    BLE_CMD_SET_MAC=10,
    //
    HOST_CMD_DISCONNECT=20,
    HOST_CMD_SYN_INFO,
    HOST_CMD_DFU,
    HOST_CMD_FAST_MODE,
    HOST_CMD_NORMAL_MODE,
    //status flag
    HOST_BLE_OK=254,
    HOST_BLE_ERROR=255,
}host_ble_msg_type_t;

#ifdef __cplusplus
}
#endif

#endif
