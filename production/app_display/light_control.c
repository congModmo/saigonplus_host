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
static __IO bool light_state=false;
static light_config_t light_config={0};

static void light_off(){
	light_state=false;
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);
}

static void light_on(){
	light_state=true;
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, (uint32_t)(light_config.red));
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, (uint32_t)(light_config.head));
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, (uint32_t)(light_config.blue));
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, (uint32_t)(light_config.green));
}

void light_control_init(){
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

void light_control(bool on)
{
	if(on && light_state!=true){
		light_state=true;
		light_on();
	}
	else if(!on && light_state ){
		light_state=false;
		light_off();
	}
}

static void light_control_restart()
{
	if(light_state){
		light_on();
	}
}

void light_control_set_config(light_config_t *config)
{
	memcpy(&light_config, config, sizeof(light_config_t));
	light_control_restart();
}
