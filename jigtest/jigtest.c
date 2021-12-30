/*
 * jigtest.c
 *
 *  Created on: Jul 5, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include "bsp.h"
#include "app_main/app_main.h"
#include "app_imu/app_imu.h"
#include "jigtest.h"
#include "jigtest_esp.h"
#include "app_config.h"
#include "jigtest_io.h"
#include "jigtest_ble.h"
#include "jigtest_gps.h"
#include "jigtest_lte.h"
#include "app_main/uart_ui_comm.h"

static struct {
	//hardware
	uint8_t ble_reset;
	uint8_t ble_tx_rx;
	uint8_t gps_reset;
	uint8_t gps_tx_rx;
	uint8_t lte_reset;
	uint8_t lte_tx_rx;
	char lte_imei[32];
	char lte_ccid[32];
	uint8_t esp_adaptor;
	uint8_t headlight;
	uint8_t sidelight_red;
	uint8_t sidelight_green;
	uint8_t sidelight_blue;
	uint8_t lockpin;
	uint8_t imu_test;
	uint8_t imu_trigger;
	uint8_t external_flash;
	//function
	uint8_t gprs_register;
	uint8_t lte_register;
	uint8_t mqtt_test;
	uint8_t lte_csq;
	char lte_carrier[32];
	uint8_t lte_key;
	char ble_mac[32];
	uint8_t ble_rssi;
	uint8_t ble_fota;
	uint8_t gps_position;
	uint8_t gps_hdop;
	uint8_t ble_transceiver;
	uint8_t ble_connect;
}jigtest_result;

void jigtest_report(uint8_t type, uint8_t *data, uint8_t len) {
	JIGTEST_LOCK();
	switch (type) {
	case UART_UI_RES_LTE_IMEI:
		memcpy(jigtest_result.lte_imei, data, len);
		break;
	case UART_UI_RES_LTE_SIM_CCID:
		memcpy(jigtest_result.lte_ccid, data, len);
		break;
	case UART_UI_RES_LTE_CARRIER:
		memcpy(jigtest_result.lte_carrier, data, len);
		break;
	case UART_UI_RES_BLE_MAC:
		memcpy(jigtest_result.ble_mac, data, len);
		break;
	}
	JIGTEST_UNLOCK();
}

void jigtest_direct_report(uint8_t type, uint8_t status) {
	JIGTEST_LOCK();
	switch (type) {
	case UART_UI_RES_BLE_RESET:
	jigtest_result.ble_reset=status;
		break;
	case UART_UI_RES_BLE_TXRX:
	jigtest_result.ble_tx_rx=status;
		break;
	case UART_UI_RES_GPS_RESET:
	jigtest_result.gps_reset=status;
		break;
	case UART_UI_RES_GPS_TXRX:
	jigtest_result.gps_tx_rx=status;
		break;
	case UART_UI_RES_LTE_RESET:
	jigtest_result.lte_reset=status;
		break;
	case UART_UI_RES_LTE_TXRX:
	jigtest_result.lte_tx_rx=status;
		break;
	case UART_UI_RES_ESP_ADAPTOR:
	jigtest_result.esp_adaptor=status;
		break;
	case UART_UI_RES_LED_HEAD:
	jigtest_result.headlight=status;
		break;
	case UART_UI_RES_LED_RED:
	jigtest_result.sidelight_red=status;
		break;
	case UART_UI_RES_LED_GREEN:
	jigtest_result.sidelight_green=status;
		break;
	case UART_UI_RES_LED_BLUE:
	jigtest_result.sidelight_blue=status;
		break;
	case UART_UI_RES_LOCKPIN:
	jigtest_result.lockpin=status;
		break;
	case UART_UI_RES_IMU_TEST:
	jigtest_result.imu_test=status;
		break;
	case UART_UI_RES_IMU_TRIGGER:
	jigtest_result.imu_trigger=status;
		break;
	case UART_UI_RES_EXTERNAL_FLASH:
	jigtest_result.external_flash=status;
		break;
	//function
	case UART_UI_RES_LTE_2G:
	jigtest_result.gprs_register=status;
		break;
	case UART_UI_RES_LTE_4G:
	jigtest_result.lte_register=status;
		break;
	case UART_UI_RES_MQTT_TEST:
	jigtest_result.mqtt_test=status;
		break;
	case UART_UI_RES_LTE_RSSI:
	jigtest_result.lte_csq=status;
		break;
	case UART_UI_RES_LTE_KEY:
	jigtest_result.lte_key=status;
		break;
	case UART_UI_RES_BLE_RSSI:
	jigtest_result.ble_rssi=status;
		break;
	case UART_UI_RES_BLE_DFU:
	jigtest_result.ble_fota=status;
		break;
	case UART_UI_RES_GPS_POSITION:
	jigtest_result.gps_position=status;
		break;
	case UART_UI_RES_GPS_HDOP:
	jigtest_result.gps_hdop=status;
		break;
	case UART_UI_RES_BLE_TRANSCEIVER:
	jigtest_result.ble_transceiver=status;
		break;
	case UART_UI_RES_BLE_CONNECT:
	jigtest_result.ble_connect=status;
		break;
	default:
		break;
	}
	JIGTEST_UNLOCK();
}

void imu_motion_detected(void)
{
//	if(!checklist.imu_trigger.reported)
//		jigtest_direct_report(UART_UI_RES_IMU_TRIGGER, 1);
}

typedef struct{
	task_init_t init;
	task_process_t process;
	void *params;
}task_t;

static jigtest_timer_t jigtest_timer;

static task_t hardware_check_list[]={
		{.init=jigtest_ble_hardware_test_init, .process=jigtest_ble_hardware_test_process, .params=NULL},
		{.init=jigtest_esp_test_init, .process=jigtest_esp_test_process, .params=NULL},
		{.init=jigtest_gps_hardware_init, .process=jigtest_gps_hardware_process, .params=NULL},
};

static task_t function_check_list[]={
//		{.init=jigtest_ble_function_test_init, .process=jigtest_ble_function_test_process, .params=NULL},
		{.init=jigtest_gps_function_init, .process=jigtest_gps_function_process, .params=&jigtest_timer},
};

static struct{
	uint8_t status;
	task_t *task;
	int count;
	int idx;
	__IO bool complete;
}task;

void jigtest_init()
{
	memset(&jigtest_result, 0, sizeof(jigtest_result));
}

void task_complete_callback()
{
	task.complete=true;
}

void jigtest_process()
{
	if(task.status==JIGTEST_TESTING_HARDWARE || task.status == JIGTEST_TESTING_FUNCTION)
	{
		task.task[task.idx].process();
		if(task.complete)
		{
			task.complete=false;
			task.idx++;
			if(task.idx==task.count)
			{
				task.status=JIGTEST_IDLE;
			}
			else
			{
				task.task[task.idx].init(task_complete_callback, task.task[task.idx].params);
			}
		}
	}
}

void jigtest_command_response(uint8_t cmd, uint8_t status)
{
	uart_ui_comm_send(cmd, &status, 1);
}

void jigtest_test_all_function()
{
	if(task.status==JIGTEST_IDLE)
	{
		jigtest_init();
		task.status=JIGTEST_TESTING_FUNCTION;
		task.task=function_check_list;
		task.count=sizeof(function_check_list)/sizeof(task_t);
		task.idx=0;
		task.complete=false;
		task.task[0].init(task_complete_callback, task.task[task.idx].params);
		jigtest_command_response(UART_UI_CMD_TEST_FUNCTION, UART_UI_RES_OK);
		jigtest_timer.tick=millis();
		jigtest_timer.timeout=180000;
	}
	else if(task.status==JIGTEST_TESTING_HARDWARE)
	{
		jigtest_command_response(UART_UI_CMD_TEST_FUNCTION, UART_UI_RES_OK);
	}
	else
	{
		jigtest_command_response(UART_UI_CMD_TEST_FUNCTION, UART_UI_RES_ERR);
	}
//	jigtest_ble_function_test();
//	jigtest_lte_function_test();
////	int timeout = 120000 - checklist.tick;
////	if (timeout < 10000)
////		timeout = 10000;
////	jigtest_gps_function_test(timeout);
//	jigtest_direct_report(UART_UI_RES_GPS_POSITION, 1);
//	float hdop_value=1;
//	jigtest_report(UART_UI_RES_GPS_HDOP, (uint8_t *)&hdop_value, sizeof(float));
//	jigtest_direct_report(UART_UI_CMD_TEST_FUNCTION, UART_UI_RES_OK);
}

void jigtest_test_all_hardware()
{
	if(task.status==JIGTEST_IDLE)
	{
		jigtest_init();
		task.status=JIGTEST_TESTING_HARDWARE;
		task.task=hardware_check_list;
		task.count=sizeof(hardware_check_list)/sizeof(task_t);
		task.idx=0;
		task.complete=false;
		task.task[0].init(task_complete_callback, task.task[task.idx].params);
		jigtest_command_response(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_OK);
	}
	else if(task.status==JIGTEST_TESTING_HARDWARE)
	{
		jigtest_command_response(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_OK);
	}
	else
	{
		jigtest_command_response(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_ERR);
	}
//	if (app_imu_init(5))
//	{
//		jigtest_direct_report(UART_UI_RES_IMU_TEST, 1);
//		app_imu_register_callback(imu_motion_detected);
//	}
//	buzzer_beepbeep(3, 100, true);
////	memset(&checklist, 0, sizeof(jigtest_checklist_t));
//	jigtest_direct_report(UART_UI_RES_EXTERNAL_FLASH,
//			GD25Q16_test(fotaCoreBuff, 4096));
//	if (jigtest_test_uart_esp())
//	{
//		jigtest_test_io();
//	}
//	else
//	{
//		jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 0);
//	}
//	{
//		bool txrx, reset;
//		testkit_gps_test_hardware(&txrx, &reset);
//		jigtest_direct_report(UART_UI_RES_GPS_TXRX, txrx);
//		jigtest_direct_report(UART_UI_RES_GPS_RESET, reset);
//	}
//	{
//		jigtest_ble_hardware_test();
//	}
//	if (jigtest_lte_test_hardware()) {
//		jigtest_lte_check_key();
//		jigtest_lte_get_info();
//	}
//	uint32_t tick=millis();
//	while(millis()-tick<2000  && !checklist.imu_trigger.reported)
//	{
//		app_imu_process();
//		delay(10);
//	}
//	jigtest_direct_report(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_OK);
}


void hardware_response_handle()
{
	uint8_t count=0;
	uart_ui_comm_send( UART_UI_RES_BLE_RESET, &jigtest_result.ble_reset, 1); count++;
	uart_ui_comm_send( UART_UI_RES_BLE_TXRX, &jigtest_result.ble_tx_rx, 1); count++;
	uart_ui_comm_send( UART_UI_RES_GPS_RESET, &jigtest_result.gps_reset, 1); count++;
	uart_ui_comm_send( UART_UI_RES_GPS_TXRX,	&jigtest_result.gps_tx_rx, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LTE_RESET,	&jigtest_result.lte_reset, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LTE_TXRX,	&jigtest_result.lte_tx_rx, 1); count++;
	uart_ui_comm_send( UART_UI_RES_ESP_ADAPTOR,	&jigtest_result.esp_adaptor, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LED_HEAD,	&jigtest_result.headlight, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LED_RED,	&jigtest_result.sidelight_red, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LED_GREEN,	&jigtest_result.sidelight_green, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LED_BLUE,	&jigtest_result.sidelight_blue, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LOCKPIN,	&jigtest_result.lockpin, 1); count++;
	uart_ui_comm_send( UART_UI_RES_IMU_TEST,	&jigtest_result.imu_test, 1); count++;
	uart_ui_comm_send( UART_UI_RES_IMU_TRIGGER,	&jigtest_result.imu_trigger, 1); count++;
	uart_ui_comm_send( UART_UI_RES_EXTERNAL_FLASH,	&jigtest_result.external_flash, 1); count++;
	if(strlen(jigtest_result.lte_imei)>0)
	{
		uart_ui_comm_send(UART_UI_RES_LTE_IMEI, (uint8_t *)jigtest_result.lte_imei, strlen(jigtest_result.lte_imei)); count++;
	}
	if(strlen(jigtest_result.lte_ccid)>0)
	{
		uart_ui_comm_send(UART_UI_RES_LTE_SIM_CCID, (uint8_t *)jigtest_result.lte_ccid, strlen(jigtest_result.lte_ccid)); count++;
	}
 	jigtest_command_response(UART_UI_RES_RESULT_COUNT, count);
}

void function_response_handle()
{
	uint8_t count=0;
	uart_ui_comm_send( UART_UI_RES_LTE_2G, &jigtest_result.gprs_register, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LTE_4G, &jigtest_result.lte_register, 1); count++;
	uart_ui_comm_send( UART_UI_RES_MQTT_TEST, &jigtest_result.mqtt_test, 1); count++;
	uart_ui_comm_send( UART_UI_RES_LTE_RSSI, &jigtest_result.lte_csq, 1); count++;
	if(strlen(jigtest_result.lte_carrier)>0)
	{
		uart_ui_comm_send( UART_UI_RES_LTE_CARRIER, (uint8_t *)jigtest_result.lte_carrier, 1); count++;
	}
	uart_ui_comm_send( UART_UI_RES_LTE_KEY, &jigtest_result.lte_key, 1); count++;
	if(strlen(jigtest_result.ble_mac)>0)
	{
		uart_ui_comm_send(UART_UI_RES_BLE_MAC, (uint8_t *)jigtest_result.ble_mac, strlen(jigtest_result.ble_mac)); count++;
	}
	uart_ui_comm_send( UART_UI_RES_BLE_TRANSCEIVER, &jigtest_result.ble_transceiver, 1); count++;
	uart_ui_comm_send( UART_UI_RES_BLE_CONNECT, &jigtest_result.ble_connect, 1); count++;
	uart_ui_comm_send( UART_UI_RES_BLE_RSSI, &jigtest_result.ble_rssi, 1); count++;
	uart_ui_comm_send( UART_UI_RES_BLE_DFU, &jigtest_result.ble_fota, 1); count++;
	uart_ui_comm_send( UART_UI_RES_GPS_POSITION, &jigtest_result.gps_position, 1); count++;
	uart_ui_comm_send( UART_UI_RES_GPS_HDOP, &jigtest_result.gps_hdop, 1); count++;
 	jigtest_command_response(UART_UI_RES_RESULT_COUNT, count);
}

void jigtest_cmd_polling_handle(uint8_t type)
{
	if(task.status!=JIGTEST_IDLE)
	{
		jigtest_command_response(UART_UI_RES_RESULT_COUNT, 0);
	}
	else if(type==UART_UI_CMD_TEST_ALL_HW)
	{
		hardware_response_handle();
	}
	else if(type==UART_UI_CMD_TEST_FUNCTION)
	{
		function_response_handle();
	}
}

void jigtest_cmd_handle(uint8_t *frame, size_t size)
{
	switch(frame[0])
	{
	case UART_UI_CMD_TEST_ALL_HW:
		jigtest_test_all_hardware();
		break;
	case UART_UI_CMD_TEST_FUNCTION:
		jigtest_test_all_function();
		break;
	case UART_UI_CMD_POLLING:
		jigtest_cmd_polling_handle(frame[1]);
		break;
	case UART_UI_CMD_FACTORY_RESET:
		uart_ui_comm_command_send(UART_UI_CMD_FACTORY_RESET, UART_UI_RES_OK);
		//factory_reset();
		break;
	case UART_UI_CMD_PING:
		uart_ui_comm_command_send(UART_UI_RES_PING, UART_UI_RES_OK);
		break;

	// to import lte cert + key, both length < 2048 byte, first half fotaCoreBuff for cert, second half for key
	// frame[1]:idx
	//data len: size-2;
	case UART_UI_LTE_CERT:
		lte_cert_handle(UART_UI_LTE_CERT, frame[1], frame +2, size-2);
		break;
	case UART_UI_LTE_KEY:
		lte_cert_handle(UART_UI_LTE_KEY, frame[1], frame+2, size-2);
		break;
	case UART_UI_CMD_IMPORT_KEY:
		cmd_import_key_handle();
		break;
	}
}

void jigtest_console_handle(char *result) {
//	if (__check_cmd("ble ")) {
//		jigtest_ble_console_handle(__param_pos("ble "));
//	} else if (__check_cmd("gps ")) {
//		jigtest_gps_console_handle(__param_pos("gps "));
//	} else if (__check_cmd("lte ")) {
//		jigtest_lte_console_handle(__param_pos("lte "));
//	} else if (__check_cmd("io ")) {
//		jigtest_io_console_handle(__param_pos("io "));
//	} else if (__check_cmd("esp ")) {
//		jigtest_esp_console_handle(__param_pos("esp "));
//	} else if (__check_cmd("info ")) {
//		app_info_console_handle(__param_pos("info "));
//	} else if (__check_cmd("other")) {
//		//jigtest_test_ext_flash();
//		jigtest_test_uart_display();
//	}
}
