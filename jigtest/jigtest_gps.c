 
#include <stdio.h>

#define __DEBUG__ 4
#include <string.h>
#include <stdbool.h>

static struct{
	bool TXT;
	bool GGA;
	bool GLL;
	bool GSA;
	bool VTG;
	bool GSV;
	bool RMC;
}msg_detected;

static void reset_message(){
	memset(&msg_detected, 0, sizeof(msg_detected));
}
static bool all_message_check(){
	return msg_detected.TXT && msg_detected.GGA  && msg_detected.GLL && msg_detected.GSA && msg_detected.VTG && msg_detected.GSV && msg_detected.RMC;
}
static bool gga_only_check(){
	return !msg_detected.TXT && msg_detected.GGA && !msg_detected.GLL && !msg_detected.GSA && !msg_detected.VTG && !msg_detected.GSV && !msg_detected.RMC;
}

bool GPS_EVA_process(){
	static char *frame;
	frame=GPS_EVA_ringbuf_polling();

	if(frame==NULL)
		return false;
	debug("%s\n", frame);
//	char *msg=strstr(frame, "RMC");
//	if(msg!=NULL){
//		parse_rmc(msg-2);
//		msg_detected.RMC=true;
//	}
	char *msg=strstr(frame, "GGA");
	if(msg!=NULL){
		parse_gga(msg-2);
		msg_detected.GGA=true;
	}
	if(strstr(frame, "RMC")) msg_detected.RMC=true;
	if(strstr(frame, "GSA")) msg_detected.GSA=true;
	if(strstr(frame, "GLL")) msg_detected.GLL=true;
	if(strstr(frame, "VTG")) msg_detected.VTG=true;
	if(strstr(frame, "GSV")) msg_detected.GSV=true;
	if(strstr(frame, "TXT")) msg_detected.TXT=true;
//	char *gsa=strstr(frame, "GNGSA");
//	if(gsa!=NULL){
//		parse_gsa(gsa);
//	}
	return true;
}

void testkit_gps_hardware_test()
{
#ifdef GPS_ENABLE
	uint32_t tick;
	bsp_gps_uart_init();
	gps_reset();
	tick=millis();
	reset_message();
	while(millis()-tick<3000){
		GPS_EVA_process();
		delay(5);
	}
	if(!all_message_check()){
		testkit_direct_report(UART_UI_RES_GPS_RESET, 0);
		testkit_direct_report(UART_UI_RES_GPS_TXRX, 0);
		return;
	}
	testkit_direct_report(UART_UI_RES_GPS_RESET, 1);
	GPS_EVA_M8M_disable_messages();
	delay(2000);
	GPS_EVA_ringbuf_reset();
	reset_message();
	tick=millis();
	while(millis()-tick<3000){
		GPS_EVA_process();
		delay(5);
	}
	if(!gga_only_check()){
		testkit_direct_report(UART_UI_RES_GPS_TXRX, 0);
		return;
	}
	testkit_direct_report(UART_UI_RES_GPS_TXRX, 1);
#else
//	testkit_direct_report(UART_UI_RES_GPS_TXRX, 0);
//	testkit_direct_report(UART_UI_RES_GPS_RESET, 0);
#endif
}

// const char valid_gga[] = "$GPGGA,172814.0,3723.46587704,N,12202.26957864,W,2,6,1.2,18.893,M,-25.669,M,2.0,0031*4F\r\n";
// const char empty_gga[] = "$GNGGA,,,,,,0,00,99.99,,,,,,*56\r\n";
// static void gps_unit_test_process()
// {
// 	static uint32_t tick = 0;
// 	static uint32_t count = 0;
// 	if (millis() - tick > 1000)
// 	{
// 		tick = millis();
// 		if (count < 5)
// 		{
// 			RINGBUF_Put_Buff(&gpsRingbuf, empty_gga, strlen(empty_gga));
// 		}
// 		else if (count < 10)
// 		{
// 			RINGBUF_Put_Buff(&gpsRingbuf, valid_gga, strlen(valid_gga));
// 		}
// 		count++;
// 	}
// }
