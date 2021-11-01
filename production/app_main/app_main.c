/*
 * app_main.c
 *
 *  Created on: Mar 16, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4

#include <string.h>
#include "retarget.h"
#include "app_config.h"
#include "app_main.h"
#include <app_ble/app_ble.h>
#include "bsp.h"
#include "console/console.h"
#include <app_common/wcb_ioctl.h>
#include <app_gps/app_gps.h>
#include "app_lte/lteTask.h"
#include <app_common/wcb_common.h>
#include <app_common/common.h>
#include "app_info.h"
#include "log_sys.h"
#include "app_imu/app_imu.h"
#include "app_display/app_display.h"
#include "publish_scheduler.h"
#include "iwdg.h"
#include "usart.h"
#include "uart_ui_comm.h"
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

void fota_callback(uint8_t source, bool status)
{
	fota_finish = true;
	debug("FOTA  is %s\n", status ? "done" : "failed");
}

void fota_start_process(uint8_t source, void *params)
{
	const fota_transport_t *transport;
	if (source == FOTA_OVER_UART)
	{
		serial_transport_init(uart_transport);
		transport = &serial_transport;
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
	fota_core_init(transport, fota_callback, &json_info, params);
	while (!fota_finish)
	{
		fota_core_process();
		delay(5);
		WATCHDOG_FEED();
	}
}

bool serial_fota_request_handle(serial_fota_request_info_t *request, uint8_t source)
{
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

static bool config_mode=false;
void app_main_enter_config_mode()
{
	config_mode=true;
}

void uart_ui_process()
{

}

void app_main(void)
{

#ifndef RELEASE
	console_init();
	MX_USART1_UART_Init(115200, false);
	RetargetInit(&huart1);
	info("Hello modmo\n");

	/* File read buffer */
#endif
	app_info_init();
	debug("Host app version: %d\n", firmware_version->hostApp);
	debug("Ble app version: %d\n", firmware_version->bleApp);
#ifdef DISPLAY_ENABLE
	uart_ui_process();
	if(!config_mode)
	{
		app_display_init();
		app_display_set_mode(*bike_locked?DISPLAY_ANTI_THEFT_MODE:DISPLAY_NORMAL_MODE);
	}
#endif
#ifdef PUBLISH_ENABLE
	publish_scheduler_init();
#endif
#ifdef BLE_ENABLE
	app_ble_init();
#endif

#ifdef LTE_ENABLE
	system_ready = true;
#endif

#ifdef IMU_ENABLE
	app_imu_init();
#endif

#ifdef ALARM_ENABLE
	alarm_init();
#endif

#ifdef GPS_ENABLE
	if(!config_mode)
		app_gps_init();
#endif

#ifdef REPORT_ENABLE
	publish_scheduler_init();
#endif

	ioctl_beepbeep(3, 100);
	uint32_t tick=millis();
	while (1)
	{
		if(millis()-tick>1000)
		{
			tick=millis();
			debug("lte rssi: %d, gps hdop: %.2f\n", *lteRssi, gps_data->hdop);
		}
#ifdef DISPLAY_ENABLE
		if(!config_mode)
			app_display_process();
		else
			uart_ui_comm_polling();
#endif

#ifdef GPS_ENABLE
		if(!config_mode)
			app_gps_process();
#endif
#ifdef PUBLISH_ENABLE
		publish_scheduler_process();
#endif
#ifdef BLE_ENABLE
		app_ble_task();
#endif
#ifdef IMU_ENABLE
		app_imu_process();
#endif
#ifdef ALARM_ENABLE
		alarm_process();
#endif
#ifndef RELEASE
		console_task();
#endif
#ifdef REPORT_ENABLE
		publish_scheduler_process();
#endif
		if (fota_start)
		{
			fota_start = false;
			if (fota_source == FOTA_OVER_LTE)
				delay(1000); // https get file will interrupt mqtt, wait to publish ack
			fota_start_process(fota_source, fota_params);
		}
		main_mail_process();

		fota_core_process();
		delay(5);
	}
}
