/*
 *
 *
 *
 *
 *
 *
 *
 *
 */

#define __DEBUG__ 4
#include <string.h>
#include <stdio.h>
#include "app_ble.h"
#include <ble/nina_b1.h>
#include "crc/crc16.h"
#include "log_sys.h"
#include "ext_flash/GD25Q16.h"
#include "app_main/app_info.h"
#include <app_fota/serial_transport.h>
#include "ble/nina_b1.h"
#include "app_config.h"
#include "app_lte/lteTask.h"
#include "app_common/wcb_ioctl.h"
#include "host_ble_comm.h"
#include "app_main/app_info.h"
#include "app_display/app_display.h"

static frame_handler transport_handler = NULL;
#define APP_BLE_CHECK_CMD(cmd2) (strncmp(cmd, cmd2, strlen(cmd2)) == 0)
static char mac[32];
const char * const ble_mac=mac;
static char ble_resp[256]={0};
#ifdef JIGTEST
bool host_ble_info_sync=false;
#endif

static struct{
	bool connected;
	uint32_t connect_tick;
	bool auth;
}ble_auth;

const bool *const ble_authenticated=&ble_auth.auth;

static void ble_request_cmd(uint8_t type)
{
	uint8_t cmd[2]={HOST_COMM_BLE_MSG, type};
	nina_b1_send0(cmd, 2);
}

static void app_ble_handle_ui_raw_packet(uint8_t * packet, size_t len)
{
	if (packet[0] == FOTA_REQUEST_FOTA)
	{
		serial_fota_request_info_t *request;
		request = (serial_fota_request_info_t *)(packet + 1);
		serial_fota_request_handle(request, FOTA_OVER_BLE) ;
	}
	else if (packet[0] == FOTA_REQUEST_MTU)
	{
//		debug("Send mtu: %d\n", ble_serial.mtu);
		uint8_t frame[2] = {DEVICE_SET_MTU, ble_serial.mtu};
		nina_b1_send1(HOST_COMM_UI_MSG, frame, 2);
	}
}

static void ble_bool_response(char *cmd, bool ok, char **res)
{
	sprintf(ble_resp, "%s %s", cmd, (ok?"OK":"ERROR"));
	*res=ble_resp;
}
static void app_ble_handle_ui_string_cmd(char * result)
{
	char *response=NULL;
	bool reset=false;
	if(!ble_auth.auth)
	{
		if(!__check_cmd(REQ_CHECK_ID))
		{
			return;
		}
		result+=strlen(REQ_CHECK_ID)+1;
		if(strlen(lteImei)>0 && strlen(lteCcid)>0 && 0==strcmp(lteImei, result))
		{
			ble_auth.auth=true;
			info("ble authentcated\n");
			sprintf(ble_resp, "%s OK %s", REQ_CHECK_ID, lteCcid);
			if(*bike_locked)
			{
				info("unlock bike\n");
				if(*bike_locked)
					app_display_unlock_bike(false);
			}
			response=ble_resp;
		}
		else
		{
			ble_bool_response(REQ_CHECK_ID, false, &response);
		}
		goto __exit;
	}
	if (__check_cmd(REQ_SYS_INFO))
	{
		//<HARDWARE_VERSION> <HOST_VERSION> <BLE_VERSION>
		sprintf(ble_resp, "%s OK %u.%u %u.%u %u.%u",REQ_SYS_INFO,\
				factory_config->hardwareVersion>>8, factory_config->hardwareVersion&0x0FF,\
				firmware_version->hostApp>>8, firmware_version->hostApp&0x0FF,\
				firmware_version->bleApp>>8, firmware_version->bleApp&0x0FF);
		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)ble_resp, strlen(ble_resp));
	}
	else if (__check_cmd(REQ_SYS_RESET))
	{
		sprintf(ble_resp, "%s OK", REQ_SYS_RESET);
		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)ble_resp, strlen(ble_resp));
		reset=true;
	}
	else if (__check_cmd(REQ_CONF_HL))
	{
		result += strlen(REQ_CONF_HL) + 1;
		int headlight;
		if (sscanf(result, "%d", &headlight) == 1)
		{
			app_info_update_headlight(headlight);
			light_control_restart();
			ble_bool_response(REQ_CONF_HL, true, &response);
		}
		else
		{
			ble_bool_response(REQ_CONF_HL, false, &response);
		}
	}
	else if (__check_cmd(REQ_CONF_BEEP))
	{
		result += strlen(REQ_CONF_BEEP) + 1;
		int beep;
		if (sscanf(result, "%d", &beep) == 1)
		{
			ble_bool_response(REQ_CONF_BEEP, true, &response);
			app_info_update_beep_sound(beep);
		}
		else
		{
			ble_bool_response(REQ_CONF_BEEP, false, &response);
		}
	}
	else if (__check_cmd(REQ_CONF_SL))
	{
		result += strlen(REQ_CONF_SL) + 1;
		int red, green, blue;
		if (sscanf(result, "%d %d %d", &red, &green, &blue) == 3)
		{
			ble_bool_response(REQ_CONF_SL, true, &response);
			app_info_update_sidelight(red, green, blue);
			light_control_restart();
		}
		else
		{
			ble_bool_response(REQ_CONF_SL, false, &response);
		}
	}
	else if (__check_cmd(REQ_CONF_GET))
	{
		sprintf(ble_resp, "%s OK %d %d %d %d %d %d", REQ_CONF_GET, user_config->head_light, user_config->side_light.red,
					  user_config->side_light.green, user_config->side_light.blue, user_config->beep_sound, user_config->imu_sensitivity );
		response=ble_resp;
	}
	else if(__check_cmd(REQ_CONF_SN))
	{
		char sn[32];
		if(sscanf(__param_pos(REQ_CONF_SN), "%s", sn)==1)
		{
			debug("ble update sn: %d\n", sn);
			app_info_update_serial_number(sn);
			ble_bool_response(REQ_CONF_SN, true, &response);
		}
		else
		{
			ble_bool_response(REQ_CONF_SN, false, &response);
		}
	}
	else if(__check_cmd(REQ_AUTO_LOCK))
	{
		result+=strlen(REQ_AUTO_LOCK)+1;
		if(__check_cmd("ON "))
		{
			int delay;
			result+=strlen("ON ");
			if(sscanf(result, "%d", &delay)==1)
			{
				if(delay<0) delay=0;
				if(delay>600) delay=600;
				info("Set auto lock on delay %d s\n", delay);
				app_info_update_auto_lock(true, delay);
				ble_bool_response(REQ_AUTO_LOCK, true, &response);
			}
			else
			{
				error("param error\n");
				ble_bool_response(REQ_AUTO_LOCK, false, &response);
			}
		}
		else if(__check_cmd("OFF"))
		{
			info("Set auto lock OFF\n");
			ble_bool_response(REQ_AUTO_LOCK, true, &response);
			app_info_update_auto_lock(false, 0);
		}
		else
		{
			error("param error\n");
			ble_bool_response(REQ_AUTO_LOCK, false, &response);
		}
	}
	else if(__check_cmd(REQ_CONF_MSS))
	{
		int mss;
		result+=strlen(REQ_CONF_MSS)+1;
		if(sscanf(result, "%d", &mss)==1)
		{
			debug("set mss level to: %d\n", mss);
			app_info_update_imu(mss);
			ble_bool_response(REQ_CONF_MSS, true, &response);
			app_imu_init();
		}
		else
		{
			error("parse params\n");
			ble_bool_response(REQ_CONF_MSS, false, &response);
		}
	}
	__exit:
	if(response!=NULL)
	{
		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)response, strlen(response));
		buzzer_beepbeep(1, 100, false);
	}
	if(reset)
	{
		delay(500);
		NVIC_SystemReset();
	}
}

static void app_ble_handle_set_mac(uint8_t *data, uint16_t len)
{
	if(len >= sizeof(mac))
	{
		error("Mac len");
	}
	else
	{
		memcpy(mac, data, len);
		mac[len]=0;
		debug("Ble mac: %s\n", mac);
	}
}

void host_comm_ble_msg_handle(uint8_t *msg, size_t len)
{
	if(msg[0]==BLE_CMD_SET_MAC)
	{
		app_ble_handle_set_mac(msg+1, len-1);
	}
	else if(msg[0]==BLE_STATE_CONNECTED)
	{
		debug("Ble connected\n");
		ble_auth.auth=false;
		ble_auth.connected=true;
		ble_auth.connect_tick=millis();
	}
	else if(msg[0]==BLE_CMD_SYNC_INFO)
	{
		nina_b1_send1(HOST_BLE_RES_INFO, (uint8_t *)host_ble_info, sizeof(host_ble_info_t));
		debug("response to ble sync cmd\n");
#ifdef JIGTEST
		host_ble_info_sync=true;
#endif
	}
	else if(msg[0]==BLE_STATE_DISCONNECTED)
	{
		debug("Ble disconnected\n");
		ble_auth.connected=false;
		ble_auth.auth=false;
	}
}

void host_comm_ui_msg_handle(uint8_t *msg, size_t len)
{
	if(msg[0] <128)
	{
		msg[len]=0;
		debug("Ui msg: %s\n", msg);
		app_ble_handle_ui_string_cmd((char *)(msg));
	}
	else
	{
		app_ble_handle_ui_raw_packet(msg, len);
		ble_auth.auth=true;
	}
}

static void packet_switch_callback(uint8_t *packet, size_t len)
{
	if(packet[0]==HOST_COMM_BLE_MSG)
	{
		host_comm_ble_msg_handle(packet+1, len-1);
	}
	else if(packet[0]==HOST_COMM_UI_MSG)
	{
		host_comm_ui_msg_handle(packet+1, len-1);
	}
}

void app_ble_init(void)
{
	nina_b1_init();
	ble_request_cmd(HOST_CMD_SYNC_INFO);
	#ifdef JIGTEST
	host_ble_info_sync=false;
	#endif
}

extern __IO bool config_mode;
void app_ble_task(void)
{
	nina_b1_polling(packet_switch_callback);
#ifndef JIGTEST
	if(ble_auth.connected && !ble_auth.auth && !config_mode)
	{
		if(millis()-ble_auth.connect_tick>5000)
		{
			ble_request_cmd(HOST_CMD_DISCONNECT);
			ble_auth.connected=false;
			info("Reject timeout connection\n");
		}
	}
#endif
}

void app_ble_console_handle(char *result)
{
	if(__check_cmd("sync info"))
	{
		debug("Request ble mac\n");
		ble_request_cmd(HOST_CMD_SYNC_INFO);
	}
	else if(__check_cmd("disconnect"))
	{
		debug("Disconnect ble device\n");
		ble_request_cmd(HOST_CMD_DISCONNECT);
	}
	else if(__check_cmd("start dfu"))
	{
		uint8_t cmd[1]={HOST_CMD_DFU};
		nina_b1_ask(HOST_COMM_BLE_MSG, cmd, 1, HOST_BLE_OK, 100, 1);
		debug("Sent dfu request\n");
	}
	else if(__check_cmd("fast mode"))
	{
		uint8_t cmd[1]={HOST_CMD_FAST_MODE};
		nina_b1_ask(HOST_COMM_BLE_MSG, cmd, 1, HOST_BLE_OK, 100, 1);
		debug("request fast mode\n");
	}
	else if(__check_cmd("normal mode"))
	{
		uint8_t cmd[1]={HOST_CMD_NORMAL_MODE};
		nina_b1_ask(HOST_COMM_BLE_MSG, cmd, 1, HOST_BLE_OK, 100, 1);
		debug("Sent dfu request\n");
	}
	else debug("Unknown command\n");
}

static void ble_frame_cb(uint8_t *frame, size_t len)
{
	if (frame[0] == HOST_COMM_UI_MSG)
	{
		if (transport_handler != NULL)
		{
			transport_handler(frame + 1, len - 1);
		}
	}
}

static void transport_polling(frame_handler handler)
{
	transport_handler = handler;
	nina_b1_polling(ble_frame_cb);
}

static void transport_send(uint8_t *data, size_t len, slip_frame_type_t type)
{
	static uint8_t cmd=HOST_COMM_UI_MSG;
	if(type==SLIP_FRAME_BEGIN )
	{
		nina_b1_send3(&cmd, 1, SLIP_FRAME_BEGIN);
		nina_b1_send3(data, len, SLIP_FRAME_MIDDLE);
	}
	else if(type==SLIP_FRAME_COMPLETE)
	{
		nina_b1_send3(&cmd, 1, SLIP_FRAME_BEGIN);
		nina_b1_send3(data, len, SLIP_FRAME_END);
	}
	else
	{
		nina_b1_send3(data, len, type);
	}
}

static void transport_init()
{
	uint8_t cmd[2]={HOST_COMM_BLE_MSG, HOST_CMD_FAST_MODE};
	nina_b1_send0(cmd, 2);
	delay(200); //delay to let ble update connection params
}

const serial_interface_t ble_serial = {.mtu = 200, .init=transport_init, .polling = transport_polling, .send = transport_send};
