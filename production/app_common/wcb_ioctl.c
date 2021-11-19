#include <app_common/wcb_ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include "bsp.h"
#include "WC_Board.h"
#include "app_main/app_info.h"

static uint32_t buzzer_tick=0;
static uint8_t buzzer_state=0;

void ioctl_init(void)
{

}

void ioctl_led(uint8_t led, uint8_t state)
{
	GPIO_TypeDef *GPIOx;
	uint16_t pin;

	switch (led){
		case LED_PERIODIC:
			GPIOx = GPIOA,
			pin = LED_1;
			break;
		case LED_GSM:
			GPIOx = GPIOA,
			pin = LED_2;
			break;
		case LED_GPS:
			GPIOx = GPIOA,
			pin = LED_3;
			break;
		case LED_BLE:
			GPIOx = GPIOA,
			pin = LED_4;
			break;
	}
	switch (state){
		case LED_OFF:
			HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_RESET);
			break;
		case LED_ON:
			HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_SET);
			break;
		case LED_TOGGLE:
			HAL_GPIO_TogglePin(GPIOx, pin);
			break;
	}
}

bool buzzer_check_margin()
{
	if(buzzer_state==0 && millis() - buzzer_tick >1000)
		return true;
	return false;
}

void buzzer_set(uint8_t state)
{
	buzzer_state=state;
	buzzer_tick=millis();
	HAL_GPIO_WritePin(GPIOB, BUZZER, state);
}

void buzzer_set_state(uint8_t state)
{
	if(!user_config->beep_sound)
		return;
	buzzer_set(state);
}

void buzzer_beep(uint32_t ms)
{
	if(!user_config->beep_sound)
		return;
	BUZZER_LOCK();
	buzzer_set(1);
	delay(ms);
	buzzer_set(0);
	BUZZER_UNLOCK();
	buzzer_tick=millis();
}

void buzzer_beepbeep(uint32_t n, uint32_t ms, bool forced)
{
	if(!forced && !user_config->beep_sound)
		return;
	BUZZER_LOCK();
	while (n--)
	{
		buzzer_set(1);
		delay(ms);
		buzzer_set(0);
		delay(ms);
	}
	BUZZER_UNLOCK();
	buzzer_tick=millis();
}

void ioctl_beep_with_all_led(uint32_t timeout)
{
	ioctl_led(LED_PERIODIC, LED_ON);
	ioctl_led(LED_GSM, LED_ON);
	ioctl_led(LED_GPS, LED_ON);
	ioctl_led(LED_BLE, LED_ON);
	ioctl_beep(timeout);
	ioctl_led(LED_PERIODIC, LED_OFF);
	ioctl_led(LED_GSM, LED_OFF);
	ioctl_led(LED_GPS, LED_OFF);
	ioctl_led(LED_BLE, LED_OFF);
}

void ioctl_led_periodic(uint32_t timeout)
{
	static uint32_t tm = 0;
	if (timeout_check(&tm, timeout)){
		timeout_init(&tm);
		ioctl_led(LED_PERIODIC, LED_TOGGLE);
	}
}
