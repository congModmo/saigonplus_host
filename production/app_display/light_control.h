/*
 * light_control.h
 *
 *  Created on: May 25, 2021
 *      Author: thanhcong
 */

#ifndef LIGHT_CONTROL_H_
#define LIGHT_CONTROL_H_

#include <stdint.h>

// light set config 0->255;
typedef struct{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t head;
}light_config_t;

typedef enum{
	SIDELIGHT_CHARGE_NONE,
	SIDELIGHT_CHARGING,
	SIDELIGHT_FULL_CHARGED,
}sidelight_charge_mode_t;

void light_control_init();
void light_control(bool on);
void light_control_set_config(light_config_t *config);
void light_control_blink(bool on);
void light_control_set_sidelight_charge_mode(sidelight_charge_mode_t mode);
#endif /* APP_MAIN_LIGHT_CONTROL_H_ */
