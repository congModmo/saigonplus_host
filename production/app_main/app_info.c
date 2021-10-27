/*
 * app_resource.c
 *
 *  Created on: Mar 19, 2021
 *      Author: thanhcong
 */
#define __DEBUG__ 4
#include <string.h>
#include "app_info.h"
#include "fota_comm.h"
#include "crc/crc16.h"
#include "ext_flash/GD25Q16.h"
#include "bsp.h"

typedef struct
{
	firmware_version_t firmware_version;
	publish_setting_cmd_t publish_setting;
	user_config_t user_config;
	char serial_number[32];
	uint16_t crc;
} app_info_setting_t;

typedef struct 
{
	bool lock;
	uint16_t crc;
}app_info_lock_t;

static app_info_setting_t app_setting={0};
static app_info_lock_t lock_state;
static factory_config_t factory;
static host_ble_info_t _host_ble_info;

const host_ble_info_t * const host_ble_info =&_host_ble_info;
const factory_config_t *const factory_config = (const factory_config_t *)&factory;
//const factory_config_t *const factory_config = (const factory_config_t *)FLASH_DEVICE_FACTORY_INFO;
const user_config_t * const user_config=&app_setting.user_config;
const firmware_version_t *const firmware_version=&app_setting.firmware_version;
const char *const serial_number=app_setting.serial_number;
const bool * const bike_locked=&lock_state.lock;
const publish_setting_cmd_t *const publish_setting=&app_setting.publish_setting;


#define app_info_update_setting() app_info_update((uint8_t *)&app_setting, sizeof(app_info_setting_t), EX_FLASH_APP_INFO)

static void app_info_update(uint8_t *data, size_t len, uint32_t flash_addr)
{
	crc16_append(data, len-2);
	GD25Q16_WriteAndVerifySector(flash_addr, data, len);
}

void app_config_factory_reset()
{
	app_setting.firmware_version.hostApp= DEFAULT_HOST_APP_VERSION;
	app_setting.firmware_version.bleApp = DEFAULT_BLE_APP_VERSION;
	app_setting.publish_setting.min_report_interval = DEFAULT_MIN_REPORT_INTERVAL;
	app_setting.publish_setting.max_report_interval = DEFAULT_MAX_REPORT_INTERVAL;
	app_setting.publish_setting.keep_alive_interval = DEFAULT_KEEP_ALIVE_INTERVAL;
	app_setting.publish_setting.report_disabled = DEFAULT_PUBLISH_DISABLED;
	app_setting.user_config.beep_sound = DEFAULT_BEEP_SOUND;
	app_setting.user_config.head_light = DEFAULT_HEAD_LIGHT;
	app_setting.user_config.side_light.red = DEFAULT_SIDE_LIGHT_RED;
	app_setting.user_config.side_light.green = DEFAULT_SIDE_LIGHT_GREEN;
	app_setting.user_config.side_light.blue = DEFAULT_SIDE_LIGHT_BLUE;
	app_setting.user_config.imu_sensitivity = DEFAULT_IMU_SENSITIVITY;
	strcpy(app_setting.serial_number, DEFAULT_SERIAL_NUMBER);
	app_info_update_setting();
}

static void app_config_init()
{
	GD25Q16_ReadSector(EX_FLASH_APP_INFO, (uint8_t *)&app_setting, sizeof(app_info_setting_t));
	if(crc16_frame_check((uint8_t *)&app_setting, sizeof(app_setting)))
	{
		debug("Valid app config\n");
		return;
	}
	debug("Set default app config\n");
	app_config_factory_reset();
}

static void app_lock_state_update(bool state)
{
	lock_state.lock=state;
	app_info_update((uint8_t *)&lock_state, sizeof(app_info_lock_t), EX_FLASH_LOCK_STATE);
}

static void app_lock_init()
{
	GD25Q16_ReadSector(EX_FLASH_LOCK_STATE, (uint8_t *)&lock_state, sizeof(lock_state));
	if(crc16_frame_check((uint8_t *)&lock_state, sizeof(app_info_lock_t)))
	{
		debug("Valid lock config\n");
		return;
	}
	debug("Set default lock setting\n");
	app_lock_state_update(DEFAULT_LOCK_STATE);
}

void app_host_ble_info_init()
{
	_host_ble_info.bleVersion=app_setting.firmware_version.bleApp;
	_host_ble_info.hostVersion=app_setting.firmware_version.hostApp;
	_host_ble_info.hwVersion=factory.hardwareVersion;
	strcpy(_host_ble_info.serial, app_setting.serial_number);
	crc16_append((uint8_t *)&_host_ble_info, sizeof(host_ble_info_t)-2);
}

void app_info_init()
{
	app_lock_init();
	app_config_init();
	factory.hardwareVersion=0x21;
	factory.broker.port=8883;
	factory.broker.secure=1;
	strcpy(factory.broker.endpoint, "a28c4si2pkzbml-ats.iot.ap-southeast-1.amazonaws.com");
	app_host_ble_info_init();
}

void app_info_update_firmware_version(uint16_t hostApp, uint16_t bleApp)
{
	if(hostApp>0)
	{
		app_setting.firmware_version.hostApp=hostApp;
	}
	if(bleApp>0)
	{
		app_setting.firmware_version.bleApp=bleApp;
	}
	app_info_update_setting();
}

// frequently config param, has it own flash sector
void app_info_update_lock_state(bool lock)
{
	app_lock_state_update(lock);
}

// sometime config param, add to the same sector
void app_info_update_user_config(user_config_t * config)
{
	app_setting.user_config=*config;
	app_info_update_setting();
}

void app_info_update_serial_number(char *sn)
{
	if(strlen(sn)>=sizeof(app_setting.serial_number))
	{
		error("serial number len\n");
	}
	else
	{
		strcpy(app_setting.serial_number, sn);
		app_info_update_setting();
	}
}

void app_info_update_publish_setting(publish_setting_cmd_t *setting)
{
	if(setting->keep_alive_interval==0 || setting->max_report_interval ==0 || setting->min_report_interval ==0){
		info("update en/dis only\n");
		app_setting.publish_setting.report_disabled=setting->report_disabled;
	}
	else{
		info("update all publish setting\n");
		app_setting.publish_setting=*setting;
	}
	app_info_update_setting();
}

void app_info_update_headlight(int hl)
{
	app_setting.user_config.head_light=hl;
	app_info_update_setting();
}

void app_info_update_sidelight(int red, int green, int blue)
{
	app_setting.user_config.side_light.red=red;
	app_setting.user_config.side_light.green=green;
	app_setting.user_config.side_light.blue=blue;
	app_info_update_setting();
}

void app_info_update_beep_sound(bool beep)
{
	app_setting.user_config.beep_sound=beep;
	app_info_update_setting();
}

void app_info_update_imu(int bss)
{
	app_setting.user_config.imu_sensitivity=bss;
	app_info_update_setting();
}

void app_info_console_handle(char *result)
{
	if(__check_cmd("set publish "))
	{
		int min, max, keep_alive, disabled;
		if(sscanf(__param_pos("set publish "), "%d %d %d %d", &min, &max, &keep_alive, &disabled)==4)
		{
			publish_setting_cmd_t _publish_setting;
			_publish_setting.min_report_interval = min;
			_publish_setting.max_report_interval = max;
			_publish_setting.keep_alive_interval = keep_alive;
			_publish_setting.report_disabled = disabled;
			debug("Publish setting min: %u, max: %u, keep alive: %u, disable: %u\n", min, max, keep_alive, disabled);
			app_info_update_publish_setting(&_publish_setting);
		}
		else debug("Params error\n");
	}
	else if(__check_cmd("set user config "))
	{
		int beep, front, red, green, blue, imu;
		if(sscanf(__param_pos("set user config "), "%d %d %d %d %d %d", &beep, &front, &red, &green, &blue, &imu)==6)
		{
			user_config_t _user_config;
			_user_config.beep_sound = beep;
			_user_config.head_light = front;
			_user_config.side_light.red = red;
			_user_config.side_light.green = green;
			_user_config.side_light.blue = blue;
			_user_config.imu_sensitivity = imu;
			debug("Set user config to: %d %d %d %d %d %d\n", beep, front, red, green, blue, imu);
			app_info_update_user_config(&_user_config);
		}
		else debug("Params error\n");
	}
	else if(__check_cmd("set serial number "))
	{
		char sn[32];
		if(sscanf(__param_pos("set serial number "), "%s", sn)==1)
		{
			app_info_update_serial_number(sn);
			debug("Set serial num ber to: %s\n", sn);
		}
		else debug("Params error\n");
	}
	else if(__check_cmd("set firmware version"))
	{
		int ble, app;
		if(sscanf(__param_pos("set firmware version"), "%d %d", &app, &ble)==2)
		{
			app_info_update_firmware_version(app, ble);
			debug("Set firmware version to app: %d, ble: %d\n", app, ble);
		}
		else debug("Params error\n");
	}
	else if(__check_cmd("set lock"))
	{
		int lock;
		if(sscanf(__param_pos("set lock "), "%d", &lock)==1)
		{
			app_info_update_lock_state(lock);
			debug("Set lock to: %d\n", lock);
		}
		else debug("Params error\n");
	}
	else if(__check_cmd("factory reset"))
	{
		app_config_factory_reset();
	}
	else debug("Unknow command\n");
}
