#ifndef __PROTOCOL_V3_H_
#define __PROTOCOL_V3_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//from link: https://docs.google.com/document/d/1YzHEPp2r7-cKaIwWEbiJdnCwA-n7QJNk/edit

typedef struct __packed{
	uint8_t type;
	uint16_t id;
	uint32_t timestamp;
}status_msg_header_t;

typedef struct __packed{
	uint8_t type;
	uint8_t id;
	uint32_t timestamp;
}cmd_msg_header_t;

enum{
    STATUS_TOPIC_DEVICE_INFO=1,
    STATUS_TOPIC_RIDING=2,
    STATUS_TOPIC_KEEP_ALIVE=3,
    STATUS_TOPIC_THEFT =4,
    STATUS_TOPIC_FALL =5,
    STATUS_TOPIC_BATT_EVENT=6,
}status_topic_type_t;

enum{
    CMD_TOPIC_SETTING=1,
    CMD_TOPIC_LOCK=2,
    CMD_TOPIC_FOTA=3,
	CMD_TOPIC_IMU_SENSITIVITY=4,
}cmd_topic_type_t;

typedef struct __packed{
    uint16_t hw_version;
    uint16_t host_version;
    uint16_t ble_version;
}device_info_msg_t;

typedef struct __packed{
    uint16_t trip;
    uint16_t average_speed;
    uint32_t odo;
    uint32_t battery_voltage;
    uint8_t battery_remain;
    uint16_t battery_temperature;
    double longitude;
    double latitude;
    uint8_t gps_signal;
    uint8_t lte_signal;
    uint8_t gear;
    uint8_t charging;
    uint8_t full_charged;
    int16_t bms_current;
}riding_status_msg_t;

typedef struct __packed{
    double longitude;
    double latitude;
    uint8_t gps_signal;
    uint8_t lte_signal;
    uint8_t bike_lock;
    uint8_t report_disabled;
    uint8_t display_on;
}keep_alive_msg_t;

typedef struct __packed{
    uint8_t theft;
    double longitude;
    double latitude;
}theft_msg_t;

typedef struct __packed{
    uint8_t fall;
}fall_msg_t;

typedef struct __packed{
    uint32_t votage;
    uint8_t remain_percent;
    uint8_t state;
    uint16_t temperature;
    uint16_t charging_cycle;
    uint16_t uncharge_time;
    uint32_t residual_capacity;
    uint8_t event;
}battery_msg_t;

typedef struct __packed{
    uint32_t min_report_interval;
    uint32_t max_report_interval;
    uint32_t keep_alive_interval;
    uint8_t report_disabled;
}publish_setting_cmd_t;

typedef struct __packed{
    uint8_t mode;
}lock_cmd_t;

typedef struct __packed{
	uint8_t value;
}imu_cmd_t;

uint32_t (*get_utc_timestamp_t)(void);
uint16_t (* get_crc16_t)(uint8_t *data, uint16_t len);

//void protocol_v3_init(get_utc_time_stamp_t utc, get_crc16_t get_crc16)
//{
//
//}

void send_ack(uint8_t type, uint8_t id, uint8_t ack);

#ifdef __cplusplus
}
#endif
#endif
