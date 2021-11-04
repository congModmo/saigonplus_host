/*
 * uart_fota.h
 *
 *  Created on: Apr 6, 2021
 *      Author: thanhcong
 */

#ifndef UART_UI_COMM_H_
#define UART_UI_COMM_H_



#include "bsp.h"

#include "app_fota/fota_core.h"
#ifdef __cplusplus
extern "C" {
#endif

//#define EXT_FLASH_TEST
#include "app_fota/serial_transport.h"
typedef enum{
	//ui->device
    UART_UI_CMD_PING			=130,
	UART_UI_CMD_FACTORY_RESET	=131,
	UART_UI_FOTA_DATA			=132,
	UART_UI_CMD_READ_MAC		=133,
	UART_UI_CMD_READ_CCID		=134,
	UART_UI_CMD_READ_IMEI		=135,
	UART_UI_CMD_READ_SN			=136,
	UART_UI_CMD_SET_SN			=137,

	UART_UI_CMD_TEST_ALL_HW		=150,
	UART_UI_CMD_TEST_FUNCTION 	=151,
	UART_UI_CMD_READOUT_PROTECT	=152,
	UART_UI_LTE_CERT			=153,
	UART_UI_LTE_KEY				=154,
	UART_UI_CMD_IMPORT_KEY 		=155,

	UART_UI_RES_LTE_IMEI		=160,
	UART_UI_RES_LTE_SIM_CCID	=161,
	UART_UI_RES_LTE_RSSI		=162,
	UART_UI_RES_LTE_RESET		=163,
	UART_UI_RES_LTE_TXRX		=164,
	UART_UI_RES_LTE_CARRIER		=165,
	UART_UI_RES_LTE_2G			=166,
	UART_UI_RES_LTE_4G			=167,
	UART_UI_RES_MQTT_TEST		=168,
	UART_UI_RES_LTE_KEY			=169,

	UART_UI_RES_BLE_RESET		=180,
	UART_UI_RES_BLE_TXRX		=181,
	UART_UI_RES_BLE_MAC			=182,
	UART_UI_RES_BLE_RSSI		=183,
	UART_UI_RES_BLE_CONNECT		=184,
	UART_UI_RES_BLE_TRANSCEIVER	=185,

	UART_UI_RES_GPS_RESET		=190,
	UART_UI_RES_GPS_POSITION	=191,
	UART_UI_RES_GPS_TXRX		=192,
	UART_UI_RES_GPS_HDOP		=193,

	UART_UI_RES_LED_HEAD		=200,
	UART_UI_RES_LED_BLUE		=201,
	UART_UI_RES_LED_GREEN		=202,
	UART_UI_RES_LED_RED			=203,
	UART_UI_RES_LOCKPIN			=204,
	UART_UI_RES_3V3				=205,
	UART_UI_RES_3V8				=206,
	UART_UI_RES_12V				=207,
	UART_UI_RES_ESP_ADAPTOR		=208,

	UART_UI_RES_IMU_TEST 		=210,
	UART_UI_RES_IMU_TRIGGER 	=211,
	UART_UI_RES_UART_DISPLAY 	=212,
	UART_UI_RES_EXTERNAL_FLASH 	=213,
	UART_UI_RES_HOST_DFU 		=214,
	UART_UI_RES_BLE_DFU			=215,

	UART_UI_RES_PING 			=220,
	UART_UI_RES_FINISH			=221,
    UART_UI_RES_OK		    	=222,
    UART_UI_RES_ERR   			=223,
	UART_UI_CMD_DUMMY			=224,
	UART_UI_RES_SN				=225,
}ui_comm_cmd_t;

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

void uart_ui_comm_init(bool highspeed);
void uart_ui_comm_command_send(uint8_t type, uint8_t status);
void uart_ui_comm_send(uint8_t type, uint8_t *data, uint16_t len);
void uart_ui_comm_polling(void);
void factory_reset();
#ifdef __cplusplus
}
#endif


#endif /* UART_UI_COMM_H_ */
