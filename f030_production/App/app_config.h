#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#ifndef RELEASE
	#define GPS_ENABLE

	#define ALARM_ENABLE

	#define BLE_ENABLE

//	#define LTE_ENABLE

	#define DISPLAY_ENABLE

	#define IMU_ENABLE

	#define PUBLISH_ENABLE

	#define MQTT_ENABLE
#else

	#define ALARM_ENABLE

	#define GPS_ENABLE

	#define BLE_ENABLE

	#define LTE_ENABLE

	#define DISPLAY_ENABLE

	#define IMU_ENABLE

	#define REPORT_ENABLE

	#define MQTT_ENABLE
#endif

#define SYSTEM_READY_EVENT  0x01U
#define NETWORK_READY_EVENT 0x02U
#define MQTT_READY_EVENT	0x04U

#endif
