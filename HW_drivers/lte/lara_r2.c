
#define __DEBUG__ 3

#include <lte/lara_r2.h>
#include "bsp.h"
#include "log_sys.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "ringbuf/ringbuf.h"
//#include <app_common/wcb_common.h>
//#include "app_main/app_info.h"

static RINGBUF lte_rb;

uint32_t convert_modem_timestring_to_timestamp(char *time);

static GSM_AT_Resp_t atResp;

void lara_r2_reset_control(uint8_t state)
{
	HAL_GPIO_WritePin(GSM_RESET_GPIO_Port, GSM_RESET_Pin, (state==0)?GPIO_PIN_RESET:GPIO_PIN_SET);
}

void lara_r2_power_control(uint8_t state)
{
	HAL_GPIO_WritePin(GSM_POWER_GPIO_Port, GSM_POWER_Pin, (state==0)?GPIO_PIN_RESET:GPIO_PIN_SET);
}

bool lara_r2_setup_power(void)
{
//	lara_r2_reset_control(1);
	info("Force GSM POWER ON\n");
	lara_r2_reset_control(1);
	delay(1000);
	lara_r2_reset_control(0);
	delay(1000);
	if(gsm_send_at_command("AT\r\n", "OK", 2000, 7, NULL))
	{
		info("GSM POWER ON SUCCESS\r\n");
		return true;
	}
	error("GSM POWER ON FAILED\n");
	return false;
}

extern osMessageQId lteResponseHandle;
void gsm_clear_buffer(void)
{
	RINGBUF_Flush(&lte_rb);
}

void gsm_at_init_resp(void)
{
	atResp.n = 0;
	atResp.buf[0] = 0;
}

void gsm_at_read_resp(int timeout)
{
	delay(timeout);
	while(RINGBUF_Available(&lte_rb) && (atResp.n <LARA_R2_BUFF_SIZE)){
		static uint8_t c;
		RINGBUF_Get(&lte_rb, &c);
		atResp.buf[atResp.n++]=c;
	}
	atResp.buf[atResp.n]='\0';
}

bool gsm_at_poll_msg(char **resp)
{
	gsm_at_init_resp();
	while(RINGBUF_Available(&lte_rb) && (atResp.n <LARA_R2_BUFF_SIZE)){
		static uint8_t c;
		RINGBUF_Get(&lte_rb, &c);
		atResp.buf[atResp.n++]=c;
	}
	atResp.buf[atResp.n]='\0';
	if(atResp.n>0 && strstr(atResp.buf, "\r\n")!=NULL){
		if(resp!=NULL){
			*resp=atResp.buf;
		}
		return true;
	}
	return false;
}
static bool wait_return(const char *resOk, int timeout){
	int at_len=0;
	uint32_t tick=millis();
	while((millis()-tick) < timeout){
		gsm_at_read_resp(10);
		if(atResp.n > 0)
		{
			if(atResp.n >at_len){
				debug("%s\n", atResp.buf);
				at_len=atResp.n;
			}
			if (strstr(atResp.buf, resOk) != NULL){
				return true;
			}
		}
	}
	return false;
}

bool gsm_send_at_command(char *atCommand, const char *resOk, int timeout, int retry, char **resp)
{
	while(retry--)
	{
		gsm_clear_buffer();
		gsm_at_init_resp();
		gsm_send_string(atCommand);
		debug("AT send: %s\n", atCommand);
		if(wait_return(resOk, timeout)){
			if(resp!=NULL){
				*resp=atResp.buf;
			}
			return true;
		}

	}
	return false;
}

int gsm_send_at_command3(char *atCommand, const char *resOk, int timeout, int retry, char **resp)
{
	uint32_t tick;
	while(retry--)
	{
		tick=millis();
		gsm_clear_buffer();
		gsm_at_init_resp();
		gsm_send_string(atCommand);
		debug("AT send: %s\n", atCommand);
		while(millis()-tick<timeout){
			gsm_at_read_resp(10);
			if(atResp.n>strlen(resOk)){
				if(0==memcmp(atResp.buf+atResp.n-strlen(resOk), resOk, strlen(resOk))){
					if(resp!=NULL){
						*resp=atResp.buf;
					}
					return atResp.n;
				}
			}
		}
	}
	return 0;
}

bool gsm_send_at_command2(char **atCommand, size_t sz, const char *resOk, int timeout, int retry, char **resp)
{
	char **cmd;
	size_t szz;
	while(retry--)
	{
		cmd = atCommand;
		szz = sz;
		gsm_clear_buffer();
		gsm_at_init_resp();
		debug("AT send: ");
		while(szz--){
			gsm_send_string(*cmd);
			debugx(*cmd);
			cmd++;
		}
		if(wait_return(resOk, timeout)){
			if(resp!=NULL)
				*resp=atResp.buf;
			return true;
		}
	}
	return false;
}

bool lara_r2_get_imei(char *_imei_str, uint16_t maxLen)
{
	char *ret;
	char *str;
	char *buf ;

	if(!gsm_send_at_command("AT+GSN\r\n", "OK", 1000, 3, &buf )){
		return false;
	}
	debug("AT+GSN ==> %s", buf);
	ret = buf;
	// search for number string
	while(!isdigit(*ret) && (*ret != 0)) ret++;
	if (*ret == 0) return false;
	str = _imei_str;
	while (maxLen--){
		if (isdigit(*ret)){
			*str++ = *ret++;
		}
		else {
			*str = 0;
			return true;
		}
	}
	if (isdigit(*ret)){
		// If ret still contains valid number ==> overflow / invalid ==> ignore it!
		_imei_str[0] = 0;
	}
	else {
		// If not, maxLen is fit ==> valid number string
		_imei_str[maxLen - 1] = 0;
		return true;
	}
	return false;
}

uint8_t lara_r2_get_ccid(char _ccid[], uint16_t maxLen)
{
	char *ret;
	char *str;
	char *buf = atResp.buf;

	if(!gsm_send_at_command("AT+CCID\r\n", "+CCID: ", 1000, 3, &buf )){
		return false;
	}
	ret = NULL;
	if((ret = strstr(buf, "+CCID: ")) != NULL)
	{
		ret += strlen("+CCID: ");
		// search for number string
		while(!isdigit(*ret) && (*ret != 0)) ret++;
		if (*ret == 0) return false;
		str = _ccid;
		while (maxLen--){
			if (isdigit(*ret)){
				*str++ = *ret++;
			}
			else {
				*str = 0;
				return true;
			}
		}
		if (isdigit(*ret)){
			// If ret still contains valid number ==> overflow / invalid ==> ignore it!
			_ccid[0] = 0;
		}
		else {
			// If not, maxLen is fit ==> valid number string
			_ccid[maxLen - 1] = 0;
			return true;
		}
	}
	return false;
}

bool lara_r2_init_info(char * imei, int imei_len, char* ccid, int ccid_len){
	ASSERT_RET(lara_r2_get_imei(imei,imei_len), false, "Process imei");
	ASSERT_RET(lara_r2_get_ccid(ccid, ccid_len), false, "get ccid");
	info("ccid: %s\n", ccid);
	info("imei: %s\n", imei);
	return true;
}

bool lara_r2_get_network_csq(int *csq)
{
	char *response;
	ASSERT_RET(gsm_send_at_command("AT+CSQ\r\n", "+CSQ: ", 1000, 2, &response), false, "AT+CSQ?");
	response=strstr(response, "+CSQ: ")+strlen("+CSQ: ");
	ASSERT_RET(sscanf(response, "%d,", csq)==1, false, "Sscanf");
	debug("csq: %s\n", *csq);
	return true;
}

bool lara_r2_get_network_info(char *carrier, int carrier_len, int *csq)
{
	char *response;
	ASSERT_RET(gsm_send_at_command("AT+COPS?\r\n", "OK", 1000, 2, &response), false, "AT+COPS?");
	char *_carrier=strtok(response, "\"");
	_carrier=strtok(NULL, "\"");
	ASSERT_RET(strlen(_carrier)>0 && strlen(_carrier)<carrier_len, false, "Carrier len");
	strcpy(carrier, _carrier);
	debug("Carrier: %s\n", carrier);
	ASSERT_RET(lara_r2_get_network_csq(csq), false, "Get csq");
	return true;
}

bool lara_r2_bsp_init(void)
{
	uart_lte_bsp_init(&lte_rb);
	return true;
}

bool lara_r2_hardware_init(void)
{
	return lara_r2_setup_power();
}

bool lara_r2_software_init(void)
{
	ASSERT_RET(gsm_send_at_command("AT\r\n",                "OK", 1000, 4, NULL), false, "AT");
	ASSERT_RET(gsm_send_at_command("ATZ\r\n",               "OK", 1000, 4, NULL), false, "ATZ");
	ASSERT_RET(gsm_send_at_command("ATE0\r\n",              "OK", 1000, 4, NULL), false, "ATE0");
	ASSERT_RET(gsm_send_at_command("AT+CPIN?\r\n",          "+CPIN: READY", 1000, 4, NULL), false, "AT+CPIN?");
	ASSERT_RET(gsm_send_at_command("AT&W\r\n",              "OK", 1000, 4, NULL), false, "AT&W");
	return true;
}

bool lara_r2_init()
{
	if(!lara_r2_hardware_init()){
		return false;
	}
	if(!lara_r2_software_init()){
		return false;
	}
	return true;
}

bool lara_r2_import_key(char *key, char *name, int type ){
	char cmd[64];
	sprintf(cmd,"AT+USECMNG=0,%d,\"%s\",%d\r\n", type, name, strlen(key));
	ASSERT_RET(gsm_send_at_command(cmd, ">", 500, 2, NULL), false,"AT+USECMNG" );
	ASSERT_RET(gsm_send_at_command(key, "OK", 500, 2, NULL), false, "Cert string");
	debug("Import cert ok\n");
	return true;
}
