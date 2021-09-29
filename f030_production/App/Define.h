#ifndef __Define_h__
#define __Define_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx.h"
	

#define ON                            1
#define OFF                           0

#define HW_VERSION                    "1.0.0"

#define POW_2_32                      4294967296
#define POW_2_24                      16777216
#define POW_2_16                      65536
#define POW_2_8                       256

// Define threshold time execute function
#define WCB_TIME_SYNC_GPS_DATA        (2     * 1000)    // 1 [s]
#define WCB_SEND_PERIODIC_LOOPTIME    (60)    // 60 [s]
#define WCB_RESET_GPS_TIME            (900   * 1000)    // 900 [s]

///////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif
