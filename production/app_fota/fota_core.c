/*
 * fota_core.c
 *
 *  Created on: Apr 13, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include "fota_core.h"
#include <string.h>
#include <stdlib.h>
#include "fota_comm.h"
#include "log_sys.h"
#include "ext_flash/GD25Q16.h"
#include "ble_dfu.h"
#include "flash_fota.h"
#include "cJSON/cJSON.h"
#include "crc/crc32.h"
#include "app_config.h"
#include "app_main/app_info.h"

enum
{
	FOTA_CORE_IDLE,
	FOTA_CORE_REQUEST_FILE,
	FOTA_CORE_WAIT_FILE,
	FOTA_CORE_PROCESS_FILE,
	FOTA_CORE_GET_FILE_ERROR,
	FOTA_CORE_START_DFU,
	FOTA_CORE_EXIT,
};

typedef struct
{
	uint8_t state;
	uint8_t source;
	bool result;
	const fota_transport_t *transport;
	fota_callback_t callback;
	uint32_t available_flash_addr;
	fota_file_info_t *requesting_file;
	fota_file_info_t json_info;
	fota_file_info_t host_app;
	uint16_t host_app_version;
	fota_file_info_t ble_init;
	fota_file_info_t ble_app;
	uint16_t ble_app_version;
	void *params;
} fota_core_t;

static fota_core_t fota;
uint8_t fotaCoreBuff[FLASH_FOTA_SECTOR_SZ];

static void tranport_err_callback_handle(int err_num)
{
	error("Transport error cb\n");
	fota.callback(fota.transport->source, false);
	fota.state = FOTA_CORE_IDLE;
	fota.transport->signal(false);
}

static void transport_get_file_cb(bool status)
{
	if (fota.state == FOTA_CORE_WAIT_FILE)
	{
		if (status)
		{
			fota.state = FOTA_CORE_PROCESS_FILE;
			info("Get file ok\n");
		}
		else
		{
			fota.state = FOTA_CORE_GET_FILE_ERROR;
			error("Get file error\n");
			fota.transport->signal(false);
		}
	}
	else
	{
		error("It should not go here \n");
	}
}

void fota_dispose()
{
	if (fota.params != NULL)
	{
		free(fota.params);
		fota.params = NULL;
	}
}

bool fota_check_fota_request(fota_request_msg_t *request)
{
	GD25Q16_ReadSector(EX_FLASH_FOTA_REQ_ADDR, (uint8_t *)request, sizeof(fota_request_msg_t));
	if (strcmp((char *)request->signature, VALID_FOTA_REQ_FLAG) != 0)
	{
		return false;
	}
	GD25Q16_SectorErase(EX_FLASH_FOTA_REQ_ADDR);
	return true;
}

static bool parse_json_info(char *json_string)
{
	bool status = false;
	cJSON *request = NULL;
	//	ASSERT_RET((fota.json_info.len)<FLASH_FOTA_SECTOR_SZ, false, "Fota json len");
	//
	ASSERT_GO((request = cJSON_Parse(json_string)) != NULL, "parse request");
	cJSON *hardwareVersion = cJSON_GetObjectItem(request, "hardwareVersion");
	ASSERT_GO(cJSON_IsNumber(hardwareVersion), "Parse hardwareVersion");
	//	ASSERT_GO(hardwareVersion->valueint==factory_config->hardwareVersion, "Hardware version mismatch");
	cJSON *host_app = cJSON_GetObjectItem(request, "hostApp");
	cJSON *ble_app = cJSON_GetObjectItem(request, "bleApp");
	ASSERT_GO(cJSON_IsObject(host_app) || cJSON_IsObject(ble_app), "empty request");
	if (cJSON_IsObject(host_app))
	{
		cJSON *version = cJSON_GetObjectItem(host_app, "version");
		cJSON *len = cJSON_GetObjectItem(host_app, "len");
		cJSON *crc = cJSON_GetObjectItem(host_app, "crc");
		ASSERT_GO(cJSON_IsNumber(version) && cJSON_IsNumber(len) && cJSON_IsNumber(crc), "invalid params");
		fota.host_app_version = version->valueint;
		fota.host_app.len = len->valueint;
		fota.host_app.crc = (uint32_t)crc->valuedouble;
		strcpy(fota.host_app.file_name, "hostApp.bin");
		debug("Host app_version: %d\n", fota.host_app_version);
		debug("Host app len: %d\n", fota.host_app.len);
		debug("Host app crc: %u\n", fota.host_app.crc);
	}
	else
	{
		memset(fota.host_app.file_name, 0, sizeof(fota.host_app.file_name));
		fota.host_app_version = 0;
	}
	if (cJSON_IsObject(ble_app))
	{
		cJSON *version = cJSON_GetObjectItem(ble_app, "version");
		cJSON *init_len = cJSON_GetObjectItem(ble_app, "initLen");
		cJSON *init_crc = cJSON_GetObjectItem(ble_app, "initCrc");
		cJSON *app_len = cJSON_GetObjectItem(ble_app, "appLen");
		cJSON *app_crc = cJSON_GetObjectItem(ble_app, "appCrc");
		ASSERT_GO(cJSON_IsNumber(version) && cJSON_IsNumber(init_len) && cJSON_IsNumber(init_crc) && cJSON_IsNumber(app_len) && cJSON_IsNumber(app_crc), "invalid params");
		fota.ble_app_version = version->valueint;
		fota.ble_init.len = init_len->valueint;
		fota.ble_init.crc = (uint32_t)init_crc->valuedouble;
		fota.ble_app.len = app_len->valueint;
		fota.ble_app.crc = (uint32_t)app_crc->valuedouble;
		strcpy(fota.ble_app.file_name, "bleApp.bin");
		strcpy(fota.ble_init.file_name, "bleApp.dat");
		debug("ble app version: %d\n", fota.ble_app_version);
		debug("ble init len :%d\n", fota.ble_init.len);
		debug("ble init crc: %u\n", fota.ble_init.crc);
		debug("ble app len: %d\n", fota.ble_app.len);
		debug("ble app crc: %u\n", fota.ble_app.crc);
	}
	else
	{
		memset(fota.ble_app.file_name, 0, sizeof(fota.ble_app.file_name));
		memset(fota.ble_init.file_name, 0, sizeof(fota.ble_init.file_name));
		fota.ble_app_version = 0;
	}
	status = true;
__exit:
	cJSON_Delete(request);
	return status;
}

void fota_core_init(const fota_transport_t *transport, fota_callback_t cb, fota_file_info_t *info_json, void *params)
{
	fota.state = FOTA_CORE_IDLE;
	fota.result = false;
	if (fota.json_info.len >= FLASH_FOTA_SECTOR_SZ)
	{
		error("info file too long\n");
		return;
	}
	fota.transport = transport;
	fota.callback = cb;
	fota.params = params;
	if (!fota.transport->init(fotaCoreBuff, FLASH_FOTA_SECTOR_SZ, params, tranport_err_callback_handle))
	{
		error("transport init error\n");
		fota.state = FOTA_CORE_EXIT;
		return;
	}
	// memcpy(&fota.json_info, info_json, sizeof(fota_file_info_t));
	fota.json_info.crc = info_json->crc;
	fota.json_info.len = info_json->len;
	strcpy(fota.json_info.file_name, "info.json");
	fota.available_flash_addr = EX_FLASH_FOTA_FILES_START;
	fota.state = FOTA_CORE_REQUEST_FILE;
	fota.requesting_file = &fota.json_info;
}

bool fota_core_flash_ble(void)
{
	ble_dfu_init(&fota.ble_init, &fota.ble_app);
	return ble_dfu_process();
}

bool host_app_dfu_start()
{
	static host_app_fota_info_t info;
	strcpy(info.signature, VALID_FOTA_FW_FLAG);
	strcpy(info.fileName, fota.host_app.file_name);
	info.source=fota.transport->source;
	info.len=fota.host_app.len;
	info.crc=fota.host_app.crc;
	info.flash_addr=fota.host_app.flash_addr;
	if(!GD25Q16_WriteAndVerifySector(EX_FLASH_APP_INFO_ADDR, (uint8_t *)&info, sizeof(host_app_fota_info_t))){
		error("Write flash\n");
		return false;
	}
	return true;
}

void fota_core_process()
{
	if (fota.state == FOTA_CORE_IDLE)
		return;
	fota.transport->process();
	if (fota.state == FOTA_CORE_PROCESS_FILE)
	{
		if (fota.requesting_file == &fota.json_info)
		{
			info("New info string comming\n");
			GD25Q16_ReadSector(fota.json_info.flash_addr, fotaCoreBuff, fota.json_info.len);
			ASSERT_RET(crc32_verify((uint8_t *)fotaCoreBuff, fota.json_info.len, fota.json_info.crc), false, "read flash");
			fotaCoreBuff[fota.json_info.len] = 0;
			if (!parse_json_info((char *)fotaCoreBuff))
			{
				error("Parse info\n");
				fota.state = FOTA_CORE_IDLE;
				fota_dispose();
				fota.callback(fota.transport->source, false);
			}
			if (fota.host_app_version > 0)
			{
				fota.requesting_file = &fota.host_app;
			}
			else if (fota.ble_app_version > 0)
			{
				fota.requesting_file = &fota.ble_init;
			}
			fota.state = FOTA_CORE_REQUEST_FILE;
		}
		else if (fota.requesting_file == &fota.host_app)
		{
			if (fota.ble_app_version > 0)
			{
				fota.requesting_file = &fota.ble_init;
				fota.state = FOTA_CORE_REQUEST_FILE;
			}
			else
			{
				fota.state = FOTA_CORE_START_DFU;
			}
		}
		else if (fota.requesting_file == &fota.ble_init)
		{
			fota.requesting_file = &fota.ble_app;
			fota.state = FOTA_CORE_REQUEST_FILE;
		}
		else if (fota.requesting_file == &fota.ble_app)
		{
			fota.state = FOTA_CORE_START_DFU;
		}
	}
	if (fota.state == FOTA_CORE_REQUEST_FILE)
	{
		info("Start get %s file\n", fota.requesting_file->file_name) ;
		fota.requesting_file->flash_addr = fota.available_flash_addr;
		if (!fota.transport->get_file(fota.requesting_file, transport_get_file_cb))
		{
			fota.state = FOTA_CORE_EXIT;
		}
		else
		{
			fota.available_flash_addr += ((fota.requesting_file->len / FLASH_FOTA_SECTOR_SZ) + 1) * FLASH_FOTA_SECTOR_SZ;
			fota.state = FOTA_CORE_WAIT_FILE;
		}
	}
	if (fota.state == FOTA_CORE_START_DFU)
	{
		//from here mcu just complete firmware file transfering process,
		//I assume that firmware dfu process is 100% reliable,
		//TODO call signal dfu source (app, server) after dfu process.
		fota.result=true;
		fota.transport->signal(fota.result);

		fota.state = FOTA_CORE_IDLE;
		if (fota.ble_app_version > 0)
		{
#ifdef BLE_ENABLE
			if (!fota_core_flash_ble())
			{
				error("Ble app dfu fail, lam sao gio choi\n");
				goto __exit;
			}
#else
			goto __exit;
#endif
		}
		if (fota.host_app_version > 0)
		{
			if (!host_app_dfu_start())
			{
				error("Host app dfu fail\n");
				goto __exit;
			}
		}
	__exit:
		fota.state=FOTA_CORE_EXIT;
	}
	if (fota.state == FOTA_CORE_EXIT)
	{
		fota.callback(fota.transport->source, fota.result);
		fota_dispose();
		fota.state=FOTA_CORE_IDLE;
		if(fota.result)
		{
			app_info_update_firmware_version(fota.host_app_version, fota.ble_app_version);
			if(fota.host_app_version > 0)
			{
				debug("restart to update host app\n");
				delay(1000);
				NVIC_SystemReset();
			}
		}
	}
}

/******************************************************************************
 * Unit test
 ******************************************************************************/

bool TEST_parse_json_info()
{
	char full_update[] = "{\
	    \"hardwareVersion\": 1,\
	    \"hostApp\": {\
	        \"version\": 2,\
	        \"len\": 345,\
	        \"crc\": 678\
	    },\
	    \"bleApp\": {\
	        \"version\": 2,\
	        \"initLen\": 345,\
	        \"initCrc\": 678,\
	        \"appLen\": 901,\
	        \"appCrc\": 234\
	    }\
	}";
	char host_update[] = "{\
	    \"hardwareVersion\": 1,\
	    \"hostApp\": {\
	        \"version\": 2,\
	        \"len\": 345,\
	        \"crc\": 678\
	    }\
	}";
	char ble_update[] = "{\
	    \"hardwareVersion\": 1,\
	    \"bleApp\": {\
	        \"version\": 2,\
	        \"initLen\": 345,\
	        \"initCrc\": 678,\
	        \"appLen\": 901,\
	        \"appCrc\": 234\
	    }\
	}";
	//full update
	ASSERT_RET(parse_json_info(full_update), false, "parse full update fail");
	ASSERT_RET(fota.host_app_version == 2, false, "parse host app version");
	ASSERT_RET(fota.host_app.len == 345, false, "parse host app len");
	ASSERT_RET(fota.host_app.crc == 678, false, "parse host app crc");
	ASSERT_RET(strcmp(fota.host_app.file_name, "hostApp.bin") == 0, false, "parse host name");
	ASSERT_RET(fota.ble_app_version == 2, false, "parse ble app version");
	ASSERT_RET(fota.ble_init.len == 345, false, "parse ble init len");
	ASSERT_RET(fota.ble_init.crc == 678, false, "parse ble init crc");
	ASSERT_RET(fota.ble_app.len == 901, false, "parse ble app len");
	ASSERT_RET(fota.ble_app.crc == 234, false, "parse ble app crc");
	ASSERT_RET(strcmp(fota.ble_init.file_name, "bleApp.dat") == 0, false, "parse ble init name");
	ASSERT_RET(strcmp(fota.ble_app.file_name, "bleApp.bin") == 0, false, "parse ble app name");
	//host update
	ASSERT_RET(parse_json_info(host_update), false, "parse host update fail");
	ASSERT_RET(fota.host_app_version == 2, false, "parse host app version");
	ASSERT_RET(fota.host_app.len == 345, false, "parse host app len");
	ASSERT_RET(fota.host_app.crc == 678, false, "parse host app crc");
	ASSERT_RET(strcmp(fota.host_app.file_name, "hostApp.bin") == 0, false, "parse host name");
	ASSERT_RET(strlen(fota.ble_app.file_name) == 0, false, "ble app name should be 0");
	ASSERT_RET(strlen(fota.ble_init.file_name) == 0, false, "ble init name should be 0");
	//
	//ble update
	ASSERT_RET(parse_json_info(ble_update), false, "parse ble update fail");
	ASSERT_RET(fota.ble_app_version == 2, false, "parse ble app version");
	ASSERT_RET(fota.ble_init.len == 345, false, "parse ble init len");
	ASSERT_RET(fota.ble_init.crc == 678, false, "parse ble init crc");
	ASSERT_RET(fota.ble_app.len == 901, false, "parse ble app len");
	ASSERT_RET(fota.ble_app.crc == 234, false, "parse ble app crc");
	ASSERT_RET(strcmp(fota.ble_init.file_name, "bleApp.dat") == 0, false, "parse ble init name");
	ASSERT_RET(strcmp(fota.ble_app.file_name, "bleApp.bin") == 0, false, "parse ble app name");
	ASSERT_RET(strlen(fota.host_app.file_name) == 0, false, "host name should be 0");
	debug("Test passed");
	return true;
}
