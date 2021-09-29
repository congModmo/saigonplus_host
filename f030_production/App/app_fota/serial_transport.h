/*
 * ble_transport.h
 *
 *  Created on: Jun 14, 2021
 *      Author: thanhcong
 */

#ifndef _BLE_TRANSPORT_H_
#define _BLE_TRANSPORT_H_
#include <stdint.h>
#include "fota_core.h"
#include "slip/slip_v2.h"
typedef enum{
    FOTA_REQUEST_FOTA = 128,
	FOTA_FILE_INFO	=129,
	FOTA_SECTOR_INFO = 130,
	FOTA_SECTOR_DATA 	= 131,
	FOTA_REQUEST_MTU=132,
    //mcu->app
	DEVICE_REQUEST_FILE_INFO 	= 200,
    DEVICE_REQUEST_SECTOR_INFO 	= 201,
	DEVICE_REQUEST_SECTOR_DATA	=202,
    DEVICE_REQUEST_BLOCK 	= 203,
	DEVICE_REQUEST_PROCESS_END = 204,
    DEVICE_PROCESS_SECTOR   =205,
    DEVICE_PROCESS_FILE     =206,
	DEVICE_FIRMWARE_VERSION=207,
	DEVICE_SET_MTU=			208,
	DEVICE_STATUS_OK		=254,
	DEVICE_STATUS_ERROR		=255,
}serial_fota_cmd_t;

typedef struct __packed{
    int info_len;
    uint32_t info_crc;
}serial_fota_request_info_t;

typedef struct __packed
{
	int len;
	uint32_t crc;
} serial_data_info_t;

typedef void (*serial_polling_t)(frame_handler handler);
typedef void (*serial_send_t)(uint8_t *data, size_t len, bool _continue);
typedef void (*serial_init_t)();

typedef struct{
	uint8_t mtu;
	serial_init_t init;
	serial_polling_t polling;
	serial_send_t send;
}serial_interface_t;

#define MIN_BLOCK_LEN 64
#define MAX_SECTOR_LEN 4096

extern const fota_transport_t serial_transport;
void serial_transport_init(serial_interface_t serial);
#endif /* APP_FOTA_BLE_TRANSPORT_H_ */
