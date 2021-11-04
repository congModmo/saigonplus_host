/*
 * kie
 * uart_fota.c
 *
 *  Created on: Apr 6, 2021
 *      Author: thanhcong
 */

/*
    RS232 bootloader

    26/12/2020
    HieuNT
*/
#define __DEBUG__ 4

#include "uart_ui_comm.h"
#include "app_main/app_main.h"
#include <string.h>
#include "log_sys.h"
#include "bsp.h"
#include "crc/crc16.h"
#include "crc/crc32.h"
#include "slip/slip_v2.h"
#include "app_fota/serial_transport.h"
#include "app_display/app_display.h"
#include "app_lte/lteTask.h"
#include "app_info.h"

static slip_t slip;
static uint8_t slip_buff[512];
static uint32_t tick = 0;
static RINGBUF uartUiRingbuf;

static frame_handler serial_handler = NULL;

char *ui_cmd_to_string(ui_comm_cmd_t cmd)
{
	switch (cmd)
	{
	case UART_UI_RES_LTE_IMEI:
		return "UART_UI_RES_LTE_IMEI";
	case UART_UI_RES_LTE_SIM_CCID:
		return "UART_UI_RES_LTE_SIM_CCID";
	case UART_UI_RES_LTE_RSSI:
		return "UART_UI_RES_LTE_RSSI";
	case UART_UI_RES_LTE_RESET:
		return "UART_UI_RES_LTE_RESET";
	case UART_UI_RES_LTE_TXRX:
		return "UART_UI_RES_LTE_TXRX";
	case UART_UI_RES_LTE_CARRIER:
		return "UART_UI_RES_LTE_CARRIER";
	case UART_UI_RES_LTE_2G:
		return "UART_UI_RES_LTE_2G";
	case UART_UI_RES_LTE_4G:
		return "UART_UI_RES_LTE_4G";
	case UART_UI_RES_MQTT_TEST:
		return "UART_UI_RES_MQTT_TEST";
	case UART_UI_RES_BLE_RESET:
		return "UART_UI_RES_BLE_RESET";
	case UART_UI_RES_BLE_TXRX:
		return "UART_UI_RES_BLE_TXRX";
	case UART_UI_RES_BLE_MAC:
		return "UART_UI_RES_BLE_MAC";
	case UART_UI_RES_BLE_RSSI:
		return "UART_UI_RES_BLE_RSSI";
	case UART_UI_RES_GPS_RESET:
		return "UART_UI_RES_GPS_RESET";
	case UART_UI_RES_GPS_POSITION:
		return "UART_UI_RES_GPS_POSITION";
	case UART_UI_RES_GPS_TXRX:
		return "UART_UI_RES_GPS_TXRX";
	case UART_UI_RES_GPS_HDOP:
		return "UART_UI_RES_GPS_HDOP";
	case UART_UI_RES_LED_HEAD:
		return "UART_UI_RES_LED_HEAD";
	case UART_UI_RES_LED_BLUE:
		return "UART_UI_RES_LED_BLUE";
	case UART_UI_RES_LED_GREEN:
		return "UART_UI_RES_LED_GREEN";
	case UART_UI_RES_LED_RED:
		return "UART_UI_RES_LED_LED";
	case UART_UI_RES_LOCKPIN:
		return "UART_UI_RES_LOCKPIN";
	case UART_UI_RES_3V3:
		return "UART_UI_RES_3V3";
	case UART_UI_RES_3V8:
		return "UART_UI_RES_3V8";
	case UART_UI_RES_12V:
		return "UART_UI_RES_12V";
	case UART_UI_RES_IMU_TEST:
		return "UART_UI_RES_IMU_TEST";
	case UART_UI_RES_ESP_ADAPTOR:
		return "UART_UI_RES_ESP_ADAPTOR";
	case UART_UI_RES_IMU_TRIGGER:
		return "UART_UI_RES_IMU_TRIGGER";
	case UART_UI_RES_UART_DISPLAY:
		return "UART_UI_RES_UART_DISPLAY";
	case UART_UI_RES_EXTERNAL_FLASH:
		return "UART_UI_RES_EXTERNAL_FLASH";
	case UART_UI_RES_HOST_DFU:
		return "UART_UI_RES_HOST_DFU";
	case UART_UI_RES_BLE_DFU:
		return "UART_UI_RES_BLE_DFU";
	}
	return NULL;
}

void uart_ui_comm_command_send(uint8_t type, uint8_t status)
{
	slip_send(&slip, &type, 1, SLIP_FRAME_BEGIN);
	slip_send(&slip, &status, 1, SLIP_FRAME_END);
}

void uart_ui_comm_send(uint8_t type, uint8_t *data, uint16_t len)
{
	slip_send(&slip, &type, 1, SLIP_FRAME_BEGIN);
	slip_send(&slip, data, len, SLIP_FRAME_END);
}

void uart_ui_comm_send0(uint8_t *data, size_t len)
{
	slip_send(&slip, data, len,SLIP_FRAME_COMPLETE);
}

static void uart_ui_command_handle(uint8_t *frame, size_t size)
{
	tick = millis();
#ifdef JIGTEST
	if(frame[0]<128)
	{
		jigtest_console_handle(frame);
		return;
	}
	else if(frame[0]>=UART_UI_CMD_TEST_ALL_HW)
	{
		jigtest_cmd_handle(frame, size);
		return;
	}
#endif

	switch (frame[0])
	{
	case UART_UI_CMD_PING:
		//for production config ui
		uart_ui_comm_command_send(UART_UI_CMD_PING, UART_UI_RES_OK);
		app_main_enter_config_mode();
		break;
	case UART_UI_CMD_FACTORY_RESET:
		app_config_factory_reset();
		break;
	case UART_UI_FOTA_DATA:
		if (frame[1] == FOTA_REQUEST_FOTA)
		{
			serial_fota_request_info_t *request = (serial_fota_request_info_t *)(frame + 2);
			serial_fota_request_handle(request, FOTA_OVER_UART);
		}
		else if(frame[1]==FOTA_REQUEST_MTU)
		{
			uint8_t data[2]={DEVICE_SET_MTU, uart_transport.mtu};
			uart_ui_comm_send(UART_UI_FOTA_DATA, data, 2);
		}
		break;
	case UART_UI_CMD_READ_MAC:
		if(strlen(ble_mac)>0)
		{
			uart_ui_comm_send(UART_UI_RES_BLE_MAC, (uint8_t *)ble_mac, strlen(ble_mac));
		}
		break;
	case UART_UI_CMD_READ_CCID:
		if(strlen(lteCcid)>0)
		{
			uart_ui_comm_send(UART_UI_RES_LTE_SIM_CCID, (uint8_t *)lteCcid, strlen(lteCcid));
		}
		break;
	case UART_UI_CMD_READ_IMEI:
		if(strlen(lteImei)>0)
		{
			uart_ui_comm_send(UART_UI_RES_LTE_IMEI, (uint8_t *)lteImei, strlen(lteImei));
		}
		break;
	case UART_UI_CMD_READ_SN:
		uart_ui_comm_send(UART_UI_RES_SN, serial_number, strlen(serial_number));
	break;
	case UART_UI_CMD_SET_SN:
		frame[size]=0;
		app_info_update_serial_number(frame+1);
		uart_ui_comm_command_send(UART_UI_CMD_SET_SN, UART_UI_RES_OK);
	break;
	default:
		break;
	}
}

static void uart_ui_fota_handler(uint8_t *frame, size_t len)
{
	if (frame[0] == UART_UI_FOTA_DATA && serial_handler != NULL)
	{
		serial_handler(frame + 1, len - 1);
	}
}

void uart_ui_polling_reset()
{
	tick = millis();
}

static void polling(frame_handler handler)
{
	while (RINGBUF_Available(&uartUiRingbuf))
	{
		static uint8_t c;
		RINGBUF_Get(&uartUiRingbuf, &c);
		static uint32_t len;
		len = slip_parse(&slip, c);
		if (len > 0 && handler != NULL)
		{
			handler(slip.buff, len);
		}
	}
}

/****************************************************************************************
 * Interface
 ****************************************************************************************/

static void serial_polling(frame_handler handler)
{
	serial_handler = handler;
	polling(uart_ui_fota_handler);
}

static void serial_send(uint8_t *data, size_t len, slip_frame_type_t type)
{
	if(type==SLIP_FRAME_BEGIN){
		uint8_t cmd=UART_UI_FOTA_DATA;
		slip_send(&slip, &cmd, 1, SLIP_FRAME_BEGIN);
		slip_send(&slip, data, len, SLIP_FRAME_MIDDLE);
	}
	else{
		slip_send(&slip, data, len, type);
	}
}

static void uart_transport_init()
{

}

const serial_interface_t uart_transport = {.init=uart_transport_init, .mtu = 128, .send = serial_send, .polling = serial_polling};

/****************************************************************************************
 * Public APIs
 ****************************************************************************************/

void uart_ui_comm_init(bool highspeed)
{
	bsp_ui_comm_deinit();
	bsp_ui_comm_init(&uartUiRingbuf, highspeed);
	slip_init(&slip, true, slip_buff, 512, bsp_ui_comm_send_byte);
}

void uart_ui_comm_polling(void)
{
	polling(uart_ui_command_handle);
}
