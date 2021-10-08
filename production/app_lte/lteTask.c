/*
 * lteTask.c
 *
 *  Created on: May 13, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4
#include "log_sys.h"
#include "cmsis_os.h"
#include <string.h>
#include "ringbuf/ringbuf.h"
#include "transport_interface.h"
#include "core_mqtt.h"
#include "lte/lara_r2_socket.h"
#include "lte/lara_r2.h"
#include "app_config.h"
#include "iwdg.h"
#include "rtc.h"
#include "app_main/app_info.h"
#include "app_main/app_main.h"
#include "time.h"
#include "app_main/app_rtc.h"

static char _imei[32];
static char _ccid[32];
static int rssi=0;

const int *const lteRssi= &rssi;
const char *const lteImei= &_imei;
const char *const lteCcid= &_ccid;

static bool network_ready=false;

bool network_is_ready(){
	return network_ready;
}

enum{
	ROOT_CA=0,
	DEVICE_CERT=1,
	DEVICE_KEY=2
}cert_type_t;

bool lte_init_rtc(){
	char *response;
	ASSERT_RET(gsm_send_at_command("AT+CCLK?\r\n", "+CCLK: ", 500, 5, &response), false, "AT+CCLK?");
	char *pos=strstr(response, "+CCLK: ");
	if(pos==NULL)
		return false;
	set_rtc_from_at_string(pos);
	return true;
}

static bool network_security_init()
{
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,0,0\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,0,1");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,1,3\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,1,3");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,2,30\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,2,30");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,3,\"TrustCA\"\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,3,\"TrustCA\"");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,5,\"DeviceCert\"\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,5,\"DeviceCert\"");
    ASSERT_RET(gsm_send_at_command("AT+USECPRF=0,6,\"DeviceKey\"\r\n", "OK", 500, 2, NULL), false, "AT+USECPRF=0,6,\"DeviceKey\"");
    return true;
}

bool network_connect_init(void)
{
	ASSERT_RET(gsm_send_at_command("AT+CMEE=2\r\n", "OK", 500, 2, NULL), false, "AT+CMEE=2");
	ASSERT_RET(gsm_send_at_command("AT+URAT=5,3\r\n", "OK", 500, 2, NULL), false, "AT+URAT=5,3");
	ASSERT_RET(gsm_send_at_command("AT+UPSD=0,1,\"TSIOT\"\r\n", "OK", 500, 2, NULL), false, "AT+UPSD=0,1,\"TSIOT\"");
	ASSERT_RET(gsm_send_at_command("AT+UPSD=0,0,0\r\n", "OK", 500, 5, NULL), false, "AT+UPSD=0,0,0");
	ASSERT_RET(gsm_send_at_command("AT+CREG=2\r\n", "OK", 500, 2, NULL), false, "AT+CREG=2");
	ASSERT_RET(gsm_send_at_command("AT+CGREG=2\r\n", "OK", 500, 2, NULL), false, "AT+CGREG=2");
	ASSERT_RET(gsm_send_at_command("AT+CEREG=2\r\n", "OK", 500, 2, NULL), false, "AT+CEREG=2");
	uint32_t tick=millis();
	bool connected=false;
	while(millis()-tick<180000){
		if(gsm_send_at_command("AT+CEREG?\r\n", "+CEREG: 2,5", 500, 1, NULL)){
			debug("Registed to 4G network\n");
			connected=true;
			break;
		}
		if(gsm_send_at_command("AT+CGREG?\r\n", "+CGREG: 2,5", 500, 1, NULL)){
			debug("Registed to GPRS network\n");
			connected=true;
			break;
		}
		delay(1000);
	}
	if(!connected){
		return false;
	}
	ASSERT_RET(gsm_send_at_command("AT+UPSDA=0,3\r\n", "OK", 10000, 10, NULL), false, "AT+UPSDA=0,3");
	ASSERT_RET(gsm_send_at_command("AT+UPSND=0,8\r\n", "OK", 500, 2, NULL), false, "AT+UPSND=0,8");
	ASSERT_RET(gsm_send_at_command("AT+UDCONF=1,1\r\n", "OK", 500, 5, NULL), false, "AT+UDCONF=1,1");
	return true;
}

//hanlde urc and unexpected reponse
int lte_async_response_handle(){
	static char *response;
	if(gsm_at_poll_msg(&response)){
		debugx("msg: %s\n", response);
		if(strstr(response,"+UUSOCL")){
			debug("handle socket close\n");
			lara_r2_socket_urc_handle(response);
		}
		if(strstr(response, "+UUSORD")){
			debug("handle socket data\n");
			lara_r2_socket_urc_handle(response);
		}
		if(strstr(response, "+UUPSDD")){
			debug("handle network close\n");
			lara_r2_socket_urc_handle(response);
			network_ready=false;
		}
	}
	return 1;
}

bool mail_status_reponse(uint8_t type, uint8_t status, osMessageQueueId_t mailBox ){
	uint8_t *st=malloc(1);
	ASSERT_RET(st!=NULL, false, "Malloc");
	*st=status;
	mail_t mail={.type=type, .data=st, .len=1};
	if(osMessageQueuePut(mailBox, &mail, 0, 10)!=osOK){
		error("Mail put failed");
		free(st);
		return false;
	}
	return true;
}

static bool lte_mail_process(){
	static mail_t mail;
	if(osMessageQueueGet(lteMailHandle, &mail, NULL, 0)!=osOK){
		return false;
	}
	debug("Input lte mail\n");
	if(mail.type==MAIL_LTE_NETWORK_RESTART){
		debug("Restart network\n");
		network_ready=false;
	}
	else if(mail.type==MAIL_LTE_HTTP_GET_FILE){
		lte_get_file_t *file_info=(lte_get_file_t *)mail.data;
		if(lara_r2_http_get_file(file_info->link, file_info->info, file_info->sector_timeout)){
			mail_status_reponse(MAIL_LTE_HTTP_GET_FILE, 1, mainMailHandle);
		}
		else{
			mail_status_reponse(MAIL_LTE_HTTP_GET_FILE, 0, mainMailHandle);
		}
		if(!lara_r2_socket_check()){
			network_ready=false;
		}
	}
	__exit:
	if(mail.data!=NULL){
		free(mail.data);
		mail.data=NULL;
	}
	return true;
}

void lte_task(){
#ifdef LTE_ENABLE
	while(!system_is_ready()){
		delay(5);
	}
	info("LTE task start\n");

	lara_r2_bsp_init();

	if(lara_r2_socket_check()){
		lara_r2_init_info(_imei, 32, _ccid, 32);
		goto __network_ready;
	}
	__network_start:
	do{
		debug("Bring up module\n");
		if(!lara_r2_hardware_init()){
			goto __init_wait;
		}
		if(!lara_r2_init_info(_imei, 32, _ccid, 32)){
			goto __init_wait;
		}
		if(!lara_r2_software_init()){
			goto __init_wait;
		}

		if(network_connect_init()){
			break;
		}
		__init_wait:
		delay(60000);
	}while(1);

	__network_ready:
	debug("network ready, let's go\n");
	lte_init_rtc();
	lara_r2_socket_close_all();
	network_security_init();
	network_ready=true;
	while(true){
		lte_async_response_handle();
		lara_r2_socket_process();
		lte_mail_process();
		if(!network_ready){
			goto __network_start;
		}
		delay(5);
	}
#else
	strcpy(_imei, "abcd1234");
	strcpy(_ccid, "12345678");
#endif
}
