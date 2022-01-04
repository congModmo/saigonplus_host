/*
 * jigest_lte.h
 *
 *  Created on: Oct 15, 2021
 *      Author: thanhcong
 */
#define __DEBUG__ 4
#include "lte/lara_r2.h"
#include "bsp.h"
#include "app_lte/lteTask.h"
#include "app_main/app_main.h"
#include "jigtest_lte.h"
#include "jigtest.h"
#include "app_mqtt/mqttTask.h"
#include "crc/crc32.h"
#include "protothread/pt.h"

enum{
	TRUST_CA=0,
	DEVICE_CERT=1,
	DEVICE_KEY=2
}cert_type_t;

char jigtest_lte_imei[32];
char jigtest_lte_ccid[32];
char jigtest_lte_carrier[32];

bool jigtest_lte_test_hardware()
{
	ASSERT_RET(lara_r2_hardware_init(), false, "hardware init");
	jigtest_direct_report(UART_UI_RES_LTE_RESET, 1);
	jigtest_direct_report(UART_UI_RES_LTE_TXRX, 1);
	return true;
}

bool jigtest_lte_import_keys(char *cert, char *private_key)
{
	ASSERT_RET(lara_r2_import_key(aws_trust_ca, "TrustCA", TRUST_CA), false, "Import trust ca");
	ASSERT_RET(lara_r2_import_key(cert, "DeviceCert", DEVICE_CERT), false, "Import certificate");
	ASSERT_RET(lara_r2_import_key(private_key, "DeviceKey", DEVICE_KEY), false, "Import private key");
	return true;
}

bool jigtest_lte_import_trustca()
{
	ASSERT_RET(lara_r2_import_key(aws_trust_ca, "TrustCA", TRUST_CA), false, "Import trust ca");
	return true;
}

bool jigtest_lte_get_info()
{
	if( lara_r2_init_info(jigtest_lte_imei, 32, jigtest_lte_ccid, 32))
	{
		jigtest_report(UART_UI_RES_LTE_IMEI, jigtest_lte_imei,
					strlen(jigtest_lte_imei));
		jigtest_report(UART_UI_RES_LTE_SIM_CCID, jigtest_lte_ccid,
					strlen(jigtest_lte_ccid));
		return true;
	}
	return false;
}

bool jigtest_lte_test_network()
{
	return network_connect_init();
}

/****************************************************************************************
 * HARDWARE TEST
 ***************************************************************************************/
static struct pt hardware_pt;
static task_complete_cb_t hardware_cb;
static void hardware_test_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	static uint32_t tick;
	jigtest_mail_direct_command(MAIL_LTE_INIT_HARDWARE, lteMailHandle);
	PT_DELAY(pt, tick, 1000);
	tick=millis();
	PT_WAIT_UNTIL(pt, millis()-tick >30000 || lte_info->hardware_init);
	if(lte_info->hardware_init)
	{
		jigtest_direct_report(UART_UI_RES_LTE_TXRX, 1);
		jigtest_direct_report(UART_UI_RES_LTE_RESET, 1);
		jigtest_report(UART_UI_RES_LTE_IMEI, (uint8_t *)lte_info->imei, strlen((char *)lte_info->imei)+1);
		jigtest_report(UART_UI_RES_LTE_SIM_CCID, (uint8_t *)lte_info->ccid, strlen((char *)lte_info->ccid)+1);
		jigtest_direct_report(UART_UI_RES_LTE_KEY, lte_info->key);
	}
	hardware_cb();
	PT_END(pt);
}

void jigtest_lte_hardware_init(task_complete_cb_t cb, void *params)
{
	PT_INIT(&hardware_pt);
	hardware_cb=cb;
}

void jigtest_lte_hardware_process()
{
	hardware_test_thread(&hardware_pt);
}
/****************************************************************************************
 * FUNCTION TEST
 ***************************************************************************************/

static struct pt function_pt;
static jigtest_timer_t *timer;
task_complete_cb_t function_callback;

static void function_test_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	static uint32_t tick;
	jigtest_mail_direct_command(MAIL_LTE_INIT_NETWORK, lteMailHandle);
	PT_DELAY(pt, tick, 1000);
	tick=millis();
	PT_WAIT_UNTIL(pt, millis()-tick > 60000 || lte_info->ready);
	if(!lte_info->ready)
	{
		goto __exit;
	}
	jigtest_report(UART_UI_RES_LTE_CARRIER, lte_info->carrier, strlen(lte_info->carrier) +1);
	jigtest_direct_report((lte_info->type==NETWORK_TYPE_2G)?UART_UI_RES_LTE_2G:UART_UI_RES_LTE_4G, 1);
	jigtest_direct_report(UART_UI_RES_LTE_RSSI, lte_info->rssi);
	tick=millis();
	PT_WAIT_UNTIL(pt, millis()-tick > 60000 || mqtt_test_done);
	if(!mqtt_test_done)
	{
		goto __exit;
	}
	jigtest_direct_report(UART_UI_RES_MQTT_TEST, mqtt_test_result?1:0);
	__exit:
	function_callback();
	PT_END(pt);
}

void jigtest_lte_function_init(task_complete_cb_t cb, void *params)
{
	timer=(jigtest_timer_t *)params;
	function_callback=cb;
	PT_INIT(&function_pt);
}

void jigtest_lte_function_process()
{
	function_test_thread(&function_pt);
}
/****************************************************************************************
 * CERT KEY
 ***************************************************************************************/

void jigtest_lte_cert_handle(uint8_t type, uint8_t idx, uint8_t *data, uint8_t data_len){
	uint8_t *des;
	if(type==UART_UI_LTE_CERT){
		des=fotaCoreBuff;
	}
	else{
		des=fotaCoreBuff +2048;
	}
	memcpy(des +idx*128, data, data_len);
	if(data_len!=128){
		des[idx*128+data_len]=0;
	}
}

void jigtest_lte_import_key_handle(void)
{
	uint8_t *des=fotaCoreBuff;
	if(!crc32_verify(des+4, strlen((char *)des+4), *((uint32_t *)des))){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Key crc failed\n");
		return;
	}
	des=fotaCoreBuff+2048;
	if(!crc32_verify(des+4, strlen((char *)des+4), *((uint32_t *)des))){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Key crc failed\n");
		return;
	}
	if(!jigtest_lte_import_keys((char *)fotaCoreBuff+4, (char *)fotaCoreBuff+4+2048)){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("import failed\n");
		return;
	}
	if(!lara_r2_check_key()){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Verify failed\n");
		return;
	}
	jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 1);
}

void jigtest_lte_console_handle(char *result)
{
//	if(__check_cmd("test hardware"))
//	{
//		if(jigtest_lte_test_hardware()){
//			debug("Test hardware ok\n");
//		}
//		else{
//			debug("Test hardware error\n");
//		}
//	}
//	else if(__check_cmd("test function"))
//	{
////		if(jigtest_lte_function_test()){
////			debug("Test function ok\n");
////		}
////		else{
////			debug("Test function error\n");
////		}
//	}
//	else if(__check_cmd("get info"))
//	{
//		if(jigtest_lte_get_info()){
//			debug("get info ok\n");
//		}
//		else{
//			debug("get info error\n");
//		}
//	}
//	else if(__check_cmd("test network"))
//	{
//		if(jigtest_lte_test_network()){
//			debug("Test network ok\n");
//		}
//		else{
//			debug("Test network error");
//		}
//	}
//	else if(__check_cmd("test key"))
//	{
////		if(jigtest_lte_check_key()){
////			debug("Key ok\n");
////		}
////		else{
////			debug("Key invalid\n");
////		}
//	}
	if(__check_cmd("remove key"))
	{
		if(lara_r2_remove_key())
		{
			debug("Key removed\n");
		}
		else{
			error("Key remove error\n");
		}
	}
//	else if(__check_cmd("import trustca"))
//	{
//		if(jigtest_lte_import_trustca())
//		{
//			debug("Import ok\n");
//		}
//		else{
//			debug("import error\n");
//		}
//	}
//	else if(__check_cmd("get network info"))
//	{
////		jigtest_report_network_info();
//	}
	else debug("Unknown cmd\n");
}
