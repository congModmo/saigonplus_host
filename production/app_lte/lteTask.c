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
#include "lte/lara_r2_http.h"
#include "app_config.h"
#include "iwdg.h"
#include "rtc.h"
#include "app_main/app_info.h"
#include "app_main/app_main.h"
#include "time.h"
#include "app_main/app_rtc.h"
#include "lteTask.h"

static __IO bool network_ready=false;

static __IO lte_info_t info;
__IO const lte_info_t *const lte_info=&info;

bool network_is_ready()
{
	return network_ready;
}

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

static bool try_to_register_lte()
{
    uint32_t tick;
    info("Try to register to lte\n");
    ASSERT_GO(gsm_send_at_command("AT+CFUN=0\r\n", "OK", 2000, 3, NULL), false, "AT+CFUN=0");
    ASSERT_GO(gsm_send_at_command("AT+URAT=3\r\n", "OK", 1000, 2, NULL), false, "AT+URAT=3");
    ASSERT_GO(gsm_send_at_command("AT+CFUN=1\r\n", "OK", 1000, 2, NULL), false, "AT+CFUN=1");
    ASSERT_GO(gsm_send_at_command("AT+CFUN=16\r\n", "OK", 1000, 2, NULL), false, "AT+CFUN=16");
    tick=millis();
    do{
		lara_r2_get_network_info(info.carrier, sizeof(info.carrier)-1, &info.type);
		if(info.type==NETWORK_TYPE_4G)
		{
			info("Success connect to 4G");
			break;
		}
		delay(2000);
    }
    while(millis()-tick<180000);

    __exit:
    ASSERT_RET(gsm_send_at_command("AT+CFUN=0\r\n", "OK", 2000, 2, NULL), false, "AT+USECPRF=0");
    ASSERT_RET(gsm_send_at_command("AT+URAT=5,3\r\n", "OK", 1000, 2, NULL), false, "AT+URAT=5,3");
    ASSERT_RET(gsm_send_at_command("AT+CFUN=1\r\n", "OK", 1000, 2, NULL), false, "AT+CFUN=1");
    ASSERT_RET(gsm_send_at_command("AT+CFUN=16\r\n", "OK", 1000, 2, NULL), false, "AT+CFUN=16");
    return true;
}

bool network_connect_init(void)
{
	ASSERT_RET(gsm_send_at_command("AT+CMEE=2\r\n", "OK", 500, 2, NULL), false, "AT+CMEE=2");
//#ifdef JIGTEST
//	ASSERT_RET(gsm_send_at_command("AT+URAT=3\r\n", "OK", 500, 2, NULL), false, "AT+URAT=3");
//#else
//	ASSERT_RET(gsm_send_at_command("AT+URAT=5,3\r\n", "OK", 500, 2, NULL), false, "AT+URAT=5,3");
//#endif
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
			info.type=NETWORK_TYPE_4G;
			break;
		}
		if(gsm_send_at_command("AT+CGREG?\r\n", "+CGREG: 2,5", 500, 1, NULL)){
			debug("Registed to GPRS network\n");
			connected=true;
			info.type=NETWORK_TYPE_2G;
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
int lte_async_response_handle()
{
	static char *response;
	if(gsm_at_poll_msg(&response))
	{
		debug("URC: %s\n", response);
		if(strstr(response,"+UUSOCL"))
		{
			debug("handle socket close\n");
			lara_r2_socket_urc_handle(response);
		}
		if(strstr(response, "+UUSORD"))
		{
			debug("handle socket data\n");
			lara_r2_socket_urc_handle(response);
		}
		if(strstr(response, "+UUPSDD"))
		{
			debug("handle network close\n");
			lara_r2_socket_urc_handle(response);
			network_ready=false;
		}
		if(strstr(response, "+CEREG: 5"))
		{
			info.type=NETWORK_TYPE_4G;
		}
		if(strstr(response, "+CGREG: 5"))
		{
			info.type=NETWORK_TYPE_2G;
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

static void get_network_info()
{
	lara_r2_get_network_info(info.carrier, sizeof(info.carrier)-1, &info.type);
	int temp;
	lara_r2_get_network_csq(&temp);
	info.rssi=(uint8_t)temp;
	if(info.type==NETWORK_TYPE_4G)
	{
		info.rssi |= 1<<7;
	}
}

void lte_network_info_handle()
{
	static uint32_t tick=0;
	if(millis()-tick < 60000)
		return;
	tick=millis();
	get_network_info();
}

void lte_rtc_handle()
{
	static uint32_t tick=0;
	if(millis()-tick>3600000)
	{
		tick=millis();
		lte_init_rtc();
	}
}

static void jigtest_init_hardware()
{
	network_ready=false;
	memset((void *)&info, 0, sizeof(lte_info_t));
	lara_r2_bsp_init();
	if(!lara_r2_hardware_init())
	{
		return;
	}
	if(!lara_r2_software_init())
	{
		return;
	}
	if(!lara_r2_init_info((char *)info.imei, 32, (char *)info.ccid, 32))
	{
		return;
	}
	info.key=lara_r2_check_key();
	info.hardware_init=true;
}

static void jigtest_init_network()
{
	info.ready=false;
	info.type=NETWORK_TYPE_NONE;
	lara_r2_bsp_init();
	try_to_register_lte();
	lara_r2_software_init();

	network_connect_init();
	delay(2000);
	if(lte_info->type!=NETWORK_TYPE_NONE)
	{
		get_network_info();
		network_security_init();
		lara_r2_socket_close_all();
		network_ready=true;
		info.ready=true;
	}
}

static bool lte_mail_process(void)
{
	static mail_t mail;
	if(osMessageQueueGet(lteMailHandle, &mail, NULL, 0)!=osOK)
	{
		return false;
	}
	debug("Input lte mail\n");
	if(mail.type==MAIL_LTE_NETWORK_RESTART)
	{
		debug("Restart network\n");
		network_ready=false;
	}
	else if(mail.type==MAIL_LTE_HTTP_GET_FILE)
	{
		lte_get_file_t *file_info=(lte_get_file_t *)mail.data;
		if(lara_r2_http_get_file(file_info->link, file_info->info, file_info->sector_timeout))
		{
			mail_status_reponse(MAIL_LTE_HTTP_GET_FILE, 1, mainMailHandle);
		}
		else
		{
			mail_status_reponse(MAIL_LTE_HTTP_GET_FILE, 0, mainMailHandle);
		}
		if(!lara_r2_socket_check())
		{
			network_ready=false;
		}
	}
	else if(mail.type==MAIL_LTE_INIT_HARDWARE)
	{
		jigtest_init_hardware();
	}
	else if(mail.type==MAIL_LTE_INIT_NETWORK)
	{
		jigtest_init_network();
	}
	__exit:
	if(mail.data!=NULL)
	{
		free(mail.data);
		mail.data=NULL;
	}
	return true;
}

#ifdef JIGTEST
void jigtest_lte_task()
{
	while(true)
	{
		lte_mail_process();
		lara_r2_socket_process();
		lte_async_response_handle();
		delay(5);
	}
}
#endif

void lte_task(void)
{
	while(!system_is_ready())
	{
		delay(5);
	}
	info("LTE task start\n");
	bool lte_attempt=false;
	lara_r2_bsp_init();

	if(lara_r2_socket_check())
	{
		lara_r2_init_info(info.imei, 32, info.ccid, 32);
		get_network_info();
		goto __network_ready;
	}
	__network_start:
	do{
		debug("Bring up module\n");
		if(!lara_r2_hardware_init())
		{
			goto __init_wait;
		}
		if(!lara_r2_init_info(info.imei, 32, info.ccid, 32))
		{
			goto __init_wait;
		}
		if(!lara_r2_software_init())
		{
			goto __init_wait;
		}
		if(network_connect_init())
		{
			if(info.type==NETWORK_TYPE_2G && !lte_attempt)
			{
				lte_attempt=true;
				try_to_register_lte();
				goto __network_start;
			}
			else
			{
				break;
			}
		}
		__init_wait:
		delay(60000);
	}while(1);

	__network_ready:
	debug("network ready, let's go\n");
	lte_init_rtc();
	lara_r2_socket_close_all();
	network_security_init();
	get_network_info();
	network_ready=true;
	while(true)
	{
		lte_rtc_handle();
		lte_network_info_handle();
		lte_async_response_handle();
		lara_r2_socket_process();
		lte_mail_process();
		if(!network_ready){
			goto __network_start;
		}
		delay(5);
	}
}
