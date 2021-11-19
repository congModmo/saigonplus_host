/*
 * alarm.c
 *
 *  Created on: Oct 25, 2021
 *      Author: thanhcong
 */
#include <stdbool.h>
#include "alarm.h"
#include "bsp.h"
#include "app_common/wcb_ioctl.h"
#include "app_display/light_control.h"
#include "app_main/app_info.h"
#include "app_display/app_display.h"
#include "app_imu/app_imu.h"

static struct{
	bool enabled;
	bool active;
	uint32_t tick;
	bool motion_detected;
	uint32_t buzz_tick;
	uint8_t buzz_state;
	uint32_t blink_tick;
	uint8_t blink_state;
}alarm_state;

static void motion_detect_handler()
{
	if(alarm_state.enabled)
		alarm_state.motion_detected=true;
}

static void alarm_off()
{
	alarm_state.active=false;
	buzzer_set(0);
	light_control_blink(0);
}

static void alarm_on()
{
	alarm_state.active=true;
	alarm_state.tick=millis();
	alarm_state.buzz_tick=millis();
	alarm_state.buzz_state=0;
	alarm_state.blink_tick=millis();
	alarm_state.blink_state=0;
}

void alarm_process_start()
{
	alarm_state.enabled=true;
	alarm_state.motion_detected=false;
}

void alarm_process_end()
{
	alarm_state.enabled=false;
	alarm_state.active=false;
	alarm_off();
}

void alarm_init()
{
	alarm_process_end();
	app_imu_register_callback(motion_detect_handler);
}

void alarm_process()
{
	if(alarm_state.active)
	{
		if(millis()-alarm_state.buzz_tick>100)
		{
			alarm_state.buzz_tick=millis();
			alarm_state.buzz_state = !alarm_state.buzz_state;
			buzzer_set(alarm_state.buzz_state);
		}
		if(millis()-alarm_state.blink_tick>1000)
		{
			alarm_state.blink_tick=millis();
			alarm_state.blink_state = !alarm_state.blink_state;
			light_control_blink(alarm_state.blink_state);
		}
		if(alarm_state.motion_detected){
			alarm_state.tick=millis();
			alarm_state.motion_detected=false;
		}
		if(millis()- alarm_state.tick>30000)
		{
			alarm_off();
		}
	}
	else
	{
		if(alarm_state.motion_detected)
		{
			alarm_state.motion_detected=false;
			alarm_on();
		}
	}
}
