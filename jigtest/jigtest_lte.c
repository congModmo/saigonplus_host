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
#include "jigtest_lte.h"
#include "jigtest.h"
#include "app_mqtt/mqttTask.h"
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

bool jigtest_lte_check_key()
{
	char *response;
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=3\r\n", "OK", 500, 2, &response), false, "AT+USECMNG");
	ASSERT_RET(strstr(response, "TrustCA")!=NULL, false, "TrustCA");
	ASSERT_RET(strstr(response, "DeviceCert")!=NULL, false, "DeviceCert");
	ASSERT_RET(strstr(response, "DeviceKey")!=NULL, false, "DeviceKey");
	jigtest_direct_report(UART_UI_RES_LTE_KEY, 1);
	return true;
}

bool jigtest_lte_remove_key()
{
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,0,\"TrustCA\"\r\n", "OK", 500, 2, NULL), false, "Remove trustCa");
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,1,\"DeviceCert\"\r\n", "OK", 500, 2, NULL), false, "Remove cert");
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,2,\"DeviceKey\"\r\n", "OK", 500, 2, NULL), false, "Remove key");
	jigtest_direct_report(UART_UI_RES_LTE_KEY, 0);
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

bool jigtest_lte_function_test()
{
	extern __IO bool system_ready;
	system_ready=true;
	mqtt_test_done=false;
	uint32_t tick=millis();
	while(millis() -tick <180000 &&!mqtt_test_done)
	{
		delay(100);
	}
	return true;
}
void lte_cert_handle(uint8_t type, uint8_t idx, uint8_t *data, uint8_t data_len){
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

void cmd_import_key_handle()
{
	uint8_t *des=fotaCoreBuff;
	if(!crc32_verify(des+4, strlen(des+4), *((uint32_t *)des))){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Key crc failed\n");
		return;
	}
	des=fotaCoreBuff+2048;
	if(!crc32_verify(des+4, strlen(des+4), *((uint32_t *)des))){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Key crc failed\n");
		return;
	}
	if(!jigtest_lte_import_keys(fotaCoreBuff+4, fotaCoreBuff+4+2048)){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("import failed\n");
		return;
	}
	if(!jigtest_lte_check_key()){
		jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 0);
		debug("Verify failed\n");
		return;
	}
	jigtest_direct_report(UART_UI_CMD_IMPORT_KEY, 1);
}

void jigtest_lte_console_handle(char *result)
{
	if(__check_cmd("test hardware"))
	{
		if(jigtest_lte_test_hardware()){
			debug("Test hardware ok\n");
		}
		else{
			debug("Test hardware error\n");
		}
	}
	else if(__check_cmd("test function"))
	{
		if(jigtest_lte_function_test()){
			debug("Test function ok\n");
		}
		else{
			debug("Test function error\n");
		}
	}
	else if(__check_cmd("get info"))
	{
		if(jigtest_lte_get_info()){
			debug("get info ok\n");
		}
		else{
			debug("get info error\n");
		}
	}
	else if(__check_cmd("test network"))
	{
		if(jigtest_lte_test_network()){
			debug("Test network ok\n");
		}
		else{
			debug("Test network error");
		}
	}
	else if(__check_cmd("test key"))
	{
		if(jigtest_lte_check_key()){
			debug("Key ok\n");
		}
		else{
			debug("Key invalid\n");
		}
	}
	else if(__check_cmd("remove key"))
	{
		if(jigtest_lte_remove_key())
		{
			debug("Key removed\n");
		}
		else{
			error("Key remove error\n");
		}
	}
	else if(__check_cmd("import trustca"))
	{
		if(jigtest_lte_import_trustca())
		{
			debug("Import ok\n");
		}
		else{
			debug("import error\n");
		}
	}
	else if(__check_cmd("get network info"))
	{
		jigtest_report_network_info();
	}
	else debug("Unknown cmd\n");
}
