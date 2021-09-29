

/*
    FLASH fota

    HieuNT
    12/4/2019
*/

#define __DEBUG__ 4

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "flash_fota.h"
#include "crc/crc32.h"
#include "fota_comm.h"
#include "log_sys.h"



/*
    SHA256
    https://emn178.github.io/online-tools/sha256.html
*/


// Working buffer
uint8_t fotaBuffer[GD25Q16_SECTOR_SIZE];


bool flash_fota_read_ext_flash(uint32_t destAddr, uint32_t srcAddr, uint32_t len)
{
	GD25Q16_ReadSector(srcAddr, (uint8_t *)destAddr, len);
    return true;
}

// Note: fotaBuffer used to read from ext flash
// Warning: MCU flash must be erased before calling this function
bool flash_fota_program_mcu_from_ext_flash(uint32_t mcuFlashAddr, uint32_t extFlashAddr, uint32_t expected_crc, uint32_t size)
{
    uint32_t _len;
    uint32_t total, _mcuFlashAddr, _extFlashAddr;

    #define CHECK_PASS(cond, doSomethingsIfFailed, message, ...) \
        if (!(cond)){doSomethingsIfFailed;error(message, ##__VA_ARGS__); goto flash_fota_program_mcu_from_ext_flash_err;}

    total = size;
    _extFlashAddr = extFlashAddr;
    _mcuFlashAddr = mcuFlashAddr;
    debug("Start programing from ext flash @ %lu to mcu @ %lu in %lu bytes...\n", extFlashAddr, mcuFlashAddr, size);
    while(total > 0){
        _len = total > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : total;
        CHECK_PASS(flash_fota_read_ext_flash((uint32_t)&fotaBuffer, _extFlashAddr, _len), (void)0, "Read ext flash failed @ %lu\n", _extFlashAddr);
        // Note: FLASH_If_Write() co verify roi!
        CHECK_PASS(FLASH_If_Write(_mcuFlashAddr, (uint32_t *)&fotaBuffer, _len) == FLASHIF_OK, (void)0, "Write int flash failed @ %lu\n", _mcuFlashAddr);
        info("Programming @ %lu OK\n", _mcuFlashAddr);
        _mcuFlashAddr += _len;
        _extFlashAddr += _len;
        total -= _len;
        //ioctl_beepbeep(1,1);
    }


    uint32_t crc=crc32_compute((uint8_t *)mcuFlashAddr, size, NULL);
    if(crc==expected_crc){
    	debug("Programming OK\n");
    	return true;
    }
    else{
    	error("Programming CRC error\n");
		return false;
    }


    flash_fota_program_mcu_from_ext_flash_err:
    return false;
    #undef CHECK_PASS
}

bool flash_fota_erase_mcu_flash_app(void)
{
    #define CHECK_PASS(cond, doSomethingsIfFailed, message, ...) \
        if (!(cond)){doSomethingsIfFailed; error(message, ##__VA_ARGS__); goto flash_fota_erase_mcu_flash_BLlication_err;}

    FLASH_If_Init();
    WDT_FEED();
    CHECK_PASS(FLASH_If_Erase(APPLICATION_START_ADDR, APPLICATION_END_ADDR) == FLASHIF_OK, (void)0, "FlashIf erase failed");
    WDT_FEED();

    // TODO: verify
    delay(100);
    debug("Erase mcu flash application OK\n");
    return true;

    flash_fota_erase_mcu_flash_BLlication_err:
    return false;
    #undef CHECK_PASS
}

// Note: fotaBuffer used to read from ext flash
// Warning: MCU flash must be erased before calling this function
bool flash_fota_rollback_backup(flash_fota_progress_cb_fn_t cb)
{
    debug("Start rollback backup fw...\n");
    //return flash_fota_program_mcu_from_ext_flash(APPLICATION_START_ADDR, FLASH_BL_BACKUP_FW_ADDR, 1, APPLICATION_FW_SIZE, cb);
    return true;
}


bool flash_fota_check_valid_app_upgrade(host_app_fota_info_t *fwInfo)
{
    if (!flash_fota_read_ext_flash((uint32_t)fwInfo, EX_FLASH_APP_INFO_ADDR, sizeof(host_app_fota_info_t))){
        error("Read fota info\n");
        return false;
    }
    if (strcmp(fwInfo->signature, VALID_FOTA_FW_FLAG) != 0){
        error("Invalid fota fw signature\n");
        return false;
    }
    if(strcmp(fwInfo->fileName, "hostApp.bin")!=0){
    	error("Not valid bootloader name\n");
    	return false;
    }
    if (!GD25Q16_Crc32_check(fwInfo->flash_addr, fwInfo->len, fwInfo->crc)){
        error("File CRC check failed\n");
        return false;
    }
    GD25Q16_SectorErase(EX_FLASH_APP_INFO_ADDR);
    debug("Fota app valid:\n");
    debug("Filename: %s\n", fwInfo->fileName);
    debug("Filesize: %lu bytes\n", fwInfo->len);
    debug("Version: %s\n", fwInfo->version);
    debug("CRC: %08lX\n", fwInfo->crc);
    return true;
}

bool flash_check_fota_request(void){

	char signature[128]={0};
	if (!flash_fota_read_ext_flash((uint32_t)signature, EX_FLASH_FOTA_REQ_ADDR, 128)){
		error("Read fota error\n");
		return false;
	}
	if (strcmp(signature, VALID_FOTA_REQ_FLAG) != 0){
		error("Invalid request signature\n");
		return false;
	}
	return true;
}



// Note: fotaBuffer used to read from ext flash
// Warning: MCU flash must be erased before calling this function
bool flash_fota_fw_upgrade(host_app_fota_info_t *info)
{
    debug("Start fota fw upgrade...\n");
    return flash_fota_program_mcu_from_ext_flash(APPLICATION_START_ADDR, info->flash_addr, info->crc, info->len);
}
