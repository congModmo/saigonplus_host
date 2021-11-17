/*
 * light_control.c
 *
 *  Created on: May 25, 2021
 *      Author: thanhcong
 */
#include "bsp.h"
#include "app_config.h"
#include "light_control.h"
#include "tim.h"
#include "app_main/app_info.h"
#include <string.h>


static struct
{
	bool light_on;
	sidelight_charge_mode_t charge_mode;
	uint32_t charge_tick;
	int charge_value;
	int led_dir;
	bool test;
	uint32_t test_tick;
	uint8_t test_count;
}light_status;

void set_headlight(int value)
{
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, value*255/100);
}

void set_redlight(int value)
{
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, value);
}

void set_greenlight(int value)
{
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, value);
}

void set_bluelight(int value)
{
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, value);
}

static void light_off()
{
	light_status.light_on=false;
	set_headlight(0);
	set_redlight(0);
	set_greenlight(0);
	set_bluelight(0);
}

static void light_on(){
	light_status.light_on=true;
	set_headlight(user_config->head_light);
	set_redlight(user_config->side_light.red);
	set_greenlight(user_config->side_light.green);
	set_bluelight(user_config->side_light.blue);
}

void light_control_init(){
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	light_status.light_on=false;
	light_status.charge_mode=SIDELIGHT_CHARGE_NONE;
}

void light_control(bool on)
{
	if(on && light_status.light_on!=true){
		light_on();
	}
	else if(!on && light_status.light_on ){
		light_off();
	}
}

void light_control_restart()
{
	if(light_status.light_on){
		light_on();
	}
}

void light_control_blink(bool on)
{
	if(on){
		set_headlight(50);
		set_redlight(255);
		set_greenlight(255);
		set_bluelight(255);
	}
	else{
		set_headlight(0);
		set_redlight(0);
		set_greenlight(0);
		set_bluelight(0);
	}
}

void light_control_set_sidelight_charge_mode(sidelight_charge_mode_t mode)
{
	light_status.charge_mode=mode;
	light_status.charge_tick=millis();
	light_status.charge_value=0;
	light_status.led_dir=1;
	if(light_status.light_on)
		return;
	set_greenlight(0);
	set_redlight(0);
	set_bluelight(0);
}

void light_control_test_start()
{
	light_status.test=true;
	light_status.test_count=0;
	light_status.test_tick=millis();
}

void sidelight_charge_process()
{
	if(light_status.charge_mode==SIDELIGHT_CHARGE_NONE)
	{
		return;
	}
	if(millis()-light_status.charge_tick>10)
	{
		light_status.charge_tick=millis();
		if(light_status.charge_mode==SIDELIGHT_CHARGING)
		{
			set_redlight(light_status.charge_value);
		}
		else
		{
			set_greenlight(light_status.charge_value);
		}
		light_status.charge_value+= light_status.led_dir*2;
		if(light_status.charge_value >=255)
		{
			light_status.charge_value=255;
			light_status.led_dir=-1;
		}
		else if(light_status.charge_value<0)
		{
			light_status.charge_value=0;
			light_status.led_dir=1;
		}
	}
}

void light_test_process()
{
	if(!light_status.test)
		return;
	if(millis()-light_status.test_tick <1000)
		return;
	light_status.test_tick=millis();
	switch(light_status.test_count)
	{
	case 0:
		set_bluelight(0);
		set_headlight(50);
		light_status.test_count++;
		break;
	case 1:
		set_headlight(0);
		set_redlight(255);
		light_status.test_count++;
		break;
	case 2:
		set_redlight(0);
		set_greenlight(255);
		light_status.test_count++;
		break;
	case 3:
		set_greenlight(0);
		set_bluelight(255);
		light_status.test_count=0;
		break;
	}
}

void light_control_process()
{
	if(light_status.light_on)
	{
		return;
	}
	sidelight_charge_process();
	light_test_process();
}
