/*
 * testkit.c
 *
 *  Created on: Jul 5, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include "bsp.h"
#include "app_main/app_main.h"
#include "testkit/uart_ui_comm.h"
#include "app_imu/app_imu.h"
#include <testkit/testkit_esp.h>
#include "app_config.h"

typedef struct{
	uint8_t reported;
	uint8_t result;
}test_t;

typedef struct
{
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
}testkit_checklist_t;

static __IO testkit_checklist_t checklist;

void testkit_report(uint8_t type, uint8_t *data, uint8_t len)
{
	TESTKIT_LOCK();
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
#if TESTKIT_DEBUG==0
	uart_ui_send(type, data, len);
#endif
	TESTKIT_UNLOCK();
}

void testkit_direct_report(uint8_t type, uint8_t status)
{
	TESTKIT_LOCK();
	debug("Test resport: %s %s\n", ui_cmd_type_to_string(type), status ? "OK" : "ERROR");
#if TESTKIT_DEBUG==0
	uart_ui_send(type, &status, 1);
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
	TESTKIT_UNLOCK();
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

void testkit_lte_hardware_test()
{
#ifdef LTE_ENABLE
	mail_direct_command(MAIL_LTE_NETWORK_START, lteMailHandle);
#else
	testkit_direct_report(UART_UI_RES_LTE_TXRX, 1);
	testkit_direct_report(UART_UI_RES_LTE_RESET, 1);
	testkit_report(UART_UI_RES_LTE_IMEI, bikeMainProps.imei, strlen(bikeMainProps.imei));
	testkit_report(UART_UI_RES_LTE_SIM_CCID, bikeMainProps.ccid, strlen(bikeMainProps.ccid));
#endif
}

void testkit_lte_function_test(){
#ifdef LTE_ENABLE
	checklist.mqtt.reported=0;
	checklist.mqtt.result=0;
	uint32_t tick=millis();
	while(millis()-tick<120000){
		if(network_is_ready()){
			break;
		}
		delay(5);
	}
	if(network_is_ready()){
		//mail_direct_command(MAIL_MQTT_TEST, mqttMailHandle);
		while(checklist.mqtt.reported==0){
			delay(10);
		}
	}
#else
	testkit_direct_report(UART_UI_RES_LTE_2G, 0);
	testkit_direct_report(UART_UI_RES_LTE_4G, 0);
	testkit_direct_report(UART_UI_RES_MQTT_TEST, 0);
#endif
}

void testkit_test_all_hardware()
{
	uint32_t tick=millis();
//	memset(bikeMainProps.mac, 0, sizeof(bikeMainProps.mac));
//	memset(&checklist, 0, sizeof(testkit_checklist_t));
//	testkit_test_ext_flash();
//	testkit_test_uart_esp();
////	if(checklist.espAdaptor.result==1){
////		testkit_test_io();
////	}
//	testkit_ble_hardware_test();
//	testkit_gps_hardware_test();
//	testkit_test_imu();
	testkit_lte_hardware_test();
//	testkit_lte_function_test();
//	testkit_gps_function_test();
	while(!checklist.lte_2g.reported && !checklist.lte_4g.reported && millis()-tick<60000){
		delay(100);
	}
//	testkit_direct_report(UART_UI_RES_LTE_KEY, 0);
	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_ALL_HW);
}

void testkit_test_gps()
{
	//testkit_test_gps_hardware();
//	testkit_gps_hardware_test();
//	if(checklist.gps_reset.result && checklist.gps_txrx.result){
//		testkit_gps_function_test();
//	}
	testkit_gps_function_test();
	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_GPS);
}

void testkit_test_imu(){
	if(app_imu_init()){
		testkit_direct_report(UART_UI_RES_IMU_TEST, 1);
		testkit_direct_report(UART_UI_RES_IMU_TRIGGER, 1);
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_7)
    {
    	return;
    }
}

void testkit_test_ext_flash(){
	if(GD25Q16_test(fotaCoreBuff, 4096)){
		testkit_direct_report(UART_UI_RES_EXTERNAL_FLASH, 1);
	}
	else{
		testkit_direct_report(UART_UI_RES_EXTERNAL_FLASH, 0);
	}
}

void testkit_test_lte()
{
	testkit_lte_hardware_test();
	//testkit_lte_function_test();
	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_LTE);
}

void testkit_gps_function_test()
{
#ifdef GPS_ENABLE
	uint32_t tick=millis();
	GPS_EVA_ringbuf_reset();
	checklist.gps_position.reported=0;
	while(millis()-tick<30000){
		GPS_EVA_process();
		if(checklist.gps_position.reported){
			break;
		}
		delay(5);
	}
//	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_GPS);
#else
	testkit_direct_report(UART_UI_RES_GPS_POSITION, 0);
	float hdop=0;
	testkit_report(UART_UI_RES_GPS_HDOP, &hdop, sizeof(float));
#endif
}

void testkit_test_ble(){
//	testkit_ble_hardware_test();
//	if(checklist.ble_reset.result & checklist.ble_txrx.result){
		testkit_ble_function_test();
//	}
	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_BLE);
}

void testkit_console_handle(char *result)
{
	if (__check_cmd("ble"))
	{
		testkit_test_ble();
	}
	else if (__check_cmd("gps"))
	{
		testkit_test_gps();
	}
	else if (__check_cmd("lte"))
	{
		testkit_test_lte();
	}
	else if (__check_cmd("io"))
	{
//		testkit_test_io();
		testkit_test_imu();
	}
	else if(__check_cmd("other")){
		//testkit_test_ext_flash();
//		testkit_test_uart_display();
	}
}