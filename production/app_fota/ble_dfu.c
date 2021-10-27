/*
 * ble_dfu.c
 *
 *  Created on: Dec 23, 2020
 *      Author: thanhcong
 *      Purpose: to update ble firmware
 */

#define __DEBUG__ 3
#ifndef __MODULE__
  #define __MODULE__ "ble dfu"
#endif

#include "ble_dfu.h"
#include "slip/slip_v2.h"
#include <ble/nina_b1.h>
#include "log_sys.h"
#include "ringbuf/ringbuf.h"
#include "nrf_dfu_req_handler.h"
#include "crc/crc32.h"
#include "flash_fota.h"
#include "bsp.h"
#include "host_ble_comm.h"

static ble_dfu_t ble_dfu = {.mtu=0, .prn=0};


static fota_file_info_t *dat_file=NULL;
static fota_file_info_t *bin_file=NULL;
int corrupt_test=0;


void ble_dfu_init(fota_file_info_t *dat, fota_file_info_t *bin){
//	slip_reset(&slip);
	//bsp_ble_dfu_init(&bleDfuRb);
	dat_file=dat;
	bin_file=bin;
	debug("init packet size: %lu bytes, flash addr: 0x%x\n", dat_file->len, dat_file->flash_addr);
	debug("firmware packet size: %lu bytes, flash addr: 0x%x\n", bin_file->len, bin_file->flash_addr);
}

bool ble_slip_ping(uint8_t id){
	uint8_t packet[2]={NRF_DFU_OP_PING, id};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 2, &resp, &resp_len, 1000, 2)){
		return false;
	}
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp +1);
	if(nrf_resp->request==NRF_DFU_OP_PING
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		return true;
	}
	return false;
}

bool ble_slip_object_select(uint8_t type, nrf_dfu_response_select_t *select){
	uint8_t packet[2]={NRF_DFU_OP_OBJECT_SELECT, type};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 2, &resp, &resp_len, 1000, 2)){
		return false;
	}
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp +1);
	if(nrf_resp->request==NRF_DFU_OP_OBJECT_SELECT
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){

		debug("crc: 0x%lx\n", nrf_resp->select.crc);
		debug("maxsize: %lu\n", nrf_resp->select.max_size);
		debug("offset: %lu\n", nrf_resp->select.offset);
		*select=nrf_resp->select;
		return true;
	}
	return false;
}
//prn=0: auto crc off
//prn=n: auto crc after n mtu frame.
bool ble_slip_prn(){
	uint8_t packet[3]={NRF_DFU_OP_RECEIPT_NOTIF_SET, 0, 0};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 3, &resp, &resp_len, 1000, 2)){
		return false;
	}
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp +1);
	if(nrf_resp->request==NRF_DFU_OP_RECEIPT_NOTIF_SET
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		return true;
	}
	return false;
}

bool ble_slip_crc(nrf_dfu_response_crc_t *crc){
	uint8_t packet[1]={NRF_DFU_OP_CRC_GET};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 1, &resp, &resp_len, 1000, 2)){
		return false;
	}
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp +1);
	if(nrf_resp->request==NRF_DFU_OP_CRC_GET
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		*crc=nrf_resp->crc;
		return true;
	}
	return false;
}

bool ble_slip_execute(void){
	uint8_t packet[1]={NRF_DFU_OP_OBJECT_EXECUTE};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 1, &resp, &resp_len, 1000, 2)){
		return false;
	}
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp +1);
	if(nrf_resp->request==NRF_DFU_OP_OBJECT_EXECUTE
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		return true;
	}
	return false;
}
//request for ble bootloader mtu
bool ble_slip_mtu(uint16_t *mtu){
	uint8_t packet[1]={NRF_DFU_OP_MTU_GET};
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 1, &resp, &resp_len, 1000, 2)){
		return false;
	}
	//printbuf(resp, resp_len);
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp + 1);
	if(nrf_resp->request==NRF_DFU_OP_MTU_GET
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		*mtu=nrf_resp->mtu.size;
		if(mtu>0)
			return true;
	}
	return false;
}

bool ble_create_object(uint8_t type, uint32_t size){

	uint8_t packet[6];
	packet[0]=NRF_DFU_OP_OBJECT_CREATE;
	nrf_dfu_request_create_t *create= (nrf_dfu_request_create_t *)(packet+1);
	create->object_size=size;
	create->object_type=type;
	uint8_t *resp;
	uint32_t resp_len;
	if(!nina_b1_ask_get_response(packet, 6, &resp, &resp_len, 1000, 2)){
		return false;
	}
	//printbuf(resp, resp_len);
	if(resp[0]!=NRF_DFU_OP_RESPONSE){
		return false;
	}
	nrf_dfu_response_t *nrf_resp= (nrf_dfu_response_t *)(resp + 1);
	if(nrf_resp->request==NRF_DFU_OP_OBJECT_CREATE
			&& nrf_resp->result==NRF_DFU_RES_CODE_SUCCESS){
		return true;
	}
	return false;
}

bool ble_send_cmd(uint8_t *cmd, uint16_t size){

	nina_b1_send0(cmd , size);

	return true;
}

void ble_request_dfu()
{
	uint8_t cmd[1]={HOST_CMD_DFU};
	nina_b1_ask(HOST_COMM_BLE_MSG, cmd, 1, HOST_BLE_OK, 100, 1);
}

bool ble_dfu_start(){
//
	int retry=3;
	while(retry)
	{
		ble_request_dfu();
		delay(200);
		if(ble_slip_ping(1))
		{
			//Ping ok
			break;
		}
		else if(retry==0)
		{
			error("ping fail\n");
			return false;
		}
		nina_b1_reset();
		delay(100);
		retry--;
	}
	delay(100);
	if(!ble_slip_prn()){
		error("prn fail\n");
		return false;
	}
	if(!ble_slip_mtu(&ble_dfu.mtu)){
		error("mtu fail\n");
		return false;
	}
	return true;
}

// Note: data is ext flash address
bool stream_data(uint32_t data, uint32_t len, uint32_t *crc, uint32_t offset){
	uint32_t cal_crc;
	if(crc==NULL)
		cal_crc=0;
	else
		cal_crc=*crc;
	uint8_t stream_buff[ble_dfu.mtu/2];
	stream_buff[0]=NRF_DFU_OP_OBJECT_WRITE;
	int max_size=(ble_dfu.mtu-1)/2 -1;
	uint32_t i=0;
	int data_to_send=0;
	while(i<len){
		if(i+max_size>len)
			data_to_send=(len-i);
		else
			data_to_send=max_size;
		// memcpy(stream_buff +1, data+i, data_to_send);
		GD25Q16_ReadBlock(data + i, (uint8_t *)stream_buff+ 1, data_to_send);
//		flash_fota_read_ext_flash((uint32_t)stream_buff + 1, data + i, data_to_send);
		if(!ble_send_cmd(stream_buff, data_to_send+1)){
			error("Send data fail\n");
			return false;
		}
		cal_crc=nrf_crc32_compute(stream_buff+1, data_to_send, &cal_crc);
		i+=data_to_send;
	}
	offset+=i;
	nrf_dfu_response_crc_t check_sum;
	if(!ble_slip_crc(&check_sum)){
		error("Cannot get checksum\n");
		return false;
	}
	if(check_sum.crc!=cal_crc){
		error("Crc error\n");
		return false;
	}
	if(check_sum.offset!=offset){
		error("offset error\n");
		return false;
	}
	if(crc!=NULL)
		*crc=cal_crc;
	return true;
}

uint32_t crc32_process_ext_flash(uint32_t flashAddr, uint32_t offset)
{
	uint32_t expected_crc=0xFFFFFFFF;
	uint32_t ofs = 0;
	uint32_t block;
	while (ofs < offset){
		block = (offset - ofs) > FLASH_FOTA_SECTOR_SZ ? FLASH_FOTA_SECTOR_SZ : (offset - ofs);
		flash_fota_read_ext_flash((uint32_t)fotaCoreBuff, flashAddr + ofs, block);
		expected_crc = nrf_crc32_compute(fotaCoreBuff, block, &expected_crc);
		ofs += block;
	}
	return expected_crc;
}

// max size is 4096 bytes, every type 4096 bytes are transmitted, calculate all data crc.
bool ble_dfu_send_init_packet(){
	nrf_dfu_response_select_t response;
	if(!ble_slip_object_select(NRF_DFU_OBJ_TYPE_COMMAND, &response)){
		error("read info fail\n");
		return false;
	}
	if(response.offset==0 || response.offset>dat_file->len){
		//nothing to recover
		debug("Nothing to recover\n");
		goto __start_streaming;
	}
	uint32_t expected_crc=crc32_process_ext_flash(dat_file->flash_addr, response.offset);
	if(expected_crc!=response.crc){
		// Present init packet is invalid.
		debug("Invalid packet\n");
		goto __start_streaming;
	}
	if(dat_file->len > response.offset){
		debug("Sending missing part\n");
		if(stream_data(dat_file->flash_addr+response.offset, dat_file->len - response.offset, &expected_crc, response.offset)){
			return true;
		}
		return false;
	}
	__start_streaming:
	debug("Start streaming\n");
	if(!ble_create_object(NRF_DFU_OBJ_TYPE_COMMAND, dat_file->len)){
		error("Create object failed\n");
		return false;
	}
	if(!stream_data(dat_file->flash_addr, dat_file->len, NULL, 0)){
		error("Stream failed\n");
		return false;
	}
	if(!ble_slip_execute()){
		error("Execute failed\n");
		return false;
	}
	return true;
}

// max size is 4096 bytes, every type 4096 bytes are transmitted, calculate all data crc.
bool ble_dfu_send_firmware_packet(){
	nrf_dfu_response_select_t response;
	if(!ble_slip_object_select(NRF_DFU_OBJ_TYPE_DATA, &response)){
		error("read info fail\n");
		return false;
	}
	if(response.offset==0){
		debug("Nothing to recover\n");
		goto __start_streaming;
	}
	warning("TODO: Double check me!\n");
	uint32_t expected_crc=crc32_process_ext_flash(bin_file->flash_addr, response.offset);
	uint32_t remainder=response.offset % response.max_size;

	if(expected_crc!=response.crc || remainder ==0){
		debug("Invalid firmware Remove corrupted data.\n");
		// CRC error
        // DFU lib can't deal with recovering straight to an execute commamd,
        // so treat remainder of 0 as a corrupted block too.
		if(remainder!=0){
			response.offset-=remainder;
		}
		else{
			response.offset-=response.max_size;
		}
		warning("TODO: Double check me!\n");
		response.crc=crc32_process_ext_flash(bin_file->flash_addr, response.offset);
		goto __start_streaming;
	}
	if(remainder!=0 && response.offset!=bin_file->len){
		debug("Send rest of the page.\n");
		if(!stream_data(bin_file->flash_addr + response.offset, remainder, &response.crc, response.offset)){
			error("Stream error\n");
			return false;
		}
		response.offset+=remainder;
	}
	if(!ble_slip_execute()){
		error("Execute data fail\n");
		return false;
	}
	__start_streaming:
	{
		debug("start streaming\n");
		uint32_t i=response.offset;
		int data_to_send=0;

		while(i<bin_file->len){
			if(i+response.max_size>bin_file->len)
				data_to_send=(bin_file->len-i);
			else
				data_to_send=response.max_size;
			if(!ble_create_object(NRF_DFU_OBJ_TYPE_DATA, data_to_send)){
				error("Creat object fail\n");
				return false;
			}
			if(!stream_data(bin_file->flash_addr+i, data_to_send, &response.crc, i)){
				error("Stream data fail\n");
				return false;
			}
			if(!ble_slip_execute()){
				error("Execute data fail\n");
				return false;
			}
			i+=data_to_send;
		}
	}
	return true;
}

bool ble_dfu_process(){
	uint32_t tick=millis();

	debug("ble dfu start...\n");
	if(!ble_dfu_start()){
		error("dfu start fail\n");
		return false;
	}
	debug("dfu start ok, mtu: %d, tick: %lu\n", ble_dfu.mtu, millis()-tick);
	tick=millis();
	if(!ble_dfu_send_init_packet()){
		error("dfu send init packet fail\n");
		return false;
	}
	debug("dfu init packet ok, tick %lu\n", millis()-tick);
	tick=millis();
	if(!ble_dfu_send_firmware_packet()){
		error("dfu send firmware packet fail\n");
		return false;
	}
	debug("dfu firmware packet ok, tick %lu\n", millis()-tick);
	return true;
}
