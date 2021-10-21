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

bool jigtest_lte_test_hardware()
{
	return lara_r2_hardware_init();
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
	return true;
}

bool jigtest_lte_remove_key()
{
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,0,\"TrustCA\"\r\n", "OK", 500, 2, NULL), false, "Remove trustCa");
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,1,\"DeviceCert\"\r\n", "OK", 500, 2, NULL), false, "Remove cert");
	ASSERT_RET(gsm_send_at_command("AT+USECMNG=2,2,\"DeviceKey\"\r\n", "OK", 500, 2, NULL), false, "Remove key");
	return true;
}

bool jigtest_lte_get_info()
{
	return lara_r2_init_info(jigtest_lte_imei, 32, jigtest_lte_ccid, 32);
}

bool jigtest_lte_test_network()
{
	return network_connect_init();
}

bool jigtest_lte_function_test()
{
	extern __IO bool system_ready;
	system_ready=true;
	uint32_t tick=millis();
	while(millis()-tick<180000 && !network_is_ready())
	{
		delay(5);
	}
	if(!network_is_ready()){
		debug("Network timeout\n");
		return false;
	}
	if(*network_type==NETWORK_TYPE_2G)
	{
		jigtest_direct_report(UART_UI_RES_LTE_2G, 1);
	}
	else if(*network_type==NETWORK_TYPE_4G)
	{
		jigtest_direct_report(UART_UI_RES_LTE_4G, 1);
	}
	tick=millis();
	while(millis()-tick<60000 && !mqtt_is_ready())
	{
		delay(5);
	}
	if(!mqtt_is_ready())
	{
		debug("Mqtt timeout\n");
		return false;
	}
	while(!mqtt_test_done)
	{
		delay(5);
	}
	jigtest_direct_report(UART_UI_RES_MQTT_TEST, mqtt_test_result);
	return true;
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
	else debug("Unknown cmd\n");
}
