/*
 * fota_core.h
 *
 *  Created on: Apr 13, 2021
 *      Author: thanhcong
 */

#ifndef FOTA_CORE_H_
#define FOTA_CORE_H_

#include "bsp.h"
#include "fota_comm.h"
typedef struct{
	char file_name[32];
	int len;
	uint32_t flash_addr;
	uint32_t crc;
}fota_file_info_t;

enum{
	FOTA_OVER_BLE=0,
	FOTA_OVER_LTE=1,
	FOTA_OVER_UART=2
};

typedef enum{
	FOTA_NONE,
	FOTA_DONE,
	FOTA_FAIL
}fota_status_t;

typedef void (*fota_callback_t)(uint8_t fota_source, fota_status_t host, fota_status_t ble);
typedef void (*transport_get_file_cb_t)(bool status);
typedef void (*transport_err_cb_t)(int num);
typedef bool (*transport_init_t)(uint8_t *buff, size_t size, void *params, transport_err_cb_t cb);
typedef bool (*transport_get_file_t)(fota_file_info_t *info, transport_get_file_cb_t cb);
typedef void (*transport_process_t)(void);
typedef void (*transport_fota_signal_t)(bool status);

typedef struct{
	uint8_t source;
	transport_init_t init;
	transport_get_file_t get_file;
	transport_process_t process;
	transport_fota_signal_t signal;
}fota_transport_t;

extern uint8_t fotaCoreBuff[FLASH_FOTA_SECTOR_SZ];

void fota_core_init(const fota_transport_t *transport, fota_callback_t cb, fota_file_info_t * info_json, void *params, uint8_t source);
void fota_core_process(void);
bool fota_check_fota_request(fota_request_msg_t *request);
#endif /* FOTA_CORE_H_ */
