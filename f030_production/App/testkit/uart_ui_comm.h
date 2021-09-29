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
enum{
	//ui->device
    UART_UI_CMD_PING			=0,
	UART_UI_CMD_FACTORY_RESET	=1,
	UART_UI_FOTA_DATA			=2,
	UART_UI_CMD_TEST_ALL_HW		=3,
	UART_UI_CMD_TEST_OTHER		=4,
	UART_UI_CMD_TEST_BLE		=5,
	UART_UI_CMD_TEST_LTE		=6,
	UART_UI_CMD_TEST_GPS		=7,
	UART_UI_CMD_TEST_IMU		=8,
	UART_UI_CMD_TEST_IO			=9,
	UART_UI_CMD_TEST_NETWORK	=10,
	UART_UI_CMD_TEST_MQTT		=11,

//	UART_UI_KEY_DATA			=20,
//	UART_UI_KEY_REQUEST			=21,
//	UART_UI_KEY_CA				=22,
	UART_UI_LTE_CERT			=20,
	UART_UI_LTE_KEY				=21,
	UART_UI_CMD_IMPORT_KEY 		=22,

	UART_UI_RES_LTE_IMEI		=130,
	UART_UI_RES_LTE_SIM_CCID	=131,
	UART_UI_RES_LTE_RSSI		=132,
	UART_UI_RES_LTE_RESET		=133,
	UART_UI_RES_LTE_TXRX		=134,
	UART_UI_RES_LTE_CARRIER		=135,
	UART_UI_RES_LTE_2G			=136,
	UART_UI_RES_LTE_4G			=137,
	UART_UI_RES_MQTT_TEST		=138,
	UART_UI_RES_LTE_KEY			=139,

	UART_UI_RES_BLE_RESET		=140,
	UART_UI_RES_BLE_TXRX		=141,
	UART_UI_RES_BLE_MAC			=142,
	UART_UI_RES_BLE_RSSI		=143,

	UART_UI_RES_GPS_RESET		=150,
	UART_UI_RES_GPS_POSITION	=151,
	UART_UI_RES_GPS_TXRX		=152,
	UART_UI_RES_GPS_HDOP		=153,

	UART_UI_RES_FRONT_LIGHT		=160,
	UART_UI_RES_LED_BLUE		=161,
	UART_UI_RES_LED_GREEN		=162,
	UART_UI_RES_LED_RED			=163,
	UART_UI_RES_LOCKPIN			=164,
	UART_UI_RES_3V3				=165,
	UART_UI_RES_3V8				=166,
	UART_UI_RES_12V				=167,
	UART_UI_RES_ESP_ADAPTOR		=168,

	UART_UI_RES_IMU_TEST 		=170,
	UART_UI_RES_IMU_TRIGGER 	=171,
	UART_UI_RES_UART_DISPLAY 	=172,
	UART_UI_RES_EXTERNAL_FLASH 	=173,
	UART_UI_RES_SERIAL_DFU 		=174,
	UART_UI_RES_BLE_DFU			=175,

	UART_UI_RES_PING 			=251,
	UART_UI_RES_FINISH			=252,
    UART_UI_RES_OK		    	=253,
    UART_UI_RES_ERR   			=254,
	UART_UI_CMD_DUMMY=			255
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
void uart_ui_send(uint8_t type, uint8_t *data, uint16_t len);
void uart_ui_polling(void);
void factory_reset();
#ifdef __cplusplus
}
#endif


#endif /* UART_UI_COMM_H_ */
