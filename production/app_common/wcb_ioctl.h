#ifndef __ss_ioctl_h__
#define __ss_ioctl_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

enum {
    LED_PERIODIC = 0,
    LED_GSM,
    LED_GPS,
    LED_BLE
};

enum {
    LED_OFF = 0,
    LED_ON,
    LED_TOGGLE
};

void ioctl_init(void);
void ioctl_led(uint8_t led, uint8_t state);
void ioctl_buzz(uint8_t state);
void ioctl_beep(uint32_t ms);
void ioctl_beepbeep(uint32_t n, uint32_t ms);
void ioctl_beep_with_all_led(uint32_t timeout);
void ioctl_led_periodic(uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
