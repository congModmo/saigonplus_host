/*
 * uart_fota.h
 *
 *  Created on: Apr 6, 2021
 *      Author: thanhcong
 */

#ifndef UART_UI_COMM_H_
#define UART_UI_COMM_H_



#include "bsp.h"

#include "fota_core.h"
#ifdef __cplusplus
extern "C" {
#endif

//#define EXT_FLASH_TEST
#include "app_fota/serial_transport.h"
enum{
	//ui->device
    UART_UI_CMD_PING			=0,
	UART_UI_CMD_FACTORY_RESET	=1,
	UART_UI_CMD_FOTA_DATA		=2,
    UART_UI_RESPONSE_STATUS_OK    =253,
    UART_UI_RESPONSE_STATUS_ERR   	=254,
	UART_UI_CMD_DUMMY=255
};

enum{
	UART_UI_EXIT,
	UART_UI_FOTA_REQUEST
};

typedef struct __packed
{
	uint8_t type;
	uint8_t status;
	uint16_t crc;
} uart_ui_response_t;

typedef struct __packed
{
	uint8_t cmd;
	int len;
	uint32_t crc32;
}uart_ui_file_info_t;

typedef void (*uart_ui_callback_t)(uint8_t type, fota_file_info_t *info);
typedef void (*uart_ui_data_callback_t)(uint16_t len);

extern const serial_interface_t uart_transport;

void uart_ui_init();
void uart_ui_command_send(uint8_t type, uint8_t status);
void uart_ui_send(uint8_t *data, uint16_t len);
void uart_ui_polling(void);
void factory_reset();
#ifdef __cplusplus
}
#endif


#endif /* UART_UI_COMM_H_ */
