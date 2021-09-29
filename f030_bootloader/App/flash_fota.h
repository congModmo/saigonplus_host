
/*
  Flash FOTA

  HieuNT
  12/4/2019
*/

#ifndef __FLASH_FOTA_H_
#define __FLASH_FOTA_H_


#include "fota_comm/fota_comm.h"
#include "flash_if.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*flash_fota_progress_cb_fn_t)(uint32_t percent);
bool flash_fota_check_valid_bl_upgrade(host_app_fota_info_t *fwInfo);
bool flash_check_fota_request(void);
bool flash_fota_fw_upgrade(host_app_fota_info_t *info);
bool flash_fota_erase_mcu_flash_bootloader(void);
#ifdef __cplusplus
}
#endif

#endif

