
#define __DEBUG__ 4
#include <string.h>
#include <app_gps/app_gps.h>
#include <app_common/common.h>
#include "Define.h"
#include <gps/eva_m8.h>
#include "kalman_filter/kalman_filter.h"
#include "log_sys.h"
#include "math.h"
#include "FreeRTOS.h"

typedef struct{
	uint8_t len;
	uint8_t count;
	uint8_t head;
	double *buff;
}avg_queue_t;

static uint32_t gps_tick=0;
static gps_data_t gps={0};
const gps_data_t * const gps_data =&gps;

//extern osMutexId appResourceHandle;
//static kalman_t long_filter;
//static kalman_t lat_filter;

static double lat_buff[5]={0};
static double long_buff[5]={0};
static avg_queue_t lat_avg, long_avg;

void avg_queue_init(avg_queue_t *avg, double *buff, uint8_t len){
	avg->len=len;
	avg->buff=buff;
	avg->head=0;
	avg->count=0;
}

double avg_queue_put(avg_queue_t *avg, double value){
	avg->buff[avg->head]=value;
	avg->head++;
	if(avg->head==avg->len){
		avg->head=0;
	}
	avg->count++;
	if(avg->count >avg->len){
		avg->count=avg->len;
	}
	double sum=0;
	for(uint8_t i=0; i<avg->count; i++){
		sum+=avg->buff[i];
	}
	return sum/avg->count;
}

// 2058.22989
// DDMM.MMMMM
void lat_string_convert(double _lat, char *des){
	if(_lat<10.0){
		sprintf(des, "0%lf", _lat);
	}
	else{
		sprintf(des, "%lf", _lat);
	}
}

double lat_long_string_to_double(char *source){
	double _lat, _degrees, _minutes;
	sscanf(source, "%lf", &_lat);
	_degrees = trunc(_lat / 100.0f);
	_minutes = _lat - (_degrees * 100.0f);
	_lat = _degrees + _minutes / 60.0f;
	return _lat;
}

// 10547.23526
// DDDMM.MMMMM
void long_string_convert(double _long, char *des){
	if(_long<10.0){
		sprintf(des, "000%lf", _long);
	}
	else if(_long<100.0){
		sprintf(des, "00%lf", _long);
	}
	else{
		sprintf(des, "0%lf", _long);
	}
}

//void kalman_callback(char *latitude, char *lat_dir, char *longitude, char *long_dir){
//	double lat=lat_string_to_double(latitude);
//	debugx("%lf,%lf\n", lat, updateEstimate(&lat_filter, lat));
//}

void average_callback(char *latitude, char *lat_dir, char *longitude, char *long_dir, char *hdop){
	if(hdop==NULL){
		gps.hdop=0;
		gps.latitude=0;
		gps.longitude=0;
		return;
	}
	sscanf(hdop, "%f", &gps.hdop);
	gps.latitude=avg_queue_put(&lat_avg, lat_long_string_to_double(latitude) * ((*lat_dir=='S')?-1:1) );
	gps.longitude=avg_queue_put(&long_avg, lat_long_string_to_double(longitude) * ((*long_dir=='W')?-1:1));
//	eva_m8_data_pack(f_latidue, lat_dir, f_longitude, long_dir);
//	debugx("%lf,%lf\n", lat, avg_queue_put(&lat_avg, lat));
}

void gps_test_double()
{
	avg_queue_init(&lat_avg, lat_buff, 5);
	avg_queue_init(&long_avg, long_buff, 5);
	uint32_t tick=millis();
	for(uint8_t i=0; i<100; i++)
	{
		average_callback("3723.46587704","N","12202.26957864","W", "1.0");
	}
	debug("time: %u\n", millis()-tick);
}

void wcb_gps_init(void)
{
	avg_queue_init(&lat_avg, lat_buff, 5);
	avg_queue_init(&long_avg, long_buff, 5);
	eva_m8_init(average_callback);
}

void wcb_gps_process(void)
{
	if(millis()-gps_tick >WCB_RESET_GPS_TIME){
		gps_tick=millis();
		eva_m8_reset();
		wcb_gps_init();
	}
	eva_m8_process();
}
