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

static void light_off(){
	light_state=false;
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);
}

static void light_on(){
	light_state=true;
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, user_config->head_light*255/100);
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, user_config->side_light.red);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, user_config->side_light.green);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, user_config->side_light.blue);
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
		light_on();
	}
	else if(!on && light_state ){
		light_off();
	}
}

void light_control_restart()
{
	if(light_state){
		light_on();
	}
}
