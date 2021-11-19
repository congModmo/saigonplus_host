
#define __DEBUG__ 4
#include "log_sys.h"
#include <string.h>
#include <stdlib.h>
#include "protocol_v3.h"
#include "app_gps/app_gps.h"
#include "app_display/app_display.h"
#include "app_lte/lteTask.h"
#include "app_rtc.h"
#include "time.h"
#include "crc/crc16.h"
#include "app_common/common.h"
#include "app_main/app_main.h"
#include "app_config.h"
#include "app_info.h"

typedef struct{
	uint8_t cmd_id;
	uint16_t status_id;
}protocol_v3_t;

static protocol_v3_t protocol={0};

void print_frame(uint8_t *data, uint16_t len)
{
	debug("Frame: [");
	for(int i=0; i<len -1; i++)
	{
		debugx(" 0x%x,", data[i]);
	}
	debugx(" 0x%x]\n", data[len-1]);
}

static void send_status_topic_message(uint8_t type, uint8_t *data, size_t len)
{
	size_t msg_len=len + sizeof(status_msg_header_t) + 2;
	uint8_t *message=(uint8_t *)malloc(msg_len);
	if(message == NULL)
		return ;
	status_msg_header_t *header =(status_msg_header_t *) message;
	header->type=type;
	header->id=protocol.status_id;
	protocol.status_id++;
	header->timestamp=rtc_to_timestamp();
	memcpy(message + sizeof(status_msg_header_t), data, len);
	crc16_append(message, msg_len-2);
#ifdef LTE_ENABLE
	mail_t mail={.type=MAIL_MQTT_V2_PUBLISH, .data=message, .len=msg_len};
	if(osMessageQueuePut(mqttMailHandle, &mail, 0, 10)!=osOK){
		error("Mail put failed");
		free(message);
	}
#else
	print_frame(message, msg_len);
	free(message);
#endif
}

void send_ack_message(uint8_t type, uint8_t id, uint8_t ack)
{
	size_t msg_len=1+ sizeof(cmd_msg_header_t) + 2;
	uint8_t *message=(uint8_t *)malloc(msg_len);
	if(message == NULL)
		return ;
	cmd_msg_header_t *header =(cmd_msg_header_t *) message;
	header->type=type;
	header->id=id;
	header->timestamp=rtc_to_timestamp();
	uint8_t *p_ack=message+sizeof(cmd_msg_header_t);
	*p_ack=ack;
	crc16_append(message, msg_len-2);
#ifdef LTE_ENABLE
	mail_t mail={.type=MAIL_MQTT_V2_ACK, .data=message, .len=msg_len};
	if(osMessageQueuePut(mqttMailHandle, &mail, 0, 10)!=osOK){
		error("Mail put failed");
		free(message);
	}
#else
	print_frame(message, msg_len);
	free(message);
#endif
}

void send_riding_status_msg(display_data_t *display, gps_data_t *gps )
{
	static riding_status_msg_t msg;
	msg.trip=display->trip;
	msg.average_speed=display->speed;
	msg.odo=display->odo;
	msg.battery_voltage=display->battery_voltage;
	msg.battery_remain=display->battery_remain;
	msg.battery_temperature=display->battery_temperature;
	msg.gear=display->gear;
	msg.charging=display->charging;
	msg.full_charged=display->full_charged;
	msg.bms_current=display->bms_current;
#ifdef PROTOCOL_V3_UNIT_TEST
	msg.lte_signal=10;
#else
	msg.lte_signal=(uint8_t)(*lteRssi);
#endif
	msg.gps_signal=(uint8_t)((gps->hdop < 20.0)?(gps->hdop*10):0);
	msg.longitude=gps->longitude;
	msg.latitude=gps->latitude;
	send_status_topic_message(STATUS_TOPIC_RIDING, (uint8_t *)&msg, sizeof(riding_status_msg_t));
}

static void send_keep_alive_msg(gps_data_t *gps)
{
	static keep_alive_msg_t msg;
#ifdef PROTOCOL_V3_UNIT_TEST
	msg.lte_signal=10;
	msg.report_disabled=1;
	msg.bike_lock=0;
#else
	msg.lte_signal=(uint8_t)(*lteRssi);
#endif
	msg.gps_signal=(uint8_t)((gps->hdop < 20.0)?(gps->hdop*10):0);
	msg.longitude=gps->longitude;
	msg.latitude=gps->latitude;
	msg.bike_lock=(uint8_t) *bike_locked;
	msg.report_disabled=publish_setting->report_disabled;
	msg.display_on=(uint8_t)display_state->display_on;
	send_status_topic_message(STATUS_TOPIC_KEEP_ALIVE, (uint8_t *)&msg, sizeof(keep_alive_msg_t));
}

void send_theft_msg()
{
}

void send_fall_msg(uint8_t fall)
{
	send_status_topic_message(STATUS_TOPIC_FALL, &fall, 1);
}

void setting_cmd_handler(uint8_t *cmd, uint8_t id)
{
	publish_setting_cmd_t *setting=(publish_setting_cmd_t *)cmd;
	debug("min report interval: %lu\n", setting->min_report_interval);
	debug("max report interval: %lu\n", setting->max_report_interval);
	debug("keep alive interval: %lu\n", setting->keep_alive_interval);
	debug("report disable: %u\n", setting->report_disabled);
	app_info_update_publish_setting(setting);
	send_ack_message(CMD_TOPIC_SETTING, id, 1);
}

void lock_cmd_handler(uint8_t *cmd, uint8_t id)
{
	lock_cmd_t *lock =(lock_cmd_t *)cmd;
	debug("Set device to %d state\n", lock->mode);
	app_info_update_lock_state(lock->mode);
	send_ack_message(CMD_TOPIC_LOCK, id, 1);
	if(lock->mode)
	{
		app_display_lock_bike();
	}
	else
	{
		app_display_unlock_bike(true);
	}
}

void fota_cmd_handler(uint8_t *cmd, uint8_t id)
{
	char *link =(char *)cmd;
	debug("fota from link: %s\n", link);
	send_ack_message(CMD_TOPIC_FOTA, id, 1);
}

void imu_cmd_handler(uint8_t *cmd)
{
	imu_cmd_t *imu=(imu_cmd_t *)cmd;
	debug("Set imu sensitivity to %u\n", imu->value);
}

void protocol_v3_handle_cmd(uint8_t *cmd, uint16_t len)
{
	if(!crc16_frame_check(cmd, len)){
		error("Crc error\n");
		return;
	}
	cmd_msg_header_t *header =(cmd_msg_header_t *)cmd;
	debug("cmd type: %u\n", header->type);
	debug("cmd id: %u\n", header->id);
	debug("timestamp: %lu\n", header->timestamp);
	if(header->type==CMD_TOPIC_SETTING)
	{
		setting_cmd_handler(cmd+ sizeof(cmd_msg_header_t), header->id);
	}
	else if(header->type==CMD_TOPIC_LOCK)
	{
		lock_cmd_handler(cmd+sizeof(cmd_msg_header_t), header->id);
	}
	else if(header->type==CMD_TOPIC_FOTA)
	{
		cmd[len-2]=0; //server string has no string delimiter, lets base on message len
		fota_cmd_handler(cmd+sizeof(cmd_msg_header_t), header->id);
	}
}

static void send_battery_msg(display_data_t *display)
{
	static battery_msg_t msg;
	msg.charging_cycle=display->battery_charge_cycle;
	msg.event=display->battery_event;
	msg.remain_percent=display->battery_remain;
	msg.residual_capacity=display->battery_capacity;
	msg.state=display->battery_state;
	msg.temperature=display->battery_temperature;
	msg.uncharge_time=display->battery_uncharge_time;
	msg.votage=display->battery_voltage;
	send_status_topic_message(STATUS_TOPIC_BATT_EVENT, (uint8_t *)&msg, sizeof(battery_msg_t));
}

void publish_riding_message()
{
	send_riding_status_msg(display_state, gps_data);
}

void publish_keep_alive_message()
{
	send_keep_alive_msg(gps_data);
}

void publish_device_info_message(void)
{
	device_info_msg_t msg ={.hw_version = factory_config->hardwareVersion,\
					.ble_version=firmware_version->bleApp,\
					.host_version=firmware_version->hostApp};
	send_status_topic_message(STATUS_TOPIC_DEVICE_INFO, (uint8_t *)&msg, sizeof(device_info_msg_t));
}

void publish_battery_message()
{
	send_battery_msg(display_state);
}

void publish_theft_message()
{
	static theft_msg_t theft;
	theft.theft=1;
	theft.longitude=gps_data->longitude;
	theft.latitude=gps_data->latitude;
	send_status_topic_message(STATUS_TOPIC_THEFT, &theft, sizeof(theft_msg_t));
}

void TEST_protocol_v3()
{
	char at_str[]="+CCLK: \"21/8/10,09:00:00+01\"";
	set_rtc_from_at_string(at_str);
	display_data_t display;
	gps_data_t gps;
	device_info_msg_t info={.hw_version=10, .host_version=10, .ble_version=10};
	display.odo=10;
	display.trip=100; // x10
	display.speed=100;// x10
	display.battery_voltage=10000; // x1000
	display.battery_remain=10;
	display.battery_temperature=100;
	display.battery_charge_cycle=10;
	display.battery_state=0xff;
	display.battery_uncharge_time=10;
	display.battery_capacity=10;
	display.battery_event=3;
	display.gear=10;
	gps.hdop=10.0;
	gps.latitude=10.0;
	gps.longitude=10.0;
	send_riding_status_msg(&display, &gps);
	send_keep_alive_msg(&gps);
	send_theft_msg(1);
	send_fall_msg(1);
	send_battery_msg(&display);

	uint8_t setting_cmd[]= {
			 1,   0, 224, 73,  18, 97, 136,
			    19,   0,   0, 96, 234,  0,   0,
			    96, 234,   0,  0,   1, 82,  82
	};

	uint8_t lock_cmd[]={
			2, 0, 224, 73, 18,
			    97, 1, 243, 50
	};

	uint8_t fota_cmd[]={
			3,   0, 224,  73,  18, 97,
			    109, 111, 100, 109, 111, 46,
			     99, 111, 109,  65, 103
	};

	uint8_t imu_cmd[]={
			 4,  0, 224, 73, 18,
			    97, 10, 189, 34
	};

	protocol_v3_handle_cmd(setting_cmd, sizeof(setting_cmd));
	protocol_v3_handle_cmd(lock_cmd, sizeof(lock_cmd));
	protocol_v3_handle_cmd(fota_cmd, sizeof(fota_cmd));
	protocol_v3_handle_cmd(imu_cmd, sizeof(imu_cmd));
}
