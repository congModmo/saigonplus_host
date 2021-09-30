#ifndef __wcb_gps_h__
#define __wcb_gps_h__

#ifdef __cplusplus
 extern "C" {
#endif

#include "bsp.h"
#include <stdbool.h>

// data parsed directly from UART4 ISR

//extern __IO bool newGpsDataForBle;
//extern char gps_latitude[15], gps_latitude_direction, gps_longitude[16], gps_longitude_direction;
//extern char gps_signal;
//
//// used in server API packet
//extern uint8_t gps_latitude_byte[8];
//extern uint8_t gps_longitude_byte[9];
//
typedef struct{
    float longitude;
    float latitude;
    float hdop;
}gps_data_t;

extern const gps_data_t * const gps_data;

void app_gps_init(void);
void app_gps_process(void);
void handle_gps_data(uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
