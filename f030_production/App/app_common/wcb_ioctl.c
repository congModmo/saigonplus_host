#include <app_common/wcb_ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include "bsp.h"
#include "WC_Board.h"
#include "Define.h"
#include "app_main/app_info.h"

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

void ioctl_buzz(uint8_t state)
{
	HAL_GPIO_WritePin(GPIOB, BUZZER, state);
}

void ioctl_beep(uint32_t ms)
{
	BUZZER_LOCK();
	ioctl_buzz(ON);
	delay(ms);
	ioctl_buzz(OFF);
	BUZZER_UNLOCK();
}

void ioctl_beepbeep(uint32_t n, uint32_t ms)
{
	if(!user_config->beep_sound)
		return;
	BUZZER_LOCK();
	while (n--)
	{
		ioctl_buzz(ON);
		delay(ms);
		ioctl_buzz(OFF);
		delay(ms);
	}
	BUZZER_UNLOCK();
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
