#include <app_common/wcb_common.h>
#include "ext_flash/GD25Q16.h"
#include "Define.h"
#include "string.h"
#include "stdbool.h"
#include "log_sys.h"
#include "bsp.h"
#include "fota_comm.h"
void flash_ext_init(void)
{
	GD25Q16_Init();
}

void flash_ext_read_report_periodic(void)
{
//	uint8_t *data = (uint8_t *)&bikeReportPeriodic;
//	uint8_t i;
//	bool commit = false;
//	for (i = 0; i < sizeof(bikeReportPeriodic); i++){
//		*data++ = GD25Q16_ReadByte(FLASH_EXT_ADDR_REPORT_PERIODIC + i);
//	}
//	if (bikeReportPeriodic.normal == 0xFFFFFFFF){
//		warning("Force factory reset normal report periodic\n");
//		bikeReportPeriodic.normal = DEFAULT_REPORT_PERIODIC_NORMAL;
//		commit = true;
//	}
//	if (bikeReportPeriodic.lowPower == 0xFFFFFFFF){
//		warning("Force factory reset low power report periodic\n");
//		bikeReportPeriodic.lowPower = DEFAULT_REPORT_PERIODIC_LOWPOWER;
//		commit = true;
//	}
//	if (commit){
//		flash_ext_write_report_periodic();
//	}
}

void flash_ext_write_report_periodic(void)
{
//	GD25Q16_RewriteDataOnSector(FLASH_EXT_ADDR_REPORT_PERIODIC, (uint8_t *)&bikeReportPeriodic, sizeof(bikeReportPeriodic));
}

void flash_ext_read_operation_mode(void)
{
//	uint8_t *data = (uint8_t *)&bikeOperationMode;
//	uint8_t i;
//	bool commit = false;
//	for (i = 0; i < sizeof(bikeOperationMode); i++){
//		*data++ = GD25Q16_ReadByte(FLASH_EXT_ADDR_LOWPOWER + i);
//	}
//	if (bikeOperationMode.lowPower == 0xFF){
//		warning("Force factory reset operation mode (normal)\n");
//		bikeOperationMode.lowPower = 0;
//		commit = true;
//	}
//	if (commit){
//		flash_ext_write_operation_mode();
//	}
}

void flash_ext_write_operation_mode(void)
{
//	GD25Q16_RewriteDataOnSector(FLASH_EXT_ADDR_LOWPOWER, (uint8_t *)&bikeOperationMode, sizeof(bikeOperationMode));
}

void flash_ext_write_serialid(uint8_t _serialid[])
{
//	_serialid[strlen((char *)_serialid)] = '\n';
//	GD25Q16_RewriteDataOnSector(FLASH_EXT_ADDR_SERIALNUMBER, _serialid, strlen((char *)_serialid) + 1);
//	_serialid[strlen((char *)_serialid)] = '\0';
}

uint16_t flash_ext_read_serialid(uint8_t _serialid[], uint16_t maxLen)
{
//	uint8_t i = 0;
//	do
//	{
//		_serialid[i] = GD25Q16_ReadByte(FLASH_EXT_ADDR_SERIALNUMBER + i);
//		i++;
//	} while (_serialid[i - 1] != '\n' && _serialid[i - 1] != '\0' && _serialid[i - 1] != 255 && i < maxLen);
//	_serialid[i - 1] = '\0';
//	return i - 1;
}
void flash_ext_write_bikeid(uint8_t _bikeid[])
{
//	_bikeid[strlen((char *)_bikeid)] = '\n';
//	GD25Q16_RewriteDataOnSector(FLASH_EXT_ADDR_BIKEID, _bikeid, strlen((char *)_bikeid) + 1);
//	_bikeid[strlen((char *)_bikeid)] = '\0';
}

uint16_t flash_ext_read_bikeid(uint8_t _bikeid[], uint16_t maxLen)
{
//	uint8_t i = 0;
//	do
//	{
//		_bikeid[i] = GD25Q16_ReadByte(FLASH_EXT_ADDR_BIKEID + i);
//		i++;
//	} while (_bikeid[i - 1] != '\n' && _bikeid[i - 1] != '\0' && _bikeid[i - 1] != 255 && i < maxLen);
//	_bikeid[i - 1] = '\0';
//	return i - 1;
}
