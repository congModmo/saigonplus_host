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
#include "app_main/factory_config.h"
// Utils
static RINGBUF debugRingbuf;

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

static GPIO_TypeDef* blink_port;
static uint16_t blink_pin;
static bool blink_run=false;
static uint32_t blink_tick;

void blink_start(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin){
	blink_port=GPIOx;
	blink_pin=GPIO_Pin;
	blink_tick=millis();
	blink_run=true;
}

void blink_stop(){
	blink_run=false;
}

void blink_process(){
	if(blink_run){
		if(millis()-blink_tick>200){
			blink_tick=millis();
			HAL_GPIO_TogglePin(blink_port, blink_pin);
		}
	}
}

extern osThreadId_t mqttTaskHandle;
extern osThreadId_t mainTaskHandle;
extern osThreadId_t lteTaskHandle;
extern osMessageQueueId_t mainMailHandle;

void console_init()
{
	bsp_uart_console_init(&debugRingbuf);
}

void console_task(void)
{
    char *result;
    uint32_t u;
    blink_process();
    static uint32_t tick=0;
    static uint32_t wmMain=1024, wmLte=1024, wmMqtt=1024;

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
    if (console_read(&result) > 0){
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
    	else if(__check_cmd("test flash"))
    	{
    		if(GD25Q16_test(fotaCoreBuff, 4096))
    		{
    			debug("flash ok\n");
    		}
    		else
    		{
    			error("flash err\n");
    		}
    	}
    	else if(__check_cmd("test len to block"))
    	{
    		TEST_len_to_block_len();
    	}
    	else if(__check_cmd("ble "))
    	{
    		app_ble_console_handle(__param_pos("ble "));
    	}
#ifdef LTE_ENABLE
//    	else if(__check_cmd("AT")){
//
//    		static char cmd[100];
//    		sprintf(cmd, "%s\r\n", result);
//    		debug("Send at: %s\n", cmd);
//    		gsm_send_string(cmd);
//    	}
//    	else if(__check_cmd("fota ")){
//    		uint32_t len, crc;
//    		static char link[128];
//    		if(sscanf(__param_pos("fota "), "%u %u %s", &len, &crc, link)==3){
//    			debug("Start fota device info len: %u, crc: %u, link: %s\n", len, crc, link);
//
//    			uint8_t *frame;
//    			uint8_t data_len= sizeof(fota_cmd_t)+ strlen(link);
//    			data_len=create_mqtt_frame0(MQTT_CMD_FOTA, data_len, &frame);
//    			if(data_len==0){
//    				error("create frame");
//    				return;
//    			}
//    			fota_cmd_t *cmd=frame+sizeof(mqtt_cmd_header_t);
//    			cmd->info_len=len;
//    			cmd->info_crc=crc;
//    			strcpy(&cmd->link, link);
//    			send_mqtt_frame(frame, data_len, mainMailHandle);
//    		}
//    	}
#endif
    	else if(__check_cmd("led ")){
    		int state;
    		result=__param_pos("led ");
    		if(__check_cmd("red ")){
    			if(sscanf(__param_pos("red "), "%d", &state)==1){
    				debug("Set led red to state: %d\n", state);
    				__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, state);
    			}
    		}
    		else if(__check_cmd("green ")){
				if(sscanf(__param_pos("green "), "%d", &state)==1){
					debug("Set led green to state: %d\n", state);
					__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, state);
				}
			}
    		else if(__check_cmd("blue ")){
				if(sscanf(__param_pos("blue "), "%d", &state)==1){
					debug("Set led blue to state: %d\n", state);
					__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, state);
				}
			}
    		else if(__check_cmd("front ")){
				if(sscanf(__param_pos("front "), "%d", &state)==1){
					debug("Set led front to state: %d\n", state);
					__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, state);
				}
			}
    		else if(__check_cmd("light on")){
    			light_control(true);
			}
    		else if(__check_cmd("light off")){
				light_control(false);
			}
    	}
        else if (__check_cmd("get sn")){
//            memset(p->serialNumber, 0, sizeof(p->serialNumber));
//            flash_ext_read_serialid((uint8_t *)p->serialNumber, sizeof(p->serialNumber));
//            info("Flash read: serialNumber: %s\n", p->serialNumber);
        }
        else if (__check_cmd("set sn ")){
//            result += strlen("set sn ");
//            strcpy(p->serialNumber, result);
//            flash_ext_write_serialid((uint8_t *)p->serialNumber);
//            delay(100);
//            flash_ext_read_serialid((uint8_t *)p->serialNumber, sizeof(p->serialNumber));
//            info("Flash written: serialNumber: %s\n", p->serialNumber);
        }
        else if (__check_cmd("get imei")){
//            info("Read IMEI: %s\n", p->imei);
        }
        else if (__check_cmd("get mac")){
//            info("Read BLEMAC: %s\n", p->mac);
        }
        else if (__check_cmd("get ccid")){
//            info("Read CCID: %s\n", p->ccid);
        }
        else if (__check_cmd("set broker ")){
            broker_t broker;
            if(sscanf(__param_pos("set broker "), "%s %d %d", broker.endpoint, &broker.port, &broker.secure)==3){
                debug("set new broker to: %s, port: %d, secure: %d\n", broker.endpoint, broker.port, broker.secure);
//                GD25Q16_WriteAndVerifySector(FLASH_EXT_ADDR_BROKER, &broker, sizeof(broker_t));
            }
        }
        else if (__check_cmd("network init")){
        	debug("init network\n");
        	heap_data_t data;
        	data.data=pvPortMalloc(strlen("+UUPSDD: ")+1);
        	data.len=strlen("+UUPSDD: ")+1;
        	strcpy(data.data,"+UUPSDD: ");
//        	extern osMessageQueueId_t lteResponseHandle;
//        	osMessageQueuePut(lteResponseHandle, &data, 0U, 0U);
		}
        else if (__check_cmd("mqtt start")){
			debug("start mqtt\n");
//			extern osEventFlagsId_t NetworkEventHandle;
//			osEventFlagsSet(NetworkEventHandle, NETWORK_READY_EVENT);
		}
        else if(__check_cmd("lte reset pulse ")){
        	int period, repeat;
        	if(sscanf(__param_pos("lte reset pulse "), "%d %d", &period, &repeat)==2){
        		debug("Set reset pulse to: %d %d\n", period, repeat);
        		while(repeat--){
        			lara_r2_reset_control(0);
        			delay(period/2);
        			lara_r2_reset_control(1);
					delay(period/2);
        		}
        	}
        }
        else if(__check_cmd("lte power pulse ")){
			int period, repeat;
			if(sscanf(__param_pos("lte power pulse "), "%d %d", &period, &repeat)==2){
				debug("Set power pulse to: %d %d\n", period, repeat);
				while(repeat--){
					lara_r2_power_control(0);
					delay(period/2);
					lara_r2_power_control(1);
					delay(period/2);
				}
			}
		}
        else if (__check_cmd("factory reset")){
            warning("Factory reset\n");
        }
        else if (__check_cmd("reset")){
            info("System reset...\n");
            ioctl_beep(500);
            NVIC_SystemReset();
        }
        else if(__check_cmd("blink ")){
			GPIO_TypeDef *port;
			int Pin;
			char port_char;
			result=__param_pos("blink ");
			if(sscanf(result, "%c %d", &port_char, &Pin)==2){
				if(port_char=='a'){
					port=GPIOA;
				}
				else if(port_char=='b'){
					port=GPIOB;
				}
				else if(port_char=='c'){
					port=GPIOC;
				}
				else return;
				debug("blink gpio: %c, pin :%d\n", port_char, Pin);
				blink_start(port, 1<<Pin);
			}
			else if(__check_cmd("stop")){
				blink_stop();
			}
		}
    }
}
