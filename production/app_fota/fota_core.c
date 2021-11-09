/*
 * fota_core.c
 *
 *  Created on: Apr 13, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 3
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
#ifdef JIGTEST
#include "jigtest.h"
#endif
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
	uint32_t host_app_version;
	fota_file_info_t ble_init;
	fota_file_info_t ble_app;
	uint32_t ble_app_version;
	fota_status_t ble_status;
	fota_status_t host_status;
	void *params;
} fota_core_t;

static fota_core_t fota;
uint8_t fotaCoreBuff[FLASH_FOTA_SECTOR_SZ];

static void tranport_err_callback_handle(int err_num)
{
	error("Transport error cb\n");
	fota.callback(fota.transport->source, FOTA_NONE, FOTA_NONE);
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

static bool parse_json_as_string(char *json_string)
{
	char *p;
	ASSERT_RET((p=strstr(json_string, "\"hardwareVersion\""))!=NULL, false, "parse hw version");
	uint32_t hardware_version;
	ASSERT_RET(sscanf(p, "\"hardwareVersion\":%lu", &hardware_version)==1, false, "parse hw version");
	ASSERT_RET(hardware_version==factory_config->hardwareVersion, false, "hardware miss match");
	ASSERT_RET((p=strstr(json_string, "\"version\""))!=NULL, false, "parse host version");
	ASSERT_RET(sscanf(p, "\"version\":%lu", &fota.host_app_version)==1, false, "parse host version");
	if(fota.host_app_version >0)
	{
		ASSERT_RET((p=strstr(json_string, "\"len\""))!=NULL, false, "parse len version");
		ASSERT_RET(sscanf(p, "\"len\":%d", &fota.host_app.len)==1, false, "scan host len");
		ASSERT_RET((p=strstr(json_string, "\"crc\""))!=NULL, false, "parse host crc");
		ASSERT_RET(sscanf(p, "\"crc\":%lu", &fota.host_app.crc)==1, false, "scan host crc");
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
	ASSERT_RET((p=strstr(json_string, "\"bleVersion\""))!=NULL, false, "parse ble version");
	ASSERT_RET(sscanf(p, "\"bleVersion\":%lu", &fota.ble_app_version)==1, false, "scan ble version");
	if(fota.ble_app_version>0)
	{
		ASSERT_RET((p=strstr(json_string, "\"initLen\""))!=NULL, false, "parse init len");
		ASSERT_RET(sscanf(p, "\"initLen\":%d", &fota.ble_init.len)==1, false, "scan init len");
		ASSERT_RET((p=strstr(json_string, "\"initCrc\""))!=NULL, false, "parse init crc");
		ASSERT_RET(sscanf(p, "\"initCrc\":%lu", &fota.ble_init.crc)==1, false, "scan init crc");
		ASSERT_RET((p=strstr(json_string, "\"appLen\""))!=NULL, false, "parse ble len");
		ASSERT_RET(sscanf(p, "\"appLen\":%d", &fota.ble_app.len)==1, false, "scan ble len");
		ASSERT_RET((p=strstr(json_string, "\"appCrc\""))!=NULL, false, "parse ble crc");
		ASSERT_RET(sscanf(p, "\"appCrc\":%lu", &fota.ble_app.crc)==1, false, "scan ble crc");
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
	return true;
}

//waring parse json may not work in low ram available.
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
		cJSON *version = cJSON_GetObjectItem(ble_app, "bleVersion");
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

void fota_core_init(const fota_transport_t *transport, fota_callback_t cb, fota_file_info_t *info_json, void *params, uint8_t source)
{
	fota.state = FOTA_CORE_IDLE;
	fota.result = false;
	fota.ble_status=FOTA_NONE;
	fota.host_status=FOTA_NONE;
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
	fota.source=source;
}

bool fota_core_flash_ble(void)
{
	ble_dfu_init(&fota.ble_init, &fota.ble_app);
	return ble_dfu_process();
}

static host_app_fota_info_t host_fota_info;
bool host_app_dfu_start()
{
	strcpy(host_fota_info.signature, VALID_FOTA_FW_FLAG);
	strcpy(host_fota_info.fileName, fota.host_app.file_name);
	host_fota_info.source=fota.source;
	host_fota_info.len=fota.host_app.len;
	host_fota_info.crc=fota.host_app.crc;
	host_fota_info.flash_addr=fota.host_app.flash_addr;
	if(!GD25Q16_WriteAndVerifySector(EX_FLASH_APP_INFO_ADDR, (uint8_t *)&host_fota_info, sizeof(host_app_fota_info_t))){
		error("Write flash\n");
		return false;
	}
	return true;
}

bool host_app_upgrade_form_uart_check()
{
	GD25Q16_ReadSector(EX_FLASH_APP_INFO_ADDR, (uint8_t *)&host_fota_info, sizeof(host_app_fota_info_t));
	if(0==strcmp(host_fota_info.signature, VALID_FOTA_FW_FLAG))
	{
		GD25Q16_SectorErase(EX_FLASH_APP_INFO_ADDR);
		if(host_fota_info.source==FOTA_OVER_UART)
		{
			return true;
		}
	}
	return false;
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
			if (!parse_json_as_string((char *)fotaCoreBuff))
			{
				error("Parse info\n");
				fota.state = FOTA_CORE_IDLE;
				fota_dispose();
				fota.callback(fota.transport->source, FOTA_NONE, FOTA_NONE);
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
		fota.state = FOTA_CORE_IDLE;
		fota.result=true;
		fota.transport->signal(fota.result);
		if (fota.ble_app_version > 0)
		{
#ifdef BLE_ENABLE
			//to let response signal back to gui
			if(fota.source==FOTA_OVER_BLE)
				delay(200);
			if (!fota_core_flash_ble())
			{
				error("Ble app dfu fail, lam sao gio choi\n");
				fota.ble_status=FOTA_FAIL;
				fota.result=false;
				goto __exit;
			}
			fota.ble_status=FOTA_DONE;
			app_info_update_ble_version(fota.ble_app_version);
#else
			goto __exit;
#endif
		}
		if (fota.host_app_version > 0)
		{
			if (!host_app_dfu_start())
			{
				error("Host app dfu fail\n");
				fota.host_status=FOTA_FAIL;
				fota.result=false;
				goto __exit;
			}
			fota.host_status=FOTA_DONE;
			//I assume that host firmware dfu process is 100% reliable,
			app_info_update_host_version(fota.host_app_version);
		}
	__exit:
		fota.state=FOTA_CORE_EXIT;
	}
	if (fota.state == FOTA_CORE_EXIT)
	{
		fota_dispose();
		fota.state=FOTA_CORE_IDLE;
		fota.callback(fota.source, fota.host_status, fota.ble_status);
	}
}

/******************************************************************************
 * Unit test
 ******************************************************************************/

bool test_check()
{
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
//	//host update
//	ASSERT_RET(parse_json_info(host_update), false, "parse host update fail");
//	ASSERT_RET(fota.host_app_version == 2, false, "parse host app version");
//	ASSERT_RET(fota.host_app.len == 345, false, "parse host app len");
//	ASSERT_RET(fota.host_app.crc == 678, false, "parse host app crc");
//	ASSERT_RET(strcmp(fota.host_app.file_name, "hostApp.bin") == 0, false, "parse host name");
//	ASSERT_RET(strlen(fota.ble_app.file_name) == 0, false, "ble app name should be 0");
//	ASSERT_RET(strlen(fota.ble_init.file_name) == 0, false, "ble init name should be 0");
//	//
//	//ble update
//	ASSERT_RET(parse_json_info(ble_update), false, "parse ble update fail");
//	ASSERT_RET(fota.ble_app_version == 2, false, "parse ble app version");
//	ASSERT_RET(fota.ble_init.len == 345, false, "parse ble init len");
//	ASSERT_RET(fota.ble_init.crc == 678, false, "parse ble init crc");
//	ASSERT_RET(fota.ble_app.len == 901, false, "parse ble app len");
//	ASSERT_RET(fota.ble_app.crc == 234, false, "parse ble app crc");
//	ASSERT_RET(strcmp(fota.ble_init.file_name, "bleApp.dat") == 0, false, "parse ble init name");
//	ASSERT_RET(strcmp(fota.ble_app.file_name, "bleApp.bin") == 0, false, "parse ble app name");
//	ASSERT_RET(strlen(fota.host_app.file_name) == 0, false, "host name should be 0");
	return true;
}

bool TEST_parse_json_info()
{
	char full_update[] = "{\
	    \"hardwareVersion\":1,\
	    \"hostApp\": {\
	        \"version\":2,\
	        \"len\":345,\
	        \"crc\":678\
	    },\
	    \"bleApp\":{\
	        \"bleVersion\":2,\
	        \"initLen\":345,\
	        \"initCrc\":678,\
	        \"appLen\":901,\
	        \"appCrc\":234\
	    }\
	}";
	char host_update[] = "{\
	    \"hardwareVersion\":1,\
	    \"hostApp\": {\
	        \"version\":2,\
	        \"len\":345,\
	        \"crc\":678\
	    }\
	}";
	char ble_update[] = "{\
	    \"hardwareVersion\":1,\
	    \"bleApp\": {\
	        \"blVersion\":2,\
	        \"initLen\":345,\
	        \"initCrc\":678,\
	        \"appLen\":901,\
	        \"appCrc\":234\
	    }\
	}";
	//full update
//	ASSERT_RET(parse_json_info(full_update), false, "parse json full update fail");
//	ASSERT_RET(test_check(), false, "check value");

	ASSERT_RET(parse_json_as_string(full_update), false, "parse json as string full update fail");
	ASSERT_RET(test_check(), false, "check value");
	debug("Test passed");
	return true;
}

void fota_console_handle(char *result)
{
	if(__check_cmd("test"))
	{
		TEST_parse_json_info();
	}
}
