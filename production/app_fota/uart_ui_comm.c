/*
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
#define __DEBUG__ 3
#include "uart_ui_comm.h"
#include <string.h>
#include "log_sys.h"
#include <bsp.h>
#include "flash_fota.h"
#include "fota_core.h"
#include "crc/crc16.h"
#include "crc/crc32.h"
#include "slip/slip_v2.h"
#include "app_fota/serial_transport.h"
#include "app_display/app_display.h"
static slip_t slip;
static uint8_t slipBuff[512];
static uint32_t tick=0;
static uart_ui_callback_t _callback=NULL;
static RINGBUF ui_comm_rb;
static frame_handler serial_handler=NULL;

void uart_ui_command_send(uint8_t type, uint8_t status)
{
	 slip_send(&slip, &type, 1, SLIP_FRAME_BEGIN);
	 slip_send(&slip, &status, 1, SLIP_FRAME_END);
}

static void uart_ui_command_handle(uint8_t *frame, size_t size)
{
	static serial_fota_request_info_t *request;
	tick=millis();
	switch (frame[0])
	{
	case UART_UI_CMD_FACTORY_RESET:
		uart_ui_command_send(UART_UI_CMD_FACTORY_RESET, UART_UI_RESPONSE_STATUS_OK);
		//factory_reset();
		break;
	case UART_UI_CMD_PING:
		uart_ui_command_send(UART_UI_CMD_PING, UART_UI_RESPONSE_STATUS_OK);
		break;
	case UART_UI_CMD_FOTA_DATA:
		if(frame[1]==FOTA_REQUEST_FOTA){
			request=(serial_fota_request_info_t *)(frame+2);
			app_serial_fota_request(FOTA_OVER_UART, request);
		}
		else if(frame[1]==FOTA_REQUEST_MTU)
		{
			debug("Send mtu: %d\n", uart_transport.mtu);
			uint8_t frame[3]={UART_UI_CMD_FOTA_DATA, DEVICE_SET_MTU, uart_transport.mtu};
			slip_send(&slip, frame, 3, SLIP_FRAME_COMPLETE);
		}
		break;
	default:
		break;
	}
}

static void uart_ui_fota_handler(uint8_t *frame, size_t len){
	if(frame[0] ==UART_UI_CMD_FOTA_DATA && serial_handler!=NULL ){
		serial_handler(frame+1, len-1);
	}
}

void uart_ui_polling_reset(){
	tick=millis();
}

void uart_ui_comm_init()
{
	slip_init(&slip, slipBuff, 512, bsp_ui_comm_send_byte);
	bsp_ui_comm_init(&ui_comm_rb);
}

static void polling(frame_handler handler){
	while (RINGBUF_Available(&ui_comm_rb))
	{
		static uint8_t c;
		RINGBUF_Get(&ui_comm_rb, &c);
		static uint32_t len;
		len=slip_parse(&slip, c);
		if (len>0 &&handler !=NULL)
		{
			handler(slip.buff, len);
		}
	}
}

void uart_ui_comm_polling(void)
{
	polling(uart_ui_command_handle);
}

static void serial_polling(frame_handler handler){
	serial_handler=handler;
	polling(uart_ui_fota_handler);
}

static void serial_send(uint8_t *data, size_t len, bool _continue){
	static bool start_frame=true;
	static uint8_t cmd=UART_UI_CMD_FOTA_DATA;
	if(!_continue){
		if(start_frame){

			slip_send(&slip, &cmd, 1, SLIP_FRAME_BEGIN);
			slip_send(&slip, data, len, SLIP_FRAME_END);
		}
		else{
			start_frame=true;
			slip_send(&slip, data, len, SLIP_FRAME_END);
		}
	}
	else{
		if(start_frame){
			start_frame=false;
			slip_send(&slip, &cmd, 1, SLIP_FRAME_BEGIN);
			slip_send(&slip, data, len, SLIP_FRAME_MIDDLE);
		}
		else{
			slip_send(&slip, data, len, SLIP_FRAME_MIDDLE);
		}
	}
}

static void serial_init()
{
}

const serial_interface_t uart_transport= {.mtu=128, .init=serial_init, .send=serial_send, .polling=serial_polling};

