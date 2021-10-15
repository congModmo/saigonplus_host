 
#include <stdio.h>

#define __DEBUG__ 4
#include <string.h>
#include <stdbool.h>
#include "bsp.h"

static struct{
	bool TXT;
	bool GGA;
	bool GLL;
	bool GSA;
	bool VTG;
	bool GSV;
	bool RMC;
}msg_detected;

bool gps_location_detected=false;

static void reset_message()
{
	memset(&msg_detected, 0, sizeof(msg_detected));
}

static bool all_message_check()
{
	return msg_detected.TXT && msg_detected.GGA  && msg_detected.GLL && msg_detected.GSA && msg_detected.VTG && msg_detected.GSV && msg_detected.RMC;
}

static bool gga_only_check(){
	return !msg_detected.TXT && msg_detected.GGA && !msg_detected.GLL && !msg_detected.GSA && !msg_detected.VTG && !msg_detected.GSV && !msg_detected.RMC;
}

void eva_m8_data_callback(char *latitude, char *lat_dir, char *longitude, char *long_dir, char *hdop)
{
	if(hdop!=NULL && latitude!=NULL && longitude!=NULL)
	{
		debug("Gps location: long: %s, lat: %s, hdop: %s\n", longitude, latitude, hdop);
		gps_location_detected=true;
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

void testkit_gps_test_hardware(bool *tx_rx, bool *reset)
{
	eva_m8_init(eva_m8_data_callback);
	eva_m8_reset();
	reset_message();
	uint32_t tick =millis();
	while(millis()-tick<3000){
		GPS_EVA_process();
		delay(5);
	}
	//after reset, all nmea messages must present
	if(!all_message_check()){
		*tx_rx=false;
		*reset=false;
		return;
	}
	*reset=1;
	eva_m8_disable_messages();
	delay(2000);
	//reset eva m8 buffer
	while(eva_m8_ringbuf_polling()!=NULL)
	{

	}
	reset_message();
	tick=millis();
	while(millis()-tick<3000){
		GPS_EVA_process();
		delay(5);
	}
	if(!gga_only_check()){
		*tx_rx=0;
		return;
	}
	*tx_rx=1;
}

bool jigtest_gps_test_function(int timeout)
{
	uint32_t tick=millis();
	while(millis()-tick<timeout && ! gps_location_detected)
	{
		eva_m8_process();
		delay(5);
	}
	return gps_location_detected;
}

void jigtest_gps_console_handle(char *result)
{
	if(__check_cmd("test hardware"))
	{
		bool tx_rx, reset;
		testkit_gps_test_hardware(&tx_rx, &reset);
		debug("Test result tx_rx: %d, reset: %d\n", tx_rx, reset);
	}
	else if(__check_cmd("test function "))
	{
		int timeout;
		if(sscanf(__param_pos("test function "), "%d", &timeout)==1)
		{
			if(jigtest_gps_test_function(timeout))
			{
				debug("Test function ok\n");
			}
			else
			{
				debug("Test function error\n");
			}
		}
		else debug("Param error\n");
	}
	else debug("Unknown cmd\n");
}

