/*
    Console task

    04/12/2020
    HieuNT
*/

#define __DEBUG__ 4
#include "bsp.h"
#include "target.h"
#include "log_sys.h"
#include <gps/eva_m8.h>
#include "app_main/app_info.h"
#include <app_common/wcb_common.h>
#include <app_common/wcb_ioctl.h>
#include "cmsis_os.h"
#include "app_config.h"
#include "tim.h"
#include "app_fota/fota_core.h"
#include "app_main/app_main.h"
#include "factory_config.h"
#include "Task.h"

// Utils
static RINGBUF debugRingbuf;

extern osThreadId_t mainTaskHandle;
extern osThreadId_t lteTaskHandle;
extern osThreadId_t mqttTaskHandle;

static uint32_t wmMain=1024, wmLte=1024, wmMqtt=1024;

int console_read(char **result)
{
    static char consoleBuffer[128];
    static int i = 0;

    while (RINGBUF_Available(&debugRingbuf))
    {
        uint8_t inChar;
        uint32_t len = 1;
        RINGBUF_Get(&debugRingbuf, &inChar);
        if (inChar == '\r' || inChar == '\n') // we play with both CR and LF!
        {
            consoleBuffer[i] = '\0';
            len = i;
            i = 0;
            *result = consoleBuffer;
            // console_flush();
            return len;
        }
        else
        {
            consoleBuffer[i] = inChar;
            i++;
            if (i == (sizeof(consoleBuffer) - 1))
            {
                consoleBuffer[i] = '\0';
                len = i;
                i = 0;
                *result = consoleBuffer;
                // console_flush();
                return len;
            }
        }
    }
    return 0;
}

void console_init(UART_HandleTypeDef *huart)
{
	bsp_uart_console_init(&debugRingbuf);
}

void console_message_handle(char *result)
{
	if(__check_cmd("hello"))
	{
		debug("Hi there\n");
	}
	else if(__check_cmd("water mark")){
		debug("Water mark main: %u, lte: %u, mqtt: %u words\n", wmMain, wmLte, wmMqtt);
	}
	else if(__check_cmd("display "))
	{
		app_display_console_handle(__param_pos("display "));
	}
	else if(__check_cmd("fota "))
	{
		fota_console_handle(__param_pos("fota "));
	}
	else if(__check_cmd("info "))
	{
		app_info_console_handle(__param_pos("info "));
	}
	else if(__check_cmd("mqtt "))
	{
		app_mqtt_console_handle(__param_pos("mqtt "));
	}
	else if(__check_cmd("imu "))
	{
		app_imu_console_handle(__param_pos("imu "));
	}
	else if(__check_cmd("set rtc "))
	{
		set_rtc_from_at_string(__param_pos("set rtc "));
	}
	else if(__check_cmd("get time"))
	{
		rtc_print_date_time();
	}
	else if(__check_cmd("test double"))
	{
		gps_test_double();
	}
	else if(__check_cmd("test fota parse info"))
	{
		TEST_parse_json_info();
	}
	else if(__check_cmd("test protocol v3 "))
	{
		TEST_protocol_v3();
	}
	else if(__check_cmd("test len to block"))
	{
		TEST_len_to_block_len();
	}
	else if(__check_cmd("ble "))
	{
		app_ble_console_handle(__param_pos("ble "));
	}
	else if(__check_cmd("lte "))
	{
		app_lte_console_handle(__param_pos("lte "));
	}
	else if(__check_cmd("mqtt "))
	{
		app_mqtt_console_handle(__param_pos("mqtt "));
	}
}

void console_task(void)
{
    char *result;
    static uint32_t tick=0;
    if(millis()-tick>1000){
    	tick=millis();
    	if(wmMain > uxTaskGetStackHighWaterMark(mainTaskHandle)){
    		wmMain=uxTaskGetStackHighWaterMark(mainTaskHandle);
    	}
    	if(wmLte > uxTaskGetStackHighWaterMark(lteTaskHandle)){
    		wmLte = uxTaskGetStackHighWaterMark(lteTaskHandle);
    	}
    	if(wmMqtt > uxTaskGetStackHighWaterMark(mqttTaskHandle)){
    		wmMqtt = uxTaskGetStackHighWaterMark(mqttTaskHandle);
    	}
    }
    if (console_read(&result) > 0)
    {
    	console_message_handle(result);
    }
}
