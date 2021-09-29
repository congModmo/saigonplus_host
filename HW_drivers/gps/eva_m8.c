
#include <stdio.h>

#define __DEBUG__ 4
#include <string.h>
#include <stdbool.h>

#include <gps/eva_m8.h>
#include "bsp.h"
#include "log_sys.h"
#include "utils/utils.h"
#include "app_config.h"
static RINGBUF gpsRingbuf;
//static uint8_t GGA_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x00, 0x00, 0xFA, 0x0F};
static uint8_t GLL_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01, 0x00, 0xFB, 0x11};
static uint8_t GSA_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x02, 0x00, 0xFC, 0x13};
static uint8_t VTG_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x05, 0x00, 0xFF, 0x19};
static uint8_t GSV_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x03, 0x00, 0xFD, 0x15};
static uint8_t RMC_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x00, 0xFE, 0x17};
/*
 *
 */
static eva_m8_data_callback_t callback=NULL;
#define NMEA_BUF_SIZE 128


static __IO bool gps_processing=false;


void eva_m8_M8M_disable_messages(void)
{
	info("GPS disable NMEA packet keep GGA\r\n");
//	delay(500);
//	eva_m8_send_data(GGA_MSG, 11);
	delay(500);
	eva_m8_send_data(GSA_MSG, 11);
	delay(500);
	eva_m8_send_data(GLL_MSG, 11);
	delay(500);
	eva_m8_send_data(VTG_MSG, 11);
	delay(500);
	eva_m8_send_data(GSV_MSG, 11);
	delay(500);
	eva_m8_send_data(RMC_MSG, 11);
	delay(500);
	info("GPS disable NMEA keep GGA success\r\n");
}

char * eva_m8_ringbuf_polling(){
	static char nmea_buf[NMEA_BUF_SIZE];
	static __IO uint16_t nmea_buf_idx=0;
	static char c;
	while(RINGBUF_Available(&gpsRingbuf)){
		RINGBUF_Get(&gpsRingbuf, (uint8_t *)&c);
		nmea_buf[nmea_buf_idx]=c;
		if(c=='\n'){
			nmea_buf[nmea_buf_idx]='\0';
			nmea_buf_idx=0;
			return nmea_buf;
		}
		else{
			nmea_buf_idx++;
			if(nmea_buf_idx==NMEA_BUF_SIZE){
				nmea_buf_idx=0;
			}
		}
	}
	return NULL;
}

static bool gps_frame_checksum(char *frame){
    //char frame[]="$GNRMC,200753.00,A,2058.22989,N,10547.23526,E,0.019,,270920,,,A*6B";
    char *asterisk=NULL;
    uint frame_check_sum;
    uint8_t checksum=0;
    asterisk=strstr(frame, "*");
    if(asterisk==NULL || asterisk<=frame){
        return false;
    }

    for(uint32_t i=(uint32_t)frame; i<(uint32_t)asterisk; i++){
        checksum=checksum^*((uint8_t *)i);
    }
    asterisk++;
    if(sscanf(asterisk, "%x", &frame_check_sum)!=1){
        return false;
    }
    if(frame_check_sum!=checksum){
        return false;
    }
    return true;
}

void parse_gga(char *frame){
	debug("Frame: %s\n", frame);
	if(!gps_frame_checksum(frame)){
		return;
	}
	static char *token[15];
	static size_t tokenNum;
	tokenNum = str_token(frame, ",", token, 15);
//	if (tokenNum != 14){
//		return;
//	}
	if(callback!=NULL){
		callback(token[2], token[3], token[4], token[5], token[8]);
	}
//	float hdop=0;
//	if(sscanf(token[8], "%f", &hdop)!=1){
//		hdop=0;
//	}
}

const char valid_gga[]="$GPGGA,172814.0,3723.46587704,N,12202.26957864,W,2,6,1.2,18.893,M,-25.669,M,2.0,0031*4F\r\n";
const char empty_gga[]="$GNGGA,,,,,,0,00,99.99,,,,,,*56\r\n";

static void gps_unit_test_process()
{
	static uint32_t tick=0;
	static uint32_t count=0;
	if(millis()-tick>1000){
		tick=millis();
		if(count < 5){
			RINGBUF_Put_Buff(&gpsRingbuf, empty_gga, strlen(empty_gga));
		}
		else if(count<10)
		{
			RINGBUF_Put_Buff(&gpsRingbuf, valid_gga, strlen(valid_gga));
		}
		count++;
	}
}

void eva_m8_init(eva_m8_data_callback_t cb)
{
	callback=cb;
#ifdef GPS_UNIT_TEST
// use simulated data in stead
	static uint8_t gps_rb_buff[128];
	RINGBUF_Init(&gpsRingbuf, gps_rb_buff, 128);
#else
	eva_m8_M8M_disable_messages();
	bsp_gps_uart_init(&gpsRingbuf);
#endif
}

void eva_m8_process(){
	static char *frame;
#ifdef GPS_UNIT_TEST
	gps_unit_test_process();
#endif
	frame=eva_m8_ringbuf_polling();

	if(frame==NULL)
		return;

	char *msg=strstr(frame, "GGA");
	if(msg!=NULL)
		parse_gga(msg-2);
}
