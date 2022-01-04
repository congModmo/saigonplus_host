/*
 * app_display.h
 *
 *  Created on: Apr 21, 2021
 *      Author: thanhcong
 */

#ifndef _APP_DISPLAY_H_
#define _APP_DISPLAY_H_
#include "ringbuf/ringbuf.h"
#include <stdint.h>
#include <stdbool.h>

//App display functions:
//- control head light: turn off head light if display off
//- read display params
//- detect display on/off

#define LOCK_CHANGE_CB_MAX_NUM 5
typedef struct{
    uint32_t odo;
    uint16_t trip;
    uint16_t speed;
    uint32_t battery_voltage;
    uint8_t battery_remain;
    uint8_t battery_state;
    uint16_t battery_temperature;
    uint16_t battery_charge_cycle;
    uint16_t battery_uncharge_time;
    uint32_t battery_capacity;
    uint8_t battery_event;
    bool display_on;
    bool light_on;
    uint8_t gear;
    uint8_t charging;
    uint8_t full_charged;
    int16_t bms_current;
    uint32_t off_tick;
}display_data_t;

typedef enum{
    DISPLAY_NORMAL_MODE,
    DISPLAY_ANTI_THEFT_MODE
}display_mode_t;

extern const display_data_t * const display_state;

typedef void (*lock_change_cb_t)(bool state);

void app_display_init();
void app_display_process();
bool app_display_add_lock_change_cb(lock_change_cb_t cb);
void app_display_set_mode(display_mode_t mode);
void app_display_reset_data();
bool lock_pin_get_state(void);

#endif /* APP_APP_DISPLAY_APP_DISPLAY_H_ */
