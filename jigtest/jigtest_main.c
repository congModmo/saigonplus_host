/*
 * jigtest_main.c
 *
 *  Created on: Sep 30, 2021
 *      Author: thanhcong
 */


#define __DEBUG__ 4

#include "jigtest_main.h"
#include <string.h>
#include "retarget.h"
#include "app_config.h"
#include "app_main/app_main.h"
#include <app_ble/app_ble.h>
#include "bsp.h"
#include "console/console.h"
#include <app_common/wcb_ioctl.h>
#include <app_gps/app_gps.h>
#include <app_common/wcb_common.h>
#include <app_common/common.h>
#include "app_main/app_info.h"
#include "log_sys.h"
#include "app_imu/app_imu.h"
#include "app_display/app_display.h"
#include "app_main/publish_scheduler.h"
#include "iwdg.h"
#include <_usart.h>
#include "app_main/uart_ui_comm.h"
#include "app_fota/fota_core.h"
#include "app_fota/serial_transport.h"
#include "tim.h"
#include "crc.h"
#include "ble/nina_b1.h"

static bool uart_ui_finish = false;
static uint8_t uart_ui_exit_type = UART_UI_EXIT;
static __IO bool fota_finish = false;
static fota_file_info_t json_info;
static __IO bool fota_start = false;
static uint8_t fota_source;
static void *fota_params;
bool system_ready = false;

bool system_is_ready()
{
	return system_ready;
}

void uart_ui_callback(uint8_t type, fota_file_info_t *file)
{
	uart_ui_finish = true;
	uart_ui_exit_type = type;
	json_info.crc = file->crc;
	json_info.len = file->len;
}

void fota_callback(uint8_t source, fota_status_t host_fota, fota_status_t ble_fota)
{
	fota_finish = true;
	if(ble_fota!=FOTA_NONE)
	{
		 jigtest_direct_report(UART_UI_RES_BLE_DFU, (ble_fota==FOTA_DONE)?1:0);
	}
	if(host_fota==FOTA_FAIL)
	{
		 jigtest_direct_report(UART_UI_RES_HOST_DFU, 0);
	}
	else if(host_fota==FOTA_DONE)
	{
		debug("Restart to update host\n");
		delay(10);
		NVIC_SystemReset();
	}
}

void fota_start_process(uint8_t source, void *params)
{
	const fota_transport_t *transport;
	if (source == FOTA_OVER_UART)
	{
		serial_transport_init(uart_transport);
		transport = &serial_transport;
		eva_m8_bsp_uart_deinit();
		uart_ui_comm_init(true);
	}
	else if (source == FOTA_OVER_LTE)
	{
		transport = &lte_transport;
	}
	else if (source == FOTA_OVER_BLE)
	{
		serial_transport_init(ble_serial);
		transport = &serial_transport;
	}
	fota_core_init(transport, fota_callback, &json_info, params, source);
	while (!fota_finish)
	{
		fota_core_process();
		delay(5);
		WATCHDOG_FEED();
	}
	if(source==FOTA_OVER_UART)
	{
		uart_ui_comm_init(false);
		eva_m8_bsp_init();
	}
}

bool serial_fota_request_handle(serial_fota_request_info_t *request, uint8_t source){
	json_info.crc=request->info_crc;
	json_info.len=request->info_len;
	json_info.flash_addr= EX_FLASH_FOTA_FILES_START;
	strcpy(json_info.file_name, "info.json");
	debug("%s fota request\n", source==FOTA_OVER_UART?"uart":"ble");
	debug("Info len: %d\n", request->info_len);
	debug("Info crc: %u\n", request->info_crc);
	fota_start=true;
	fota_finish=false;
	fota_source= source;
	fota_params=NULL;
	return true;
}

void main_mail_process()
{
	static mail_t mail;
	if (osOK == osMessageQueueGet(mainMailHandle, &mail, NULL, 0))
	{
		debug("Check mail\n");
		if (mail.type == MAIL_MQTT_V2_SERVER_DATA)
		{
			protocol_v3_handle_cmd(mail.data, mail.len);
		}
	__exit:
		if (mail.data != NULL)
		{
			free(mail.data);
		}
	}
}
void app_main_enter_config_mode()
{

}

void host_upgrade_check()
{
	if(host_app_upgrade_form_uart_check())
	{
		 jigtest_direct_report(UART_UI_RES_HOST_DFU, 1);
		 debug("Just upgrade host from uart\n");
	}
	else{
		debug("Normal boot\n");
	}
}

void app_main(void)
{
	retarget_init();
	uart_ui_comm_init(false);
	host_upgrade_check();
	light_control_init();
	jigtest_uart_esp_init();
	app_ble_init();
	app_gps_init();
	lara_r2_bsp_init();
	app_info_init();
	ioctl_beep(100);
	while (1)
	{
		uart_ui_comm_polling();
		if (fota_start)
		{
			fota_start = false;
			if (fota_source == FOTA_OVER_LTE)
				delay(1000); // https get file will interrupt mqtt, wait to publish ack
			fota_start_process(fota_source, fota_params);
		}
		main_mail_process();
		fota_core_process();
		app_ble_task();
		delay(5);
	}

