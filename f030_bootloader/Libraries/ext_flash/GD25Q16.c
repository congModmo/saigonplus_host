#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "GD25Q16.h"
#include "bsp.h"
#include "crc/crc32.h"

void GD25Q16_Init(void)
{

}

void GD25Q16_GetID(uint8_t data[3])
{
	uint8_t txData[1] = {0};
	
	txData[0] = GD25Q16_COMMAND_RDID;
	
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 1, data, 3);
	flash_chip_select(HIGH);
}

void GD25Q16_SetAddress(uint32_t address)
{
	uint8_t txData[3] = {0};
	txData[0] = address >> 16;
	txData[1] = address >> 8;
	txData[2] = address;
	
	flash_rw_bytes(txData, 3, NULL, 0);
}

void GD25Q16_WriteEnable(void)
{
	uint8_t txData[1] = {0};
	txData[0] = GD25Q16_COMMAND_WREN;
	
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 1, NULL, 0);
	flash_chip_select(HIGH);
}

void GD25Q16_SetStatus(uint8_t status)
{
	uint8_t txData[2] = {0};
	txData[0] = GD25Q16_COMMAND_WRSR;
	txData[1] = status;
	
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 2, NULL, 0);
	flash_chip_select(HIGH);
}

uint8_t GD25Q16_GetStatus(void)
{
	uint8_t status;
	uint8_t txData[1] = {0};

	txData[0] = GD25Q16_COMMAND_RDSR;
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 1, &status, 1);
	flash_chip_select(HIGH);
	return status;
}

void GD25Q16_SectorErase(uint32_t address)
{
	uint8_t txData[4] = {0};
	txData[0] = GD25Q16_COMMAND_SE;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;
	
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 4, NULL, 0);
	flash_chip_select(HIGH);
	
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
}

void GD25Q16_BlockErase(uint32_t address)
{
	uint8_t txData[4] = {0};
	txData[0] = GD25Q16_COMMAND_BE;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;
	
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 4, NULL, 0);
	flash_chip_select(HIGH);
	
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
}

void GD25Q16_ChipErase(void)
{
	uint8_t txData[1] = {0};
	txData[0] = GD25Q16_COMMAND_CE;
	
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 1, NULL, 0);
	flash_chip_select(HIGH);
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
}

void GD25Q16_Write(uint32_t address, uint8_t data)
{
	uint8_t txData[5] = {0};
	txData[0] = GD25Q16_COMMAND_WRITE;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;
	txData[4] = data;
	
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 5, NULL, 0);
	flash_chip_select(HIGH);
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
}

// Warning: write byte with erase entire sector
void GD25Q16_WriteByte(uint32_t address, uint8_t data)
{
	GD25Q16_SectorErase(address);
	GD25Q16_Write(address, data);
}

uint8_t GD25Q16_ReadByte(uint32_t address)
{
	uint8_t txData[4] = {0};
	uint8_t rData = 0;
	
	txData[0] = GD25Q16_COMMAND_READ;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;
	
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 4, &rData, 1);
	flash_chip_select(HIGH);
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
	
	return rData;
}

void GD25Q16_ReadAllDataOnSector(uint32_t sector_address, uint8_t rData[], uint16_t len)
{
	uint16_t i = 0;
	
	for(i = 0; i < len; i++)
	{
		rData[i] = GD25Q16_ReadByte(sector_address + i);
	}
}

// Warning: erase entire sector
void GD25Q16_RewriteDataOnSector(uint32_t sector_address, uint8_t wData[], uint16_t len)
{
	GD25Q16_SectorErase(sector_address);
	for(uint16_t i = 0; i < len; i++)
	{
		GD25Q16_Write(sector_address + i, wData[i]);
	}
}
// Dau fong, co may ham read/write flash ma roi vai~... tu viet cho r!
// HieuNT: simple & efficient!
void GD25Q16_WriteBlock(uint32_t flashAddr, uint8_t *data, uint32_t len)
{
	while(len-->0){
		GD25Q16_Write(flashAddr++, *data++);
	}
}

void GD25Q16_ReadBlock(uint32_t flashAddr, uint8_t *data, uint32_t len)
{
	while(len-->0){
		*data++ = GD25Q16_ReadByte(flashAddr++);
	}
}


void GD25Q16_Write_Page(uint32_t address, uint8_t *data, uint16_t len)
{
	if(len>GD25Q16_PAGE_SIZE)
		return;

	uint8_t txData[4+len];
	txData[0] = GD25Q16_COMMAND_WRITE;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;
	memcpy(txData+4, data, len);
	GD25Q16_WriteEnable();
	flash_chip_select(LOW);
	flash_rw_bytes(txData, 4 +len,NULL, 0);
	flash_chip_select(HIGH);
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);
}

void GD25Q16_Read_Page(uint32_t address, uint8_t *page, uint16_t len)
{
	if(len>GD25Q16_PAGE_SIZE)
		return;

	uint8_t txData[4] = {0};

	txData[0] = GD25Q16_COMMAND_READ;
	txData[1] = address >> 16;
	txData[2] = address >> 8;
	txData[3] = address;

	flash_chip_select(LOW);
	flash_rw_bytes(txData, 4, page, len);
	flash_chip_select(HIGH);
	while(GD25Q16_GetStatus() & GD25Q16_STATUS_WIP);

}

// Warning: erase entire sector
void GD25Q16_WriteSector(uint32_t sector_address, uint8_t *data, uint16_t len)
{
	GD25Q16_SectorErase(sector_address);
//	GD25Q16_WriteBlock(sector_address, data, len);
	uint16_t offset=0;
	while(offset<len){
		if(offset+GD25Q16_PAGE_SIZE<=len){
			GD25Q16_Write_Page(sector_address+offset, data+offset, GD25Q16_PAGE_SIZE);
			offset+=GD25Q16_PAGE_SIZE;
		}
		else{
			GD25Q16_WriteBlock(sector_address+offset, data+offset, len-offset);
			offset=len;
		}
	}
}

// Warning: erase entire sector
bool GD25Q16_WriteAndVerifySector(uint32_t sector_address, uint8_t *data, uint16_t len)
{
	GD25Q16_SectorErase(sector_address);

	uint16_t offset=0;
	while(offset<len){
		if(offset+GD25Q16_PAGE_SIZE<=len){
			GD25Q16_Write_Page(sector_address+offset, data+offset, GD25Q16_PAGE_SIZE);
			offset+=GD25Q16_PAGE_SIZE;
		}
		else{
			GD25Q16_WriteBlock(sector_address+offset, data+offset, len-offset);
			offset=len;
		}
	}
	uint32_t crc=crc32_compute(data, len, NULL);
	return GD25Q16_Crc32_check(sector_address, len, crc);
}

void GD25Q16_ReadSector(uint32_t sector_address, uint8_t *data, uint16_t len)
{
//	GD25Q16_ReadBlock(data, sector_address, len);
	uint16_t offset=0;
	while(offset<len){
		if(offset+GD25Q16_PAGE_SIZE<=len){
			GD25Q16_Read_Page(sector_address+offset, data+offset, GD25Q16_PAGE_SIZE);
			offset+=GD25Q16_PAGE_SIZE;
		}
		else{
			GD25Q16_ReadBlock(sector_address+offset, data+offset, len-offset);
			offset=len;
		}
	}
}


bool GD25Q16_Crc32_check(uint32_t address, uint32_t len, uint32_t expected_crc)
{
	uint32_t crc=0xFFFFFFFF;
	uint32_t offset=0;
	static uint8_t page[GD25Q16_PAGE_SIZE];
	while(offset<len){
		if(offset+GD25Q16_PAGE_SIZE<=len){
			GD25Q16_Read_Page(address+offset, page, GD25Q16_PAGE_SIZE);
			crc=crc32_compute(page, GD25Q16_PAGE_SIZE, &crc);
			offset+=GD25Q16_PAGE_SIZE;
		}
		else{
			GD25Q16_ReadBlock(address+offset, page, len-offset);
			crc=crc32_compute(page, len-offset, &crc);
			offset=len;
		}
	}
	if(crc==expected_crc)
		return true;
	return false;
}


uint8_t flash_sector[GD25Q16_SECTOR_SIZE];

static void write_random_number_to_flash(uint32_t startAddr, uint32_t size){
	memset(flash_sector +startAddr, 0, size);
	for(int i=startAddr; i<size; i++){
		flash_sector[i]=(uint8_t)rand();
	}

	printf("write %lu bytes to flash\n", size);
	uint32_t tick=millis();
	//write whole sector
	uint32_t crc=crc32_compute(flash_sector+ startAddr, size, NULL);
	GD25Q16_WriteSector(startAddr, flash_sector+startAddr, size);
	printf("write tick: %lu\n", millis()-tick);
	tick=millis();
	memset(flash_sector+startAddr, 0, size);
	GD25Q16_ReadSector(startAddr, flash_sector+startAddr, size);
	printf("read tick: %lu\n", millis()-tick);
	if(crc32_compute(flash_sector+startAddr, size, NULL)==crc){
		printf("Write %lu bytes OK\n\n", size);
	}
	else{
		printf("Write sector FAIL\n\n");
		return;
	}
}
void GD25Q16_test(){

	write_random_number_to_flash(0, GD25Q16_SECTOR_SIZE);
	write_random_number_to_flash(0, 2048);
	write_random_number_to_flash(0, 256);
	write_random_number_to_flash(0, 512);
	write_random_number_to_flash(0, 100);
	write_random_number_to_flash(0, GD25Q16_SECTOR_SIZE-1000);
	write_random_number_to_flash(2048, 1000);
}

