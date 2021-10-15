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

enum{
	TRUST_CA=0,
	DEVICE_CERT=1,
	DEVICE_KEY=2
}cert_type_t;

char jigtest_lte_imei[32];
char jigtest_lte_ccid[32];

bool jigtest_lte_test_hardware()
{
	lara_r2_bsp_init();
	return lara_r2_hardware_init();
}

bool jigtest_lte_import_keys(char *cert, char *private_key)
{
	ASSERT_RET(lara_r2_import_key(aws_trust_ca, "TrustCA", TRUST_CA), false, "Import trust ca");
	ASSERT_RET(lara_r2_import_key(cert, "DeviceCert", DEVICE_CERT), false, "Import certificate");
	ASSERT_RET(lara_r2_import_key(private_key, "DeviceKey", TRUST_CA), false, "Import private key");
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
	else debug("Unknown cmd\n");
}
