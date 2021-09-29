
/*
  Flash FOTA

  HieuNT
  12/4/2019
*/

#ifndef __FLASH_FOTA_H_
#define __FLASH_FOTA_H_


#include "ext_flash/GD25Q16.h"
#include "bsp.h"

#include "flash_if.h"
#ifdef __cplusplus
extern "C" {
#endif

bool flash_fota_erase_mcu_app(void);
bool flash_fota_program_app_from_ext_flash( uint32_t extFlashAddr, uint32_t size, uint32_t expected_crc);

#ifdef __cplusplus
}
#endif

#endif

