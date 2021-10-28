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
#include "jigtest_esp.h"
#include "app_config.h"
#include "jigtest_io.h"
#include "jigtest_ble.h"
#include "jigtest_gps.h"
#include "jigtest_lte.h"
#include "app_main/uart_ui_comm.h"

typedef struct {
	uint8_t reported;
	uint8_t result;
} test_t;

typedef struct {
	uint32_t tick;
	bool function_test_finish;
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
} jigtest_checklist_t;

static __IO jigtest_checklist_t checklist;

void jigtest_report(uint8_t type, uint8_t *data, uint8_t len) {
	JIGTEST_LOCK();
	debug("%s ", ui_cmd_to_string(type));
	switch (type) {
	case UART_UI_RES_LTE_IMEI:
		debugx("%.*s\n", len, data);
		checklist.lte_imei.reported = 1;
		checklist.lte_imei.result = (data != NULL) ? 1 : 0;
		break;
	case UART_UI_RES_LTE_SIM_CCID:
		debugx("%.*s\n", len, data);
		checklist.lte_ccid.reported = 1;
		checklist.lte_ccid.result = (data != NULL) ? 1 : 0;
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

void jigtest_direct_report(uint8_t type, uint8_t status) {
	JIGTEST_LOCK();
//	debug("Test resport: %s %s\n", ui_cmd_type_to_string(type), status ? "OK" : "ERROR");
#if JIGTEST_DEBUG==0
	uart_ui_comm_send(type, &status, 1);
#endif
	switch (type) {
	case UART_UI_RES_ESP_ADAPTOR:
		checklist.espAdaptor.reported = 1;
		checklist.espAdaptor.result = status;
		break;
	case UART_UI_RES_LTE_RESET:
		checklist.lte_reset.reported = 1;
		checklist.lte_reset.result = status;
		break;
	case UART_UI_RES_LTE_TXRX:
		checklist.lte_txrx.reported = 1;
		checklist.lte_txrx.result = status;
		break;
	case UART_UI_RES_LTE_4G:
		checklist.lte_4g.reported = 1;
		checklist.lte_4g.result = status;
		break;
	case UART_UI_RES_LTE_2G:
		checklist.lte_2g.reported = 1;
		checklist.lte_2g.result = status;
		break;
	case UART_UI_RES_MQTT_TEST:
		checklist.mqtt.reported = 1;
		checklist.mqtt.result = status;
		break;
	case UART_UI_RES_GPS_RESET:
		checklist.gps_reset.reported = 1;
		checklist.gps_reset.result = status;
		break;
	case UART_UI_RES_GPS_TXRX:
		checklist.gps_txrx.reported = 1;
		checklist.gps_txrx.result = status;
		break;
	}
	JIGTEST_UNLOCK();
}

void jigtest_readout_protect() {
	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();
	FLASH_OBProgramInitTypeDef OBInit;
	HAL_FLASHEx_OBGetConfig(&OBInit);
	if (OBInit.RDPLevel == OB_RDP_LEVEL_0) {
		debug("Turning read protected\n");
		/* Enable the read protection **************************************/
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_1;
		if (HAL_FLASHEx_OBProgram(&OBInit) != HAL_OK) {
			/* Error occurred while options bytes programming. **********************/
			jigtest_direct_report(UART_UI_CMD_READOUT_PROTECT, 0);
		}
		/* Generate System Reset to load the new option byte values ***************/
		jigtest_direct_report(UART_UI_CMD_READOUT_PROTECT, 1);
		delay(100);
		//chip will restart here
		HAL_FLASH_OB_Launch();
	} else {
		debug("Already read protected\n");
		jigtest_direct_report(UART_UI_CMD_READOUT_PROTECT, 1);
	}
	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();

}

bool mail_direct_command(uint8_t command, osMessageQueueId_t mailBox) {
	mail_t mail = { .type = command, .data = 0, .len = 0 };
	if (osMessageQueuePut(mailBox, &mail, 0, 10) != osOK) {
		error("Mail put failed");
		return false;
	}
	return true;
}

extern __IO bool imu_test;
void jigtest_test_all_hardware() {
	memset(&checklist, 0, sizeof(jigtest_checklist_t));
	jigtest_direct_report(UART_UI_RES_EXTERNAL_FLASH,
			GD25Q16_test(fotaCoreBuff, 4096));
	if (jigtest_test_uart_esp()) {
		jigtest_test_io();
	}
	else
	{
		jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 0);
	}
	{
		checklist.tick = millis();
		bool txrx, reset;
		testkit_gps_test_hardware(&txrx, &reset);
		jigtest_direct_report(UART_UI_RES_GPS_TXRX, txrx);
		jigtest_direct_report(UART_UI_RES_GPS_RESET, reset);
	}
	{
		jigtest_ble_hardware_test();
	}

	if (app_imu_init()) {
		jigtest_direct_report(UART_UI_RES_IMU_TEST, 1);
	}
	if (jigtest_lte_test_hardware()) {
		jigtest_lte_check_key();
		jigtest_lte_get_info();
	}
	ioctl_beepbeep(3, 100);
	jigtest_direct_report(UART_UI_CMD_TEST_ALL_HW, UART_UI_RES_OK);
}

void imu_motion_detected(void)
{
	jigtest_direct_report(UART_UI_RES_IMU_TRIGGER, 1);
}

void jigtest_test_all_function() {
	jigtest_ble_function_test();
	jigtest_lte_function_test();
	int timeout = 120000 - checklist.tick;
	if (timeout < 10000)
		timeout = 10000;
	jigtest_gps_function_test(timeout);
	imu_test = true;
	app_imu_register_callback(imu_motion_detected);
	jigtest_direct_report(UART_UI_CMD_TEST_FUNCTION, UART_UI_RES_OK);
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
	case UART_UI_CMD_READOUT_PROTECT:
		jigtest_readout_protect();
		break;
	case UART_UI_CMD_TEST_LTE:
//		jigtest_test_lte();
		break;
	case UART_UI_CMD_TEST_BLE:
//		jigtest_ble_function_test();
		break;
	case UART_UI_CMD_TEST_GPS:
//		jigtest_test_gps();
		break;
	case UART_UI_CMD_TEST_IO:
//		jigtest_test_io();
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
	if (__check_cmd("ble ")) {
		jigtest_ble_console_handle(__param_pos("ble "));
	} else if (__check_cmd("gps ")) {
		jigtest_gps_console_handle(__param_pos("gps "));
	} else if (__check_cmd("lte ")) {
		jigtest_lte_console_handle(__param_pos("lte "));
	} else if (__check_cmd("io ")) {
		jigtest_io_console_handle(__param_pos("io "));
	} else if (__check_cmd("esp ")) {
		jigtest_esp_console_handle(__param_pos("esp "));
	} else if (__check_cmd("info ")) {
		app_info_console_handle(__param_pos("info "));
	} else if (__check_cmd("other")) {
		//jigtest_test_ext_flash();
//		jigtest_test_uart_display();
	}
}
