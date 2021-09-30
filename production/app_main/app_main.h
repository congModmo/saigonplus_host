/*
 * app_main.h
 *
 *  Created on: Mar 16, 2021
 *      Author: thanhcong
 */

#ifndef APP_MAIN_H_
#define APP_MAIN_H_
#include "FreeRTOS.h"
#include "cmsis_os.h"

void app_main(void);
#include <stdbool.h>
#include <stdlib.h>
#include "app_fota/fota_core.h"
#include "app_info.h"
#include "lte/lara_r2_socket.h"
#include "app_fota/lte_transport.h"

enum{
	MAIL_MQTT_V1_PUBLISH_FRAME,
	MAIL_MQTT_V1_LOCATIONMODE,
	MAIL_MQTT_V2_PUBLISH,
	MAIL_MQTT_V2_ACK,
	MAIL_MQTT_V2_SERVER_DATA,
	MAIL_LTE_HTTP_GET_FILE,
	MAIL_LTE_NETWORK_RESTART,
};

typedef struct __packed{
	uint8_t type;
	uint8_t *data;
	uint16_t len;
}mail_t;

extern osMessageQueueId_t lteMailHandle;
extern osMessageQueueId_t mainMailHandle;
extern osMessageQueueId_t mqttMailHandle;
bool system_is_ready();
#endif /* APP_MAIN_APP_MAIN_H_ */
