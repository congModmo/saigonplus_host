/*
 * fota_comm.h
 *
 *  Created on: Apr 13, 2021
 *      Author: thanhcong
 */

#ifndef FOTA_COMM_H_
#define FOTA_COMM_H_
#include <stdint.h>

#define FIRST_BOOTLOADER_START_ADDR   (0x08000000UL)
#define FIRST_BOOTLOADER_SIZE         (0x4000UL)

#define APPLICATION_START_ADDR  (0x08008000UL)
#define APPLICATION_FW_SIZE     (0x38000UL)
#define APPLICATION_END_ADDR     (APPLICATION_START_ADDR + APPLICATION_FW_SIZE)

// On FLASH chip
// First block of 512K used for fota info & fw

#define FLASH_FOTA_SECTOR_SZ 4096
#define EX_FLASH_FOTA_REQ_ADDR   (0x32000UL) // offset 128K, for both app and 2nd bootloader
#define EX_FLASH_FOTA_REQ_SZ     FLASH_FOTA_SECTOR_SZ*2

#define EX_FLASH_APP_INFO_ADDR (EX_FLASH_FOTA_REQ_ADDR+EX_FLASH_FOTA_REQ_SZ)
#define EX_FLASH_APP_INFO_SZ FLASH_FOTA_SECTOR_SZ*2

#define EX_FLASH_FOTA_FILES_START EX_FLASH_APP_INFO_ADDR+EX_FLASH_APP_INFO_SZ

#define FLASH_DEVICE_FACTORY_INFO      0x08007000
#define EX_FLASH_APP_INFO		GD25Q16_SECTOR_0_START
#define EX_FLASH_LOCK_STATE		GD25Q16_SECTOR_1_START

#define VALID_FOTA_FW_FLAG "041c5cfe3ec8e1e050ad8acdce2a8c5e5f5a8042d18d3e6ebbbb73d3434165ce"
#define VALID_FOTA_REQ_FLAG "94ae72514dcf784d2933a66d818d818752a14e9b165be8229e481a6d73fbd894"

typedef struct{
	char signature[128];
	char model[32];
	char version[32];
	int batch;
	int info_len;
	uint32_t info_crc;
	int source;
	char link[512];
}fota_request_msg_t;

typedef struct {
  char signature[128];
  char fileName[100];
  char version[32];
  uint32_t source;
  uint32_t flash_addr;
  uint32_t len;
  uint32_t crc;
} host_app_fota_info_t;

#endif /* EXT_FLASH_FOTA_COMM_H_ */
