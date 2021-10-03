#include "jigtest.h"
#include "jigtest_esp.h"
#include "jigtest_io.h"
#include "esp_host_comm.h"
#include "app_display/light_control.h"

static struct
{
	int count;
	bool state;
}lockpin;

static struct{
	light_config_t config;
	bool done;
	bool result;
}light_test;

#define ADC_TO_SIDELIGHT(adc) adc
#define ADC_TO_HEADLIGHT(adc) adc

void lockpin_check_polling()
{
	if(lockpin.state!=lock_pin_get_state())
	{
		delay(5);
		if(lockpin.state!=lock_pin_get_state())
		{
			lockpin.count++;
			lockpin.state=lock_pin_get_state();
		}
	}
}

void lockpin_check_init()
{
	lockpin.count=0;
	lockpin.state=lock_pin_get_state();
}

bool tolerance_check(int value, int setpoint, int percent )
{
	if(value*100 > setpoint*(100 +percent))
		return false;
	if(value*100 < setpoint*(100-percent))
		return false;
	return true;
}

void io_test_cb(uint8_t *data, uint16_t len)
{
	host_adc_value_t *_adc=(host_adc_value_t *) data;
	light_test.done=true;
	light_test.result=false;
	if(!tolerance_check(ADC_TO_SIDELIGHT(_adc->led_blue), light_test.config.blue, 5))
		return;
	if(!tolerance_check(ADC_TO_SIDELIGHT(_adc->led_red), light_test.config.red, 5))
		return;
	if(!tolerance_check(ADC_TO_SIDELIGHT(_adc->led_green), light_test.config.green, 5))
		return;
	if(!tolerance_check(ADC_TO_HEADLIGHT(_adc->front_light), light_test.config.head, 5))
		return;
	light_test.result=true;
	return;
}

bool check_io_match()
{
	uint32_t tick=millis();
	light_test.done=false;
	uart_esp_send_cmd(ESP_BLE_HOST_ADC_VALUE);
	while(millis()-tick<100 && !light_test.done){
		esp_uart_polling(io_test_cb);
		delay(5);
	}
	if(!light_test.done)
		return false;
	return light_test.result;
}

static void light_testing()
{
	light_control_init();
	light_control(true);
	int test_count=4;
	light_test.config.blue=light_test.config.red=light_test.config.green=0;
	light_test.config.head=0;
	bool result=true;
	while(test_count--){
		light_control_set_config(&light_test.config);
		delay(100);
		if(!check_io_match()){
			result=false;
			break;
		}
		light_test.config.blue+=50;
		light_test.config.red+=50;
		light_test.config.green+=50;
		light_test.config.head+=25;
	}
	if(result){
		jigtest_direct_report(UART_UI_RES_FRONT_LIGHT, 1);
		jigtest_direct_report(UART_UI_RES_LED_BLUE, 1);
		jigtest_direct_report(UART_UI_RES_LED_GREEN, 1);
		jigtest_direct_report(UART_UI_RES_LED_RED, 1);
	}
}

void lockpin_testing()
{
	uart_esp_send_cmd(ESP_BLE_HOST_BLINK);
	lockpin_check_init();
	uint32_t tick=millis();
	while(millis()-tick<1000)
	{
		lockpin_check_polling();
		delay(5);
	}
	if(lockpin.count>5){
		jigtest_direct_report(UART_UI_RES_LOCKPIN, 1);
	}
	else{
		jigtest_direct_report(UART_UI_RES_LOCKPIN, 0);
	}
}

void jigtest_test_io()
{
	light_testing();
	lockpin_testing();
}
