#ifndef APP_INFO_H
#define APP_INFO_H

#include "protocol_v3.h"
#include "app_ble/app_ble.h"

typedef struct
{
    char endpoint[128];
    int port;
    int secure;
} broker_t;

typedef struct
{
    uint16_t hardwareVersion;
    broker_t broker;
} factory_config_t;

typedef struct 
{
    uint16_t hostApp;
    uint16_t bleApp;
}firmware_version_t;

#define DEFAULT_HOST_APP_VERSION 1
#define DEFAULT_BLE_APP_VERSION 1
#define DEFAULT_MIN_REPORT_INTERVAL 5000
#define DEFAULT_MAX_REPORT_INTERVAL 60000
#define DEFAULT_KEEP_ALIVE_INTERVAL 60000
#define DEFAULT_PUBLISH_DISABLED 0
#define DEFAULT_BEEP_SOUND 1
#define DEFAULT_HEAD_LIGHT 50
#define DEFAULT_SIDE_LIGHT_RED 255
#define DEFAULT_SIDE_LIGHT_GREEN 0
#define DEFAULT_SIDE_LIGHT_BLUE 0
#define DEFAULT_IMU_SENSITIVITY 2 //(1 2 3)
#define DEFAULT_LOCK_STATE 0

extern const factory_config_t * const factory_config;
extern const user_config_t * const user_config;
extern const firmware_version_t *const firmware_version;
extern const char *const serial_number;
extern const publish_setting_cmd_t *const publish_setting;
extern const bool *const bike_lock;

void app_info_init();
// frequently config param, has it own flash sector
void app_info_update_lock_state(bool lock);
// sometime config param, add to the same sector
void app_info_update_firmware_version(uint16_t hostApp, uint16_t bleApp);
void app_info_update_serial_number(char *sn);
void app_info_update_publish_setting(publish_setting_cmd_t *setting);
void app_info_update_imu_sensitivity(uint8_t sen);
void app_info_update_headlight(int hl);
void app_info_update_sidelight(int red, int green, int blue);
void app_info_update_beep_sound(bool beep);
void app_info_update_imu(int bss);
#endif
