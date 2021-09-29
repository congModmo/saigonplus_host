/*
 * app_imu.h
 *
 *  Created on: Apr 26, 2021
 *      Author: thanhcong
 */

#ifndef _APP_IMU_H_
#define _APP_IMU_H_
#include <stdbool.h>
/*
 * Theft detect only active when display off
 * Publish interval is not less than 5 second
 */
bool app_imu_init();
void app_imu_process();
void app_imu_set_active(bool active);

#endif /* APP_APP_IMU_APP_IMU_H_ */
