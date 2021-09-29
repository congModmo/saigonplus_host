/*
 * ble_transport.c
 *
 *  Created on: Jun 14, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include <string.h>
#include "fota_core.h"
#include <app_fota/serial_transport.h>
#include "app_ble/host_comm/host_comm.h"
#include "ble/nina_b1.h"
#include "crc/crc32.h"
//typedef struct __packed
//{
//	uint8_t index;
//	int len;
//	uint32_t crc;
//} serial_data_info;

#define TRANSPORT_RETRY_NUM 2

typedef struct{
	uint8_t state;
	transport_err_cb_t err_cb;
	transport_get_file_cb_t file_cb;
	uint32_t tick;
	serial_interface_t serial;
	uint8_t *buff;
	uint16_t buff_len;
	//file
	fota_file_info_t* file_info;
	uint16_t file_retry;
	uint32_t file_received;
	//sector
	uint16_t sector_index;
	uint16_t sector_num;
	serial_data_info_t sector_info;
	uint16_t sector_retry;
	//block
	uint16_t block_index;
	uint16_t block_num;
	uint16_t block_retry;
}serial_transport_t;

typedef struct __packed{
	uint8_t cmd;
	uint8_t type;
	uint8_t index;
}ble_block_header_t;

static serial_transport_t transport;

enum{
	TRANSPORT_IDLE,
	TRANSPORT_WAIT_FILE_INFO,
	TRANSPORT_WAIT_SECTOR_INFO,
	TRANSPORT_WAIT_BLOCK,
	TRANSPORT_PROCESS_SECTOR,
	TRANSPORT_PROCESS_FILE,
	TRANSPORT_FINISH,
	TRANSPORT_ERROR
};

void serial_transport_init(serial_interface_t serial){
	transport.serial=serial;
}

static bool transport_init(uint8_t *buff, size_t size, void *params, transport_err_cb_t cb){
	transport.state=TRANSPORT_IDLE;
	transport.err_cb=cb;
	transport.tick=millis();
	transport.buff=buff;
	transport.buff_len=(uint16_t)size;
	transport.err_cb=cb;
	transport.serial.init();
	debug("Serial transport init buff size=%d\n", transport.buff_len);
	return true;
}

//void transport_send_cmd_array(uint8_t type, uint8_t *data, size_t len){
//	nina_b1_send2(HOST_COMM_UI_DATA, type, data, len);
//}

void transport_send_cmd(uint8_t type, uint8_t param){
	transport.serial.send(&type, 1, true);
	transport.serial.send(&param, 1, false);
}

static void transport_send_file_request(char *fileName){
	uint8_t cmd=DEVICE_REQUEST_FILE_INFO;
	transport.serial.send(&cmd, 1, true);
	transport.serial.send((uint8_t *)fileName, strlen(fileName), false);
}

static bool transport_get_file(fota_file_info_t *info, transport_get_file_cb_t cb){
	info("Transport get file %s\n", info->file_name);
	info("Addr 0x%lx\n", info->flash_addr);
	transport.file_cb=cb;
	transport.file_info=info;
	transport_send_file_request(info->file_name);
	transport.file_retry=TRANSPORT_RETRY_NUM;
	transport.state =TRANSPORT_WAIT_FILE_INFO;
	transport.tick=millis();
	return true;
}

static bool transport_process_sector()
{
	if(!crc32_verify(transport.buff, transport.sector_info.len, transport.sector_info.crc))
	{
		error("Invalid sector crc\n");
		return false;
	}
	if(!GD25Q16_WriteAndVerifySector(transport.file_info->flash_addr+transport.file_received, fotaCoreBuff, transport.sector_info.len))
	{
		//try again
		error("Save flash error\n");
		return false;
	}
	return true;
}

static void transport_end(bool success, uint8_t err_code)
{
	if(success)
	{
		debug("File oke la\n");
		transport.file_cb(true);
	}
	else
	{
		error("Something wrong\n");
		transport.err_cb(err_code);
	}
	transport.state=TRANSPORT_IDLE;
}

char *type_to_string(uint8_t type)
{
	switch(type)
	{
	case TRANSPORT_WAIT_BLOCK:
		return "block";
	case TRANSPORT_WAIT_SECTOR_INFO:
		return "sector";
	case TRANSPORT_WAIT_FILE_INFO:
		return "file";
	}
}

static void transport_retry()
{

	debug("retry type: %s\n", type_to_string(transport.state));
	if(transport.state==TRANSPORT_WAIT_BLOCK)
	{
		if(transport.block_retry>0)
		{
			transport.block_retry--;
			transport_send_cmd(DEVICE_REQUEST_BLOCK, transport.block_index);
		}
		else
		{
			transport.state=TRANSPORT_WAIT_SECTOR_INFO;
		}
	}
	if(transport.state==TRANSPORT_WAIT_SECTOR_INFO)
	{
		if(transport.sector_retry>0)
		{
			transport.sector_retry--;
			transport_send_cmd(DEVICE_REQUEST_SECTOR_INFO, transport.sector_index);
		}
		else
		{
			transport.state=TRANSPORT_WAIT_FILE_INFO;
		}
	}
	if(transport.state==TRANSPORT_WAIT_FILE_INFO)
	{
		if(transport.file_retry>0)
		{
			transport.file_retry--;
			transport_send_file_request(transport.file_info->file_name);
		}
		else
		{
			transport_end(false, 0);
		}
	}
}

static char *cmd_to_string(uint8_t cmd)
{
	switch(cmd)
	{
	case FOTA_REQUEST_FOTA:
		return "FOTA_REQUEST_FOTA";
	case FOTA_FILE_INFO:
		return "FOTA_FILE_INFO";
	case FOTA_SECTOR_INFO:
		return "FOTA_SECTOR_INFO";
	case FOTA_SECTOR_DATA:
		return "FOTA_SECTOR_DATA";
	case FOTA_REQUEST_MTU:
		return "FOTA_REQUEST_MTU";
	default:
		return NULL;
	}
}

uint16_t len_to_block_len(uint32_t len, uint16_t block_len)
{
	return (len/block_len) + ((len%block_len==0)?0:1);
}


bool TEST_len_to_block_len()
{
	ASSERT_RET(len_to_block_len(128, 128)==1, false, "fail");
	ASSERT_RET(len_to_block_len(52192, 4096)==13, false, "fail");
	ASSERT_RET(len_to_block_len(8192, 4096)==2, false, "fail");
	debug("test len to block ok\n");
	return true;
}

static void transport_command_handle(uint8_t *cmd, size_t len)
{
	transport.tick=millis();
	if(transport.state ==TRANSPORT_WAIT_FILE_INFO)
	{
		if(cmd[0]!=	FOTA_FILE_INFO)
		{
			error("Wating file info, invalid cmd %s\n", cmd_to_string(cmd[1]));
			return;
		}
		serial_data_info_t *info=(serial_data_info_t *)(cmd+1);
		if((info->len != transport.file_info->len) || (info->crc != transport.file_info->crc))
		{
			error("Invalid file, received len: %d, expecting %d, received crc: %lu, expecting %lu\n", info->len, transport.file_info->len, info->crc, transport.file_info->crc);
			transport_retry();
		}
		else
		{
			transport.state=TRANSPORT_WAIT_SECTOR_INFO;
			transport.sector_index=0;
			transport.sector_num=len_to_block_len(transport.file_info->len, transport.buff_len);
			transport.sector_retry=TRANSPORT_RETRY_NUM;
			transport.file_received=0;
			transport_send_cmd(DEVICE_REQUEST_SECTOR_INFO, transport.sector_index);
			info("start get file, file len: %d, buff_len: %d, sector num: %d\n", transport.file_info->len, transport.buff_len, transport.sector_num);
		}
	}
	else if(transport.state==TRANSPORT_WAIT_SECTOR_INFO){
		if(cmd[0]!=FOTA_SECTOR_INFO)
		{
			error("Waiting sector info, Invalid cmd %s\n", cmd_to_string(cmd[0]));
			return;
		}
		if(cmd[1]!=transport.sector_index)
		{
			error("Invalid sector idx\n");
			transport_retry();
		}
		else
		{
			serial_data_info_t *sector=(serial_data_info_t *)(cmd+2);
			transport.sector_info.len=sector->len;
			transport.sector_info.crc=sector->crc;
			transport.block_num=len_to_block_len(sector->len, transport.serial.mtu);
			transport.block_index=0;
			transport_send_cmd(DEVICE_REQUEST_BLOCK, transport.block_index);
			transport.state=TRANSPORT_WAIT_BLOCK;
			transport.block_retry=TRANSPORT_RETRY_NUM;
			debug("Sector info index: %u, sector len: %u, block_num: %d\n", transport.sector_index, transport.sector_info.len, transport.block_num);
		}
	}
	else if(transport.state==TRANSPORT_WAIT_BLOCK)
	{
		uint8_t block_len=len-2;
		if(cmd[0]!=FOTA_SECTOR_DATA)
		{
			error("Waiting sector data, Invalid cmd %s\n", cmd_to_string(cmd[0]));
		}
		else if(transport.block_index!=cmd[1])
		{
			error("Invalid block idx %d\n", block_len);
			transport_retry();
		}
		else if(block_len> transport.serial.mtu )
		{
			error("Invalid data len %d\n", block_len);
		} 
		else
		{
			debug("Sector rx block index %u, block len: %u\n", transport.block_index, block_len);
			memcpy(transport.buff + transport.block_index*transport.serial.mtu, cmd+2, block_len);
			transport.block_index++;
			if(transport.block_index==transport.block_num){
				transport.state=TRANSPORT_PROCESS_SECTOR;
			}
			else{
				transport_send_cmd(DEVICE_REQUEST_BLOCK, transport.block_index);
				transport.block_retry=TRANSPORT_RETRY_NUM;
			}
		}
	}
	//process
	if(transport.state==TRANSPORT_PROCESS_SECTOR)
	{
		debug("Process sector idx: %d\n", transport.sector_index);
		if(transport_process_sector())
		{
			transport.file_received+=transport.sector_info.len;
			transport.sector_index++;
			if(transport.sector_index==transport.sector_num)
			{
				debug("Start process file\n");
				transport.state=TRANSPORT_PROCESS_FILE;
			}
			else
			{
				debug("Request new sector idx %d\n", transport.sector_index);
				transport_send_cmd(DEVICE_REQUEST_SECTOR_INFO, transport.sector_index);
				transport.state=TRANSPORT_WAIT_SECTOR_INFO;
				transport.sector_retry=TRANSPORT_RETRY_NUM;
			}
		}
		else
		{
			transport.state=TRANSPORT_WAIT_SECTOR_INFO;
			transport_retry();
		}
	}
	if(transport.state==TRANSPORT_PROCESS_FILE)
	{
		if(GD25Q16_Crc32_check(transport.file_info->flash_addr, transport.file_info->len, transport.file_info->crc))
		{
			transport_end(true, 0);
		}
		else
		{
			transport.state=TRANSPORT_WAIT_FILE_INFO;
			transport_retry();
		}
	}
}

static void transport_process(void){
	if(transport.state!=TRANSPORT_IDLE)
	{
		if(millis()-transport.tick>500)
		{
			debug("transport timeout\n");
			transport.tick=millis();
			transport_retry();
		}
		transport.serial.polling(transport_command_handle);
	}
}

static void transport_fota_signal(bool status){
	if(status){
		transport_send_cmd(DEVICE_REQUEST_PROCESS_END, DEVICE_STATUS_OK);
	}
	else{
		transport_send_cmd(DEVICE_REQUEST_PROCESS_END, DEVICE_STATUS_ERROR);
	}
}

const fota_transport_t serial_transport={ .init= transport_init,\
										.get_file=transport_get_file,\
										.signal=transport_fota_signal,\
										.process=transport_process};
