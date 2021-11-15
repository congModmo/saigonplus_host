/*
 * app_display.c
 *
 *  Created on: Apr 21, 2021
 *      Author: thanhcong
 */
#define __DEBUG__ 3
#include "app_display.h"
#include "display_parser/display_parser.h"
#include "ringbuf/ringbuf.h"
#include "bsp.h"
#include "log_sys.h"
#include "debounce/debounce3.h"
#include "app_main/publish_scheduler.h"
#include "light_control.h"

static RINGBUF displayRb;
static display_data_t display = {0};
const display_data_t *const display_state = &display;
static Bounce3_t lockPinDebounce;
static display_mode_t display_mode = DISPLAY_NORMAL_MODE;

static struct
{
	bool charging;
	bool full_charged;
}charge_status;

static void charge_process_handle()
{
	if(!charge_status.charging && display.charging==1)
	{
		info("start charging\n");
		charge_status.charging=true;
		light_control_set_sidelight_charge_mode(SIDELIGHT_CHARGING);
	}
	if(charge_status.charging && display.charging==0)
	{
		info("stop charing\n");
		charge_status.charging=false;
		charge_status.full_charged=false;
		light_control_set_sidelight_charge_mode(SIDELIGHT_CHARGE_NONE);
	}
	if(charge_status.charging && display.full_charged==1 && !charge_status.full_charged)
	{
		info("full charge detected\n");
		publish_scheduler_handle_full_charge();
		charge_status.full_charged=true;
		light_control_set_sidelight_charge_mode(SIDELIGHT_FULL_CHARGED);
	}
}

void display_parser_cb(int header, Display_Proto_Unified_Arg_t *value)
{
	switch (header)
	{
	case DISP_PROTO_CMD_ODO_GEAR_BMS_CHARGE:
		display.odo=(*(uint32_t *)(&value->odoGearBmsCharge.odo[0])) & 0x00FFFFFF;
		display.gear=(uint8_t)GET_GEAR(value->odoGearBmsCharge.gearBmsCharge);
		display.charging=GET_BMS_CHARGING(value->odoGearBmsCharge.gearBmsCharge);
		display.full_charged=GET_BMS_FULL_CHARGED(value->odoGearBmsCharge.gearBmsCharge);
		charge_process_handle();
		debug("-ODO: %u\n", display.odo);
		debug("-Gear: %u\n", display.gear);
		debug("-Charging: %d, full charge: %d\n", display.charging, display.full_charged);
		break;
	case DISP_PROTO_CMD_TRIP_SPD:
		display.trip=value->tripSpeed.trip;
		display.speed=value->tripSpeed.speed;
		debug("-Trip: %.1f km, speed: %.1f km/h\n",(float)display.trip / 10,  (float)display.speed / 10);
		break;
	case DISP_PROTO_CMD_VOLT:
		display.battery_voltage=value->voltage & 0xFFFF;
		debug("-Voltage: %umV\n", display.battery_voltage );
		break;
	case DISP_PROTO_CMD_CHRG:
		display.battery_charge_cycle=value->charge.cycles;
		display.battery_uncharge_time=value->charge.unchargedTime;
		debug("-Batt. charge cycles: %u, uncharge time: %u\n", display.battery_charge_cycle, display.battery_uncharge_time);
		break;
	case DISP_PROTO_CMD_BATT:
		display.battery_state= value->battery.state;
		display.battery_temperature = value->battery.temperature;
		display.battery_remain = value->battery.remainCapacity;
		debug("-Batt state: %u, temp: %.1f, remain cap. %u%%\n", display.battery_state, (float)display.battery_temperature / 10, display.battery_remain);
		break;
	case DISP_PROTO_CMD_CAP_BMS_CURRENT:
		display.battery_capacity=value->resCapBmsCurrent.resCap;
		display.bms_current=value->resCapBmsCurrent.bmsCurrent;
		debug("-Batt. residual cap. %umAh\n", display.battery_capacity & 0xFFFF);
		debug("-Bms current: %d\n", display.bms_current);
		break;
	case DISP_PROTO_CMD_LIGHT:
		debug("light: %s\n", (value->light_on?"on":"off"));
		light_control(value->light_on);
		display.light_on=value->light_on;
	default:
		break;
	}
}

bool lock_pin_get_state(void)
{
	return (HAL_GPIO_ReadPin(LOCK_GPIO_Port, LOCK_Pin) == GPIO_PIN_RESET);
}

static void lockPinUpdate()
{
	display.display_on = Bounce3_Read(&lockPinDebounce);
	if (display_mode == DISPLAY_ANTI_THEFT_MODE)
	{
		if (display.display_on)
		{
			info("Pull down uart bus\n");
			HAL_GPIO_WritePin(DISPLAY_TX_GPIO_Port, DISPLAY_TX_Pin, GPIO_PIN_RESET);
		}
		else
		{
			info("Release uart bus\n");
			HAL_GPIO_WritePin(DISPLAY_TX_GPIO_Port, DISPLAY_TX_Pin, GPIO_PIN_SET);
		}
	}
	if (display.display_on == false)
	{
		debug("Turn light off\n");
		light_control_set_sidelight_charge_mode(SIDELIGHT_CHARGE_NONE);
		light_control(false);
		ioctl_beepbeep(1, 300);
	}
	else
	{
		app_display_reset_data();
		ioctl_beepbeep(2, 100);
	}
}

void app_display_set_mode(display_mode_t mode)
{
	display_mode=mode;
	if (mode == DISPLAY_NORMAL_MODE)
	{
		info("Set display to normal mode\n");
		HAL_GPIO_DeInit(DISPLAY_TX_GPIO_Port, DISPLAY_TX_Pin);
		bsp_display_init(&displayRb);
		alarm_process_end();
	}
	else
	{
		info("Set display to anti theft mode\n");
		alarm_process_start();
		publish_scheduler_bike_lock_handle();
		bsp_display_deinit();
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		HAL_GPIO_WritePin(DISPLAY_TX_GPIO_Port, DISPLAY_TX_Pin, GPIO_PIN_SET);
		GPIO_InitStruct.Pin = DISPLAY_TX_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(DISPLAY_TX_GPIO_Port, &GPIO_InitStruct);
		lockPinUpdate();
	}
}

void app_display_init()
{
	display_parser_init(display_parser_cb);
	Bounce3_Init(&lockPinDebounce, 100, lock_pin_get_state);
	lockPinUpdate();
	light_control_init();
}

void app_display_process()
{
	light_control_process();
	if (Bounce3_Update(&lockPinDebounce))
	{
		lockPinUpdate();
	}
	while (RINGBUF_Available(&displayRb))
	{
		static uint8_t c;
		RINGBUF_Get(&displayRb, &c);
		display_parse_byte(c);
	}
}

void app_display_console_handle(char *result)
{
	if(__check_cmd("set speed "))
	{
		int speed;
		if(sscanf(__param_pos("set speed "), "%d", &speed) ==1)
		{
			info("Set device speed to %.1f\n", (float)speed/10);
			display.speed=speed;
		}
		else error("Params error\n");
	}
	else if(__check_cmd("set mode "))
	{
		int mode;
		if(sscanf(__param_pos("set mode "), "%d", &mode) ==1)
		{
			app_display_set_mode((mode==1)?DISPLAY_ANTI_THEFT_MODE:DISPLAY_NORMAL_MODE);
		}
	}
	else if(__check_cmd("set charge "))
	{
		int charging, full;
		if(sscanf(__param_pos("set charge "), "%d %d", &charging, &full) ==2)
		{
			info("Charge to: %d, full charge: %d\n", charging, full);
			display.charging=charging;
			display.full_charged=full;
			charge_process_handle();
		}
	}
	else error("Unknown cmd\n");
}

void app_display_reset_data()
{
	charge_status.charging=false;
	charge_status.full_charged=false;
	display.trip=0xffff;
	display.speed=0xffff;
	display.odo=0xffffffff;
	display.battery_voltage=0xffffffff;
	display.battery_remain=0xff;
	display.battery_temperature=0xffff;
	display.battery_state=0xC0;
	display.battery_charge_cycle=0xffff;
	display.battery_uncharge_time=0xffff;
	display.battery_capacity=0xffffffff;
	display.battery_event=0;
	display.charging=0xff;
	display.full_charged=0xff;
	display.bms_current=0x7fff;
}
