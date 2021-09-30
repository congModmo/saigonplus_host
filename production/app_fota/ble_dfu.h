/*
 * ble_dfu.h
 *
 *  Created on: Dec 23, 2020
 *      Author: thanhcong
 */

#ifndef _BLE_DFU_H_
#define _BLE_DFU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nrf_dfu_req_handler.h"
#include "flash_fota.h"
#include "fota_core.h"
typedef struct{
	uint16_t mtu;
	int prn;
}ble_dfu_t;

void ble_dfu_init(fota_file_info_t *dat, fota_file_info_t *bin);
bool ble_dfu_process();
void ble_slip_send_cmd(uint8_t *cmd, size_t cmd_len);
bool ble_cmd_get_resp(uint8_t *cmd, size_t cmd_len, uint8_t **resp, uint32_t *resp_len, size_t timeout);

bool ble_slip_ping(uint8_t id);
bool ble_slip_prn();
bool ble_slip_mtu(uint16_t *mtu);
bool ble_slip_object_select(uint8_t type, nrf_dfu_response_select_t *select);
bool ble_dfu_start();
bool ble_dfu_send_init_packet();
bool ble_dfu_send_firmware_packet();
#ifdef __cplusplus
}
#endif
#endif /* STM32_NRF5_DFU_BLE_DFU_H_ */
