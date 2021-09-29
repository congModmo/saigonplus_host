#include "testkit.h"
#include "testkit_esp.h"
#include "testkit_io.h"
static io_test_t io_tester={0};

typedef struct
{
	int count;
	bool state;
}lockpin_check_t;

static lockpin_check_t lockpin;

void lockpin_check_polling()
{
	if(lockpin.state!=lock_pin_get_state()){
		delay(5);
		if(lockpin.state!=lock_pin_get_state()){
			lockpin.count++;
			lockpin.state=lock_pin_get_state();
		}
	}
}

void lockpin_check_init(){
	lockpin.count=0;
	lockpin.state=lock_pin_get_state();
}

void io_test_cb(uint8_t *data, uint16_t len){
	if(data[0]==ESP_BLE_HOST_IO_VALUE){
		host_io_value_t *value =(host_io_value_t *)(data+1);
		if(io_tester.light_on){
			if(value->led_blue==1) io_tester.led_blue_count++;
			if(value->led_red==1) io_tester.led_red_count++;
			if(value->led_green==1) io_tester.led_green_count++;
			if(value->front_light==1) io_tester.led_front_count++;
		}
		else{
			if(value->led_blue==0) io_tester.led_blue_count++;
			if(value->led_red==0) io_tester.led_red_count++;
			if(value->led_green==0) io_tester.led_green_count++;
			if(value->front_light==0) io_tester.led_front_count++;
		}
	}
	io_tester.completed=true;
}

void check_io_match(){
	uint32_t tick=millis();
	io_tester.completed=false;
	uart_esp_send_cmd(ESP_BLE_HOST_IO_VALUE);
	while(millis()-tick<100 && !io_tester.completed){
		esp_uart_polling(io_test_cb);
		delay(5);
	}
}

void testkit_test_io()
{
	#define TEST_NUM 5
	memset(&io_tester, 0, sizeof(io_test_t));
	uint8_t cmd=ESP_BLE_HOST_IO_VALUE;
	for(uint8_t i=0; i<TEST_NUM; i++){
		light_on();
		io_tester.light_on=true;
		check_io_match();
		light_off();
		io_tester.light_on=false;
		check_io_match();
	}
	if(io_tester.led_blue_count==TEST_NUM*2)
		testkit_direct_report(UART_UI_RES_LED_BLUE, 1);
	else 
		testkit_direct_report(UART_UI_RES_LED_BLUE, 0);
	if(io_tester.led_red_count==TEST_NUM*2)
		testkit_direct_report(UART_UI_RES_LED_RED, 1);
	else 
		testkit_direct_report(UART_UI_RES_LED_RED, 0);
	if(io_tester.led_green_count==TEST_NUM*2)
		testkit_direct_report(UART_UI_RES_LED_GREEN, 1);
	else 
		testkit_direct_report(UART_UI_RES_LED_GREEN, 0);
	if(io_tester.led_front_count==TEST_NUM*2)
		testkit_direct_report(UART_UI_RES_FRONT_LIGHT, 1);
	else 
		testkit_direct_report(UART_UI_RES_FRONT_LIGHT, 0);
	uint32_t tick=millis();
	uart_esp_send_cmd(ESP_BLE_HOST_BLINK);
	lockpin_check_init();
	while(millis()-tick<1000){
		lockpin_check_polling();
		delay(5);
	}
	if(lockpin.count>5){
		testkit_direct_report(UART_UI_RES_LOCKPIN, 1);
	}
	else{
		testkit_direct_report(UART_UI_RES_LOCKPIN, 0);
	}
}
