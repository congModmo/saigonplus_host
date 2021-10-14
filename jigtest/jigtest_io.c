
#define __DEBUG__ 4
#include "jigtest.h"
#include "jigtest_esp.h"
#include "jigtest_io.h"
#include "esp_host_comm.h"
#include "app_display/light_control.h"
#include "tim.h"

static struct
{
	int count;
	bool state;
}lockpin;

static struct{
	bool on;
	int red;
	int green;
	int blue;
	int head;
	bool done;
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

void io_test_cb(uint8_t *data, uint16_t len)
{
	if(data[0]==ESP_BLE_HOST_ADC_VALUE)
	{
		host_adc_value_t *_adc=(host_adc_value_t *) (data +1);
		light_test.done=true;
		if(light_test.on)
		{
			if(_adc->front_light <2500 && _adc->front_light >2400){
				light_test.head++;
			}
			if(_adc->led_red <2500 && _adc->led_red >2400){
				light_test.red++;
			}
			if(_adc->led_green <2500 && _adc->led_green >2400){
				light_test.green++;
			}
			if(_adc->led_blue <2500 && _adc->led_blue >2400){
				light_test.blue++;
			}
		}
		else{
			if(_adc->front_light <100){
				light_test.head++;
			}
			if(_adc->led_red <100){
				light_test.red++;
			}
			if(_adc->led_green <100){
				light_test.green++;
			}
			if(_adc->led_blue <100){
				light_test.blue++;
			}
		}
	}

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
}

static void light_testing()
{
	light_control_init();
	light_control(true);
	int test_num=10;
	int test_count=0;
	light_test.red=light_test.green=light_test.blue=light_test.head=0;
	while(test_count <test_num){
		light_test.on=true;
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 255);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 255);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 255);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 255);
		delay(10);
		check_io_match();
		light_test.on=false;
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 0);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);
		delay(10);
		check_io_match();
		test_count++;
	}
	if(light_test.red==test_num*2){
		jigtest_direct_report(UART_UI_RES_LED_RED, 1);
	}
	else{
		jigtest_direct_report(UART_UI_RES_LED_RED, 0);
	}
	if(light_test.green==test_num*2){
		jigtest_direct_report(UART_UI_RES_LED_GREEN, 1);
	}
	else{
		jigtest_direct_report(UART_UI_RES_LED_GREEN, 0);
	}
	if(light_test.blue==test_num*2){
		jigtest_direct_report(UART_UI_RES_LED_BLUE, 1);
	}
	else{
		jigtest_direct_report(UART_UI_RES_LED_BLUE, 0);
	}
	if(light_test.head==test_num*2){
		jigtest_direct_report(UART_UI_RES_FRONT_LIGHT, 1);
	}
	else{
		jigtest_direct_report(UART_UI_RES_FRONT_LIGHT, 0);
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

void jigtest_io_console_handle(char *result)
{
	//red green blue head
	if(__check_cmd("set pwm ")){
		int red, green, blue, head;
		if(sscanf(__param_pos("set pwm "), "%d %d %d %d", &red, &green, &blue, &head)!=4){
			error("pwm params\n");
			return;
		}
		debug("Set pwm to red: %d, green: %d, blue: %d, head: %d\n", red, green, blue, head);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, head*255/100);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, red);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, green);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, blue);
	}
	else if(__check_cmd("test led"))
	{
		light_testing();
	}
	else if(__check_cmd("test lock"))
	{
		lockpin_testing();
	}
	else debug("Unknown command\n");
}
