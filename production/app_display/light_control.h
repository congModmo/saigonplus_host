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

void light_control_init();
void light_control(bool on);
void light_control_set_config(light_config_t *config);
void light_control_blink(bool on);
#endif /* APP_MAIN_LIGHT_CONTROL_H_ */
