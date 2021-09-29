#ifndef __GD25Q16_H__
#define __GD25Q16_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#define LOW                                 0
#define HIGH                                1

#define GD25Q16_SECTOR_COUNT 			512
#define GD25Q16_BLOCK_SECTOR_COUNT 			8

#define GD25Q16_COMMAND_WREN                0x06
#define GD25Q16_COMMAND_WRDI                0x04
#define GD25Q16_COMMAND_RDSR                0x05
#define GD25Q16_COMMAND_WRSR                0x01
#define GD25Q16_COMMAND_READ                0x03
#define GD25Q16_COMMAND_FREAD               0x0B
#define GD25Q16_COMMAND_WRITE               0x02
#define GD25Q16_COMMAND_RDID                0x9F
#define GD25Q16_COMMAND_SE                  0x20
#define GD25Q16_COMMAND_BE                  0x52
#define GD25Q16_COMMAND_CE                  0x60

#define GD25Q16_STATUS_WIP                  0x01
#define GD25Q16_STATUS_WEL                  0x02
#define GD25Q16_STATUS_BP0                  0x04
#define GD25Q16_STATUS_BP1                  0x08
#define GD25Q16_STATUS_BP2                  0x10
#define GD25Q16_STATUS_BP3                  0x20
#define GD25Q16_STATUS_RES                  0x40
#define GD25Q16_STATUS_SWRD                 0x80

#define DUMMY                               0xFF

#define GD25Q16_CAPACITY	                (1L * 1024L * 1024L)

#define GD25Q16_SECTOR_SIZE                 0x1000UL   //4K
#define GD25Q16_SECTOR_BASE                 0x000000 // Sector 0 
#define GD25Q16_SECTOR_0_START              (GD25Q16_SECTOR_BASE)
#define GD25Q16_SECTOR_1_START              (GD25Q16_SECTOR_0_START  + GD25Q16_SECTOR_SIZE)
#define GD25Q16_SECTOR_2_START              (GD25Q16_SECTOR_1_START  + GD25Q16_SECTOR_SIZE)
#define GD25Q16_SECTOR_3_START              (GD25Q16_SECTOR_2_START  + GD25Q16_SECTOR_SIZE)
#define GD25Q16_SECTOR_4_START              (GD25Q16_SECTOR_3_START  + GD25Q16_SECTOR_SIZE)

#define FLASH_EXT_COMMON_SECTOR             GD25Q16_SECTOR_0_START
#define FLASH_EXT_CONN_PARAMETER_SECTOR     GD25Q16_SECTOR_1_START
#define FLASH_EXT_BLE_PARAMETER_SECTOR		GD25Q16_SECTOR_2_START
#define FLASH_EXT_GSM_PARAMETER_SECTOR		GD25Q16_SECTOR_3_START
#define FLASH_EXT_LP_PARAMETER_SECTOR		GD25Q16_SECTOR_4_START

#define GD25Q16_PAGE_SIZE 128

void GD25Q16_Init(void);
void GD25Q16_GetID(uint8_t data[3]);
void GD25Q16_SectorErase(uint32_t address);
void GD25Q16_BlockErase(uint32_t address);
void GD25Q16_ChipErase(void);
void GD25Q16_WriteByte(uint32_t address, uint8_t data);
uint8_t GD25Q16_ReadByte(uint32_t address);
void GD25Q16_ReadAllDataOnSector(uint32_t sector_address, uint8_t rData[], uint16_t len);
void GD25Q16_RewriteDataOnSector(uint32_t sector_address, uint8_t wData[], uint16_t len);
void GD25Q16_WriteBlock(uint32_t flashAddr, uint8_t *data, uint32_t len);
void GD25Q16_ReadBlock(uint32_t flashAddr, uint8_t *data, uint32_t len);
void GD25Q16_WriteSector(uint32_t sector_address, uint8_t *data, uint16_t len);
void GD25Q16_ReadSector(uint32_t sector_address, uint8_t *data, uint16_t len);
bool GD25Q16_Crc32_check(uint32_t address, uint32_t len, uint32_t expected_crc);
bool GD25Q16_WriteAndVerifySector(uint32_t sector_address, uint8_t *data, uint16_t len);
void GD25Q16_test();
/*
 * Implement in BSP
 */

void flash_chip_select(uint8_t state);
void flash_rw_bytes(uint8_t *txData, uint16_t txLen, uint8_t *rxData, uint16_t rxLen);


#ifdef __cplusplus
}
#endif

#endif
