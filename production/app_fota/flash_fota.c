

/*
    FLASH fota

    HieuNT
    12/4/2019
*/

#include "flash_fota.h"

#include "log_sys.h"
#include "fota_comm.h"
#include <string.h>

#include "fota_core.h"
#include "crc/crc32.h"
#define WDT_FEED()

bool flash_fota_read_ext_flash(uint32_t destAddr, uint32_t srcAddr, uint32_t len)
{
	GD25Q16_ReadBlock(srcAddr, (uint8_t *)destAddr, len);
	return true;
}

// Note: fotaBuffer used to read from ext flash
// Warning: MCU flash must be erased before calling this function
bool flash_fota_program_app_from_ext_flash( uint32_t extFlashAddr, uint32_t size, uint32_t expected_crc)
{
    uint32_t _len;
    uint32_t total, _mcuFlashAddr, _extFlashAddr;

    #define CHECK_PASS(cond, doSomethingsIfFailed, message, ...) \
        if (!(cond)){doSomethingsIfFailed; error(message, ##__VA_ARGS__); goto flash_fota_program_mcu_from_ext_flash_err;}

    total = size;
    _extFlashAddr = extFlashAddr;
    _mcuFlashAddr = APPLICATION_START_ADDR;
    debug("Start programing from ext flash @ %p to mcu @ %p in %u bytes...\n", extFlashAddr, APPLICATION_START_ADDR, size);
    while(total > 0){
        _len = total > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : total;
        CHECK_PASS(flash_fota_read_ext_flash((uint32_t)&fotaCoreBuff, _extFlashAddr, _len), (void)0, "Read ext flash failed @ %p\n", _extFlashAddr);
        // Note: FLASH_If_Write() co verify roi!
        CHECK_PASS(FLASH_If_Write(_mcuFlashAddr, (uint32_t *)&fotaCoreBuff, _len) == FLASHIF_OK, (void)0, "Write int flash failed @ %p\n", _mcuFlashAddr);
        info("Programming @ %p OK\n", _mcuFlashAddr);
        _mcuFlashAddr += _len;
        _extFlashAddr += _len;
        total -= _len;
        //ioctl_beepbeep(1,1);
    }
    CHECK_PASS(crc32_verify(APPLICATION_START_ADDR , size, expected_crc), (void)0, "Verify crc failed \n");

    info("Programming OK\n");
    return true;

    flash_fota_program_mcu_from_ext_flash_err:
    return false;
    #undef CHECK_PASS
}

bool flash_fota_erase_mcu_app(void)
{
    #define CHECK_PASS(cond, doSomethingsIfFailed, message, ...) \
        if (!(cond)){doSomethingsIfFailed; error(message, ##__VA_ARGS__); goto flash_fota_erase_mcu_flash_application_err;}

    FLASH_If_Init();
    WDT_FEED();
    CHECK_PASS(FLASH_If_Erase(APPLICATION_START_ADDR, APPLICATION_END_ADDR) == FLASHIF_OK, (void)0, "FlashIf erase failed");
    WDT_FEED();

    // TODO: verify
    delay(100);
    info("Erase mcu flash application OK\n");
    return true;

    flash_fota_erase_mcu_flash_application_err:
    return false;
    #undef CHECK_PASS
}
