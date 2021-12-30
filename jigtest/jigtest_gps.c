 
#include <stdio.h>

#define __DEBUG__ 4
#include <string.h>
#include <stdbool.h>
#include "bsp.h"
#include "jigtest.h"
#include "jigtest_gps.h"
#include "protothread/pt.h"
#include "gps/eva_m8.h"

static struct{
	bool TXT;
	bool GGA;
	bool GLL;
	bool GSA;
	bool VTG;
	bool GSV;
	bool RMC;
}msg_detected;

static struct
{
	uint8_t count;
	uint8_t hdop;
}gps_test;

static void reset_message()
{
	memset(&msg_detected, 0, sizeof(msg_detected));
}

static bool all_message_check()
{
	return msg_detected.TXT && msg_detected.GGA  && msg_detected.GLL && msg_detected.GSA && msg_detected.VTG && msg_detected.GSV && msg_detected.RMC;
}

static bool gga_only_check()
{
	return !msg_detected.TXT && msg_detected.GGA && !msg_detected.GLL && !msg_detected.GSA && !msg_detected.VTG && !msg_detected.GSV && !msg_detected.RMC;
}

void eva_m8_data_callback(char *latitude, char *lat_dir, char *longitude, char *long_dir, char *hdop)
{
	if(hdop!=NULL && latitude!=NULL && longitude!=NULL)
	{
//		debug("Gps location: long: %s, lat: %s, hdop: %s\n", longitude, latitude, hdop);
		gps_test.count++;
		float hdop_value;
		sscanf(hdop, "%f", &hdop_value);
		gps_test.hdop=(uint8_t)(hdop_value*10);
	}
}

bool GPS_EVA_process(){
	static char *frame;
	frame=eva_m8_ringbuf_polling();

	if(frame==NULL)
		return false;
	if(strstr(frame, "GGA")) msg_detected.GGA=true;
	if(strstr(frame, "RMC")) msg_detected.RMC=true;
	if(strstr(frame, "GSA")) msg_detected.GSA=true;
	if(strstr(frame, "GLL")) msg_detected.GLL=true;
	if(strstr(frame, "VTG")) msg_detected.VTG=true;
	if(strstr(frame, "GSV")) msg_detected.GSV=true;
	if(strstr(frame, "TXT")) msg_detected.TXT=true;
	return true;
}

/*******************************************************************
 * TEST_FUNCTION
 *******************************************************************/

static task_complete_cb_t function_callback;
static jigtest_timer_t *timer;
static struct pt function_pt;

static int function_test_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	memset(&gps_test, 0, sizeof(gps_test));
	eva_m8_init(eva_m8_data_callback);
	while(millis()-timer->tick < timer->timeout)
	{
		if(gps_test.count >5)
		{
			jigtest_direct_report(UART_UI_RES_GPS_POSITION, 1);
			jigtest_direct_report(UART_UI_RES_GPS_HDOP, gps_test.hdop);
			break;
		}
		eva_m8_process();
		PT_YIELD(pt);
	}
	function_callback();
	PT_END(pt);
}

void jigtest_gps_function_init(task_complete_cb_t cb, void *params)
{
	function_callback=cb;
	timer=(jigtest_timer_t *)params;
	PT_INIT(&function_pt);
}

void jigtest_gps_function_process(void)
{
	function_test_thread(&function_pt);
}

/*******************************************************************
 * TEST_HARDWARE
 *******************************************************************/

static struct pt hardware_pt, init_pt;
static task_complete_cb_t hardware_callback;

static int hardware_test_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	static uint32_t tick;
	PT_SPAWN(pt, &init_pt, eva_m8_init_proto(&init_pt, eva_m8_data_callback));
	PT_DELAY(pt, tick, 2000);
	eva_m8_buffer_reset();
	tick=millis();
	while(millis()-tick<3000)
	{
		GPS_EVA_process();
		PT_YIELD(pt);
	}
	if(!gga_only_check())
	{
		goto __exit;
	}
	jigtest_direct_report(UART_UI_RES_GPS_TXRX, 1);
	eva_m8_reset();
	//after reset, all nmea messages must be presented
	tick=millis();
	while(millis()-tick<3000)
	{
		GPS_EVA_process();
		PT_YIELD(pt);
	}
	if(!all_message_check()){
		goto __exit;
	}
	jigtest_direct_report(UART_UI_RES_GPS_RESET, 1);
	__exit:
	hardware_callback();
	PT_END(pt);
}

void jigtest_gps_hardware_init(task_complete_cb_t cb, void *params)
{
	hardware_callback=cb;
	PT_INIT(&hardware_pt);
	reset_message();
}

void jigtest_gps_hardware_process(void)
{
	hardware_test_thread(&hardware_pt);
}

void jigtest_gps_console_handle(char *result)
{
//	if(__check_cmd("test hardware"))
//	{
//		bool tx_rx, reset;
//		testkit_gps_test_hardware(&tx_rx, &reset);
//		debug("Test result tx_rx: %d, reset: %d\n", tx_rx, reset);
//	}
//	else if(__check_cmd("test function "))
//	{
//		int timeout;
//		if(sscanf(__param_pos("test function "), "%d", &timeout)==1)
//		{
//			if(jigtest_gps_function_test(timeout))
//			{
//				debug("Test function ok\n");
//			}
//			else
//			{
//				debug("Test function error\n");
//			}
//		}
//		else debug("Param error\n");
//	}
//	else debug("Unknown cmd\n");
}

