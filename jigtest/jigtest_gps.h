#ifndef __JIGTEST_GPS_H
#define __JIGTEST_GPS_H

#include "jigtest.h"
#include "protothread/pt.h"

void jigtest_gps_hardware_init(task_complete_cb_t cb, void *params);
void jigtest_gps_hardware_process(void);

void jigtest_gps_function_init(task_complete_cb_t cb, void *params);
void jigtest_gps_function_process(void);
#endif
