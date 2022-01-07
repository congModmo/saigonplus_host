
#define __DEBUG__ 4
#include "jigtest.h"
#include "jigtest_esp.h"
#include "jigtest_io.h"
#include "esp_host_comm.h"
#include "app_display/light_control.h"
#include "app_display/app_display.h"
#include "tim.h"
#include "protothread/pt.h"
#include "app_imu/app_imu.h"
#include "app_common/wcb_ioctl.h"
#include "app_main/app_info.h"

static task_complete_cb_t callback;

/******************************************************************
 * TEST LED
 *****************************************************************/
static struct{
	bool on;
	int red;
	int green;
	int blue;
	int head;
	bool done;
}light_test;

static jigtest_io_result_t test_io_result;
static struct pt io_pt, led_pt, blink_pt, check_io_pt, lockpin_pt;

#define ADC_TO_SIDELIGHT(adc) adc
#define ADC_TO_HEADLIGHT(adc) adc

void io_test_cb(uint8_t *data, size_t len)
{
	if(data[0]==ESP_BLE_HOST_ADC_VALUE)
	{
		host_adc_value_t *_adc=(host_adc_value_t *) (data +1);
		light_test.done=true;
		if(light_test.on)
		{
			if(_adc->front_light <2500 && _adc->front_light >2300)
			{
				light_test.head++;
			}
			if(_adc->led_red <2500 && _adc->led_red >2300)
			{
				light_test.red++;
			}
			if(_adc->led_green <2500 && _adc->led_green >2300)
			{
				light_test.green++;
			}
			if(_adc->led_blue <2500 && _adc->led_blue >2300)
			{
				light_test.blue++;
			}
		}
		else
		{
			if(_adc->front_light <100)
			{
				light_test.head++;
			}
			if(_adc->led_red <100)
			{
				light_test.red++;
			}
			if(_adc->led_green <100)
			{
				light_test.green++;
			}
			if(_adc->led_blue <100)
			{
				light_test.blue++;
			}
		}
	}
	return;
}

static int check_io_match(struct pt *pt)
{
	PT_BEGIN(pt);
	uint32_t tick=millis();
	light_test.done=false;
	uart_esp_send_cmd(ESP_BLE_HOST_ADC_VALUE);
	while(millis()-tick<100 && !light_test.done)
	{
		jigtest_esp_uart_polling(io_test_cb);
		PT_YIELD(pt);
	}
	PT_END(pt);
}

static int light_blink_test(struct pt *pt, bool *test_result)
{
	PT_BEGIN(pt);
	static int test_num;
	static uint32_t tick;
	static int test_count;
	test_count=0;
	test_num=10;
	*test_result=false;
	light_test.red=light_test.green=light_test.blue=light_test.head=0;
	while(test_count <test_num)
	{
		light_test.on=true;
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 255);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 255);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 255);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 255);
		tick=millis();
		PT_WAIT_UNTIL(pt, millis()-tick>100);
		PT_SPAWN(pt, &check_io_pt, check_io_match(&check_io_pt));
		light_test.on=false;
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 0);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);
		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);
		tick=millis();
		PT_WAIT_UNTIL(pt, millis()-tick>100);
		PT_SPAWN(pt, &check_io_pt, check_io_match(&check_io_pt));
		test_count++;
	}
	if(light_test.red>=test_num)
	{
		test_io_result.red=true;
	}
	if(light_test.green>=test_num){
		test_io_result.green=true;
	}
	if(light_test.blue>=test_num){
		test_io_result.blue=true;
	}
	if(light_test.head>=test_num){
		test_io_result.head=true;
	}
	if(test_io_result.red && test_io_result.green && test_io_result.blue && test_io_result.head)
	{
		*test_result=true;
	}
	PT_END(pt);
}

static int led_testing(struct pt *pt)
{
	PT_BEGIN(pt);
	light_control_init();
	light_control(true);
	static bool result;
	for(uint8_t i=0; i<3; i++)
	{
		PT_SPAWN(pt, &blink_pt, light_blink_test(&blink_pt, &result));
		if(result)
		{
			break;
		}
	}
	jigtest_direct_report(UART_UI_RES_LED_RED, test_io_result.red);
	jigtest_direct_report(UART_UI_RES_LED_GREEN, test_io_result.green);
	jigtest_direct_report(UART_UI_RES_LED_BLUE, test_io_result.blue);
	jigtest_direct_report(UART_UI_RES_LED_HEAD, test_io_result.head);
	PT_END(pt);
}

/*****************************************************************************
 * TEST LOCKPIN
 ****************************************************************************/

static struct
{
	int count;
	bool state;
}lockpin;

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

static int lockpin_testing(struct pt *pt)
{
	PT_BEGIN(pt);
	uart_esp_send_cmd(ESP_BLE_HOST_BLINK);
	lockpin_check_init();
	static uint32_t tick;
	tick=millis();
	while(millis()-tick<1000)
	{
		lockpin_check_polling();
		PT_YIELD(pt);
	}
	if(lockpin.count>5){
		test_io_result.lock=true;
		jigtest_direct_report(UART_UI_RES_LOCKPIN, test_io_result.lock);
	}
	PT_END(pt);
}

/*****************************************************************************
 * MISC TEST
 ****************************************************************************/

static bool motion_detected=false;
void imu_motion_detected(void)
{
	motion_detected=true;
	jigtest_direct_report(UART_UI_RES_IMU_TRIGGER, 1);
}

static struct pt misc_pt;

static int misc_io_test(struct pt *pt)
{
	PT_BEGIN(pt);
	static uint32_t tick;
	motion_detected=false;
	if (jigtest_app_imu_init(5))
	{
		jigtest_direct_report(UART_UI_RES_IMU_TEST, 1);
		app_imu_register_callback(imu_motion_detected);
	}
	buzzer_beepbeep(3, 100, true);
	tick=millis();
	while(millis() - tick < 1000 && !motion_detected )
	{
		app_imu_process();
		PT_YIELD(pt);
	}
	bool flash_test=GD25Q16_test(fotaCoreBuff, 4096);
	if(flash_test)
	{
		jigtest_direct_report(UART_UI_RES_EXTERNAL_FLASH, 1);
		app_config_factory_reset();
	}
	PT_END(pt);
}
/*****************************************************************************
 * MAIN TEST
 ****************************************************************************/
static int io_testing_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	PT_SPAWN(pt, &led_pt, led_testing(&led_pt));
	PT_SPAWN(pt, &lockpin_pt, lockpin_testing(&lockpin_pt));
	PT_SPAWN(pt, &misc_pt, misc_io_test(&misc_pt));
	callback();
	PT_END(pt);
}

void jigtest_io_init(task_complete_cb_t cb, void *params)
{
	memset(&test_io_result, 0, sizeof(jigtest_io_result_t));
	PT_INIT(&io_pt);
	callback=cb;
}

void jigtest_io_process()
{
	io_testing_thread(&io_pt);
}

void jigtest_io_console_handle(char *result)
{
//	//red green blue head
//	if(__check_cmd("set pwm ")){
//		int red, green, blue, head;
//		if(sscanf(__param_pos("set pwm "), "%d %d %d %d", &red, &green, &blue, &head)!=4){
//			error("pwm params\n");
//			return;
//		}
//		debug("Set pwm to red: %d, green: %d, blue: %d, head: %d\n", red, green, blue, head);
//		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, head*255/100);
//		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, red);
//		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, green);
//		__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, blue);
//	}
//	else if(__check_cmd("test led"))
//	{
//		light_testing();
//		debug("test led result red green blue head %d %d %d %d\n", test_io_result.red, test_io_result.green, test_io_result.blue, test_io_result.head);
//	}
//	else if(__check_cmd("test lock"))
//	{
//		lockpin_testing();
//		debug("Test lock %s\n", test_io_result.lock?"ok":"error");
//	}
//	else debug("Unknown command\n");
}
