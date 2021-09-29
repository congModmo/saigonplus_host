/*
 * app_imu.c
 *
 *  Created on: Apr 26, 2021
 *      Author: thanhcong
 */
#define __DEBUG__ 4
#include "app_imu.h"
#include "kxtj3/kxtj3.h"
#include "app_display/app_display.h"
#include "bsp.h"

typedef struct
{
	bool detected;
	bool motion_detected;
	uint8_t state;
	uint32_t publish_tick;
	uint32_t imu_tick;
	uint8_t int_rel;
}imu_t;

static imu_t imu={0};

static void imu_int_update()
{
	if(HAL_GPIO_ReadPin(IMU_INT_GPIO_Port, IMU_INT_Pin)!=imu.state)
	{
		imu.state=HAL_GPIO_ReadPin(IMU_INT_GPIO_Port, IMU_INT_Pin);
		if(imu.state==1)
		{
			debug("Motion detected\n");
			kxtj3_readRegister(&imu.int_rel, KXTJ3_INT_REL);
		}
	}
}

bool app_imu_init()
{
	if(kxtj3_begin(50, 2 )!=IMU_SUCCESS)
	{
		error("Imu init failed\n");
		return false;
	}
	debug("Imu ready\n");
	kxtj3_intConf(32, 5, 10, HIGH);
	imu.detected=true;
	imu.motion_detected=false;
	imu.state=0;
	return true;
}

void app_imu_process()
{
	if(!imu.detected)
		return;
	imu_int_update();
}

void app_imu_console_handle(char *result)
{
	if(__check_cmd("self test "))
	{
		result=__param_pos("self test ");
		if(__check_cmd("start"))
		{
			debug("Start imu self test\n");
			kxtj3_selfTest(true);
		}
		else
		{
			debug("Stop imu self test\n");
			kxtj3_selfTest(false);
		}
	}
	else if(__check_cmd("read"))
	{
		debug("Raw x: %f\n", kxtj3_axisAccel(KXTJ3_X));
		debug("Raw y: %f\n", kxtj3_axisAccel(KXTJ3_Y));
		debug("Raw z: %f\n\n", kxtj3_axisAccel(KXTJ3_Z));
	}
	else if(__check_cmd("threshold "))
	{
		uint threshold;
		if(sscanf(__param_pos("threshold "), "%u", &threshold)==1)
		{
			debug("Config imu threshold: %u\n", threshold);
			kxtj3_configThreshold(threshold);
		}
	}
	else debug("Unknown command\n");
}
