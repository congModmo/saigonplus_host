
#include <stdio.h>

#define __DEBUG__ 3
#include <string.h>
#include <stdbool.h>

#include <gps/eva_m8.h>
#include "bsp.h"
#include "log_sys.h"
#include "utils/utils.h"
#include "app_config.h"

#define NMEA_BUF_SIZE 128

static RINGBUF gpsRingbuf;
static char nmea_buf[NMEA_BUF_SIZE];
static __IO uint16_t nmea_buf_idx = 0;
static eva_m8_data_callback_t callback = NULL;

uint8_t GLL_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01, 0x00, 0xFB, 0x11};
uint8_t GSA_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x02, 0x00, 0xFC, 0x13};
uint8_t VTG_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x05, 0x00, 0xFF, 0x19};
uint8_t GSV_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x03, 0x00, 0xFD, 0x15};
uint8_t RMC_MSG[11] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x00, 0xFE, 0x17};

void eva_m8_disable_messages(void)
{
	//	delay(500);
	//	eva_m8_send_data(GGA_MSG, 11);
	delay(500);
	eva_m8_bsp_send_data(GSA_MSG, 11);
	delay(500);
	eva_m8_bsp_send_data(GLL_MSG, 11);
	delay(500);
	eva_m8_bsp_send_data(VTG_MSG, 11);
	delay(500);
	eva_m8_bsp_send_data(GSV_MSG, 11);
	delay(500);
	eva_m8_bsp_send_data(RMC_MSG, 11);
	delay(500);
//	info("GPS disable NMEA keep GGA success\r\n");
}

void eva_m8_buffer_reset()
{
	RINGBUF_Flush(&gpsRingbuf);
	memset(nmea_buf, 0, sizeof(nmea_buf));
	nmea_buf_idx=0;
}

char *eva_m8_ringbuf_polling()
{
	static char c;
	while (RINGBUF_Available(&gpsRingbuf))
	{
		RINGBUF_Get(&gpsRingbuf, (uint8_t *)&c);
		nmea_buf[nmea_buf_idx] = c;
		if (c == '\n')
		{
			nmea_buf[nmea_buf_idx] = '\0';
			nmea_buf_idx = 0;
			return nmea_buf;
		}
		else
		{
			nmea_buf_idx++;
			if (nmea_buf_idx == NMEA_BUF_SIZE)
			{
				nmea_buf_idx = 0;
			}
		}
	}
	return NULL;
}

bool gps_frame_checksum(char *frame)
{
	//char frame[]="$GNRMC,200753.00,A,2058.22989,N,10547.23526,E,0.019,,270920,,,A*6B";
	char *asterisk = NULL;
	uint frame_check_sum;
	uint8_t checksum = 0;
	asterisk = strstr(frame, "*");
	if (asterisk == NULL || asterisk <= frame)
	{
		return false;
	}
	for (uint32_t i = (uint32_t)frame; i < (uint32_t)asterisk; i++)
	{
		checksum = checksum ^ *((uint8_t *)i);
	}
	asterisk++;
	if (sscanf(asterisk, "%x", &frame_check_sum) != 1)
	{
		return false;
	}
	if (frame_check_sum != checksum)
	{
		return false;
	}
	return true;
}

void parse_gga(char *frame)
{
	debug("Frame: %s\n", frame);
	if (!gps_frame_checksum(frame))
	{
		return;
	}
	static char *token[15];
	int tokenNum = str_token(frame, ",", token, 15);
	if (callback != NULL)
	{
		if(tokenNum<14)
			callback(NULL, NULL, NULL, NULL, NULL);
		else
			callback(token[2], token[3], token[4], token[5], token[8]);
	}
}
void eva_m8_bsp_init()
{
	eva_m8_bsp_uart_init(&gpsRingbuf);
}

void eva_m8_init(eva_m8_data_callback_t cb)
{
	callback = cb;
	eva_m8_disable_messages();
	eva_m8_bsp_init();
}

void eva_m8_process()
{
	static char *frame;
	frame = eva_m8_ringbuf_polling();
	if (frame == NULL)
		return;
	char *msg = strstr(frame, "GGA");
	if (msg != NULL)
		parse_gga(msg - 2);
}

void eva_m8_reset()
{
	eva_m8_bsp_set_reset_pin(0);
	delay(100);
	eva_m8_bsp_set_reset_pin(1);
}
