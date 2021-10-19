/*
 * jigtest.c
 *
 *  Created on: Jul 5, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include "bsp.h"
#include "app_main/app_main.h"
#include "uart_ui_comm.h"
#include "app_imu/app_imu.h"
#include "jigtest_esp.h"
#include "app_config.h"
#include "jigtest_io.h"
#include "jigtest_ble.h"
#include "jigtest_gps.h"
#include "jigtest_lte.h"

typedef struct{
	uint8_t reported;
	uint8_t result;
}test_t;

typedef struct
{
	uint32_t tick;
	test_t espAdaptor;
	test_t ble_txrx;
	test_t ble_reset;
	test_t lte_txrx;
	test_t lte_reset;
	test_t lte_imei;
	test_t lte_ccid;
	test_t lte_2g;
	test_t lte_4g;
	test_t gps_txrx;
	test_t gps_reset;
	test_t ext_flash;
	test_t led;
	test_t lockPin;
	test_t gps_position;
	test_t gps_hdop;
	test_t mqtt;
}jigtest_checklist_t;

static __IO jigtest_checklist_t checklist;

void jigtest_report(uint8_t type, uint8_t *data, uint8_t len)
{
	JIGTEST_LOCK();
	debug("%s ", ui_cmd_type_to_string(type));
	switch (type)
	{
	case UART_UI_RES_LTE_IMEI:
		debugx("%.*s\n", len, data);
		checklist.lte_imei.reported=1;
		checklist.lte_imei.result= (data!=NULL)?1:0;
		break;
	case UART_UI_RES_LTE_SIM_CCID:
		debugx("%.*s\n", len, data);
		checklist.lte_ccid.reported=1;
		checklist.lte_ccid.result= (data!=NULL)?1:0;
		break;
	case UART_UI_RES_LTE_CARRIER:
		debugx("%.*s\n", len, data);
		break;
	case UART_UI_RES_LTE_RSSI:
		debugx("%d\n", *(int *)data);
		break;
	case UART_UI_RES_BLE_MAC:
		debugx("%.*s\n", len, data);
		break;
	}
#if JIGTEST_DEBUG==0
	uart_ui_comm_send(type, data, len);
#endif
	JIGTEST_UNLOCK();
}

void jigtest_direct_report(uint8_t type, uint8_t status)
{
	JIGTEST_LOCK();
//	debug("Test resport: %s %s\n", ui_cmd_type_to_string(type), status ? "OK" : "ERROR");
#if JIGTEST_DEBUG==0
	uart_ui_comm_send(type, &status, 1);
#endif
	switch(type){
	case UART_UI_RES_ESP_ADAPTOR:
		checklist.espAdaptor.reported=1;
		checklist.espAdaptor.result=status;
		break;
	case UART_UI_RES_LTE_RESET:
		checklist.lte_reset.reported=1;
		checklist.lte_reset.result=status;
		break;
	case UART_UI_RES_LTE_TXRX:
		checklist.lte_txrx.reported=1;
		checklist.lte_txrx.result=status;
		break;
	case UART_UI_RES_LTE_4G:
		checklist.lte_4g.reported=1;
		checklist.lte_4g.result=status;
		break;
	case UART_UI_RES_LTE_2G:
		checklist.lte_2g.reported=1;
		checklist.lte_2g.result=status;
		break;
	case UART_UI_RES_MQTT_TEST:
		checklist.mqtt.reported=1;
		checklist.mqtt.result=status;
		break;
	case UART_UI_RES_GPS_RESET:
		checklist.gps_reset.reported=1;
		checklist.gps_reset.result=status;
		break;
	case UART_UI_RES_GPS_TXRX:
		checklist.gps_txrx.reported=1;
		checklist.gps_txrx.result=status;
		break;
	}
	JIGTEST_UNLOCK();
}

bool mail_direct_command(uint8_t command, osMessageQueueId_t mailBox)
{
	mail_t mail = {.type = command, .data = 0, .len = 0};
	if (osMessageQueuePut(mailBox, &mail, 0, 10) != osOK)
	{
		error("Mail put failed");
		return false;
	}
	return true;
}

//void jigtest_lte_function_test(){
//#ifdef LTE_ENABLE
//	checklist.mqtt.reported=0;
//	checklist.mqtt.result=0;
//	uint32_t tick=millis();
//	while(millis()-tick<120000){
//		if(network_is_ready()){
//			break;
//		}
//		delay(5);
//	}
//	if(network_is_ready()){
//		//mail_direct_command(MAIL_MQTT_TEST, mqttMailHandle);
//		while(checklist.mqtt.reported==0){
//			delay(10);
//		}
//	}
//#else
//	jigtest_direct_report(UART_UI_RES_LTE_2G, 0);
//	jigtest_direct_report(UART_UI_RES_LTE_4G, 0);
//	jigtest_direct_report(UART_UI_RES_MQTT_TEST, 0);
//#endif
//}

void jigtest_test_all_hardware()
{
//	memset(&checklist, 0, sizeof(jigtest_checklist_t));
//	jigtest_direct_report(UART_UI_RES_EXTERNAL_FLASH, GD25Q16_test(fotaCoreBuff, 4096));
//	if(jigtest_test_uart_esp()){
//		jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 1);
//		jigtest_io_result_t *result=jigtest_test_io();
//		jigtest_direct_report(UART_UI_RES_LED_RED, result->red);
//		jigtest_direct_report(UART_UI_RES_LED_GREEN, result->green);
//		jigtest_direct_report(UART_UI_RES_LED_BLUE, result->blue);
//		jigtest_direct_report(UART_UI_RES_LED_HEAD, result->head);
//		jigtest_direct_report(UART_UI_RES_LOCKPIN, result->lock);
//	}
//	else{
//		jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 0);
//	}
	{
		checklist.tick=millis();
		bool txrx, reset;
		testkit_gps_test_hardware(&txrx, &reset);
		jigtest_direct_report(UART_UI_RES_GPS_TXRX, txrx);
		jigtest_direct_report(UART_UI_RES_GPS_RESET, reset);
	}
//	{
//		jigtest_ble_hardware_test();
//	}
//	if(app_imu_init())
//	{
//		jigtest_direct_report(UART_UI_RES_IMU_TEST, 1);
//	}
//	jigtest_direct_report(UART_UI_RES_LTE_KEY, 0);
//	if(jigtest_lte_test_hardware())
//	{
//		jigtest_direct_report(UART_UI_RES_LTE_RESET, 1);
//		jigtest_direct_report(UART_UI_RES_LTE_TXRX, 1);
//		jigtest_direct_report(UART_UI_RES_LTE_KEY, jigtest_lte_check_key());
//		if(jigtest_lte_get_info())
//		{
//			jigtest_report(UART_UI_RES_LTE_IMEI, jigtest_lte_imei, strlen(jigtest_lte_imei));
//			jigtest_report(UART_UI_RES_LTE_SIM_CCID, jigtest_lte_ccid, strlen(jigtest_lte_ccid));
//		}
//	}

	jigtest_direct_report(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_OK);
}

void jigtest_test_all_function()
{
//	jigtest_ble_function_test();
//	jigtest_lte_function_test();
	uint32_t timeout=120000-checklist.tick;
	if(timeout<10000) timeout=10000;
	jigtest_gps_function_test(timeout);
}

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    if(GPIO_Pin == GPIO_PIN_7)
//    {
//    	return;
//    }
//}

//void jigtest_gps_function_test()
//{
//#ifdef GPS_ENABLE
//	uint32_t tick=millis();
////	GPS_EVA_ringbuf_reset();
//	checklist.gps_position.reported=0;
//	while(millis()-tick<30000){
//		GPS_EVA_process();
//		if(checklist.gps_position.reported){
//			break;
//		}
//		delay(5);
//	}
////	jigtest_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_GPS);
//#else
//	jigtest_direct_report(UART_UI_RES_GPS_POSITION, 0);
//	float hdop=0;
//	jigtest_report(UART_UI_RES_GPS_HDOP, &hdop, sizeof(float));
//#endif
//}

void jigtest_console_handle(char *result)
{
	if (__check_cmd("ble "))
	{
		jigtest_ble_console_handle(__param_pos("ble "));
	}
	else if (__check_cmd("gps "))
	{
		jigtest_gps_console_handle(__param_pos("gps "));
	}
	else if (__check_cmd("lte "))
	{
		jigtest_lte_console_handle(__param_pos("lte "));
	}
	else if (__check_cmd("io "))
	{
		jigtest_io_console_handle(__param_pos("io "));
	}
	else if(__check_cmd("esp ")){
		jigtest_esp_console_handle(__param_pos("esp "));
	}
	else if(__check_cmd("other")){
		//jigtest_test_ext_flash();
//		jigtest_test_uart_display();
	}
}
