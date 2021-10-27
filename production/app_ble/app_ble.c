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

static frame_handler transport_handler = NULL;
#define APP_BLE_CHECK_CMD(cmd2) (strncmp(cmd, cmd2, strlen(cmd2)) == 0)
static char mac[32];
const char * const ble_mac=mac;

static char imei_mac[64] ={0};
static char ble_resp[256]={0};

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
		app_serial_fota_request(FOTA_OVER_BLE, request);
	}
	else if (packet[0] == FOTA_REQUEST_MTU)
	{
//		debug("Send mtu: %d\n", ble_serial.mtu);
		uint8_t frame[2] = {DEVICE_SET_MTU, ble_serial.mtu};
		nina_b1_send1(HOST_COMM_UI_MSG, frame, 2);
	}
}

static bool imei_mac_valid()
{
	if(strlen(imei_mac)>0)
		return true;
	//create imei_mac
	if(strlen(mac)==0)
	{
		ble_request_cmd(HOST_CMD_SYNC_INFO);
		return false;
	}
	if(strlen(lteImei) == 0)
		return false;
	sprintf(imei_mac, "%s_%s", lteImei, mac);
	return true;
}

static void app_ble_handle_ui_string_cmd(char * result)
{
	if (__check_cmd(REQ_GET_IMEI_MAC))
	{
		if(imei_mac_valid())
		{
			sprintf(ble_resp, "%s OK %s", REQ_GET_IMEI_MAC, imei_mac);
		}
		else
		{
			sprintf(ble_resp, "%s ERROR", REQ_GET_IMEI_MAC);
		}
		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)ble_resp, strlen(ble_resp));
		debug("rep: %s\n", ble_resp);
		return;
	}
	if(!imei_mac_valid())
	{
		error("Imei mac not available\n");
		return;
	}
	if(!__check_cmd(imei_mac))
	{
		error("Invalid cmd\n");
		return;
	}
	result+=strlen(imei_mac)+1;
	char *response=NULL;
	bool reset=false;
	if (__check_cmd(REQ_SYS_INFO))
	{
		//<IMEI> <CCID> <MAC> <HARDWARE_VERSION> <HOST_VERSION> <BLE_VERSION>
		sprintf(ble_resp, "%s OK %s %s %s %d %d %d", REQ_SYS_INFO, lteImei, lteCcid, mac, factory_config->hardwareVersion, firmware_version->hostApp, firmware_version->bleApp);
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
			sprintf(ble_resp, "%s OK", REQ_CONF_HL);
		}
		else
		{
			sprintf(ble_resp, "%s ERROR", REQ_CONF_HL);
		}
		response=ble_resp;
	}
	else if (__check_cmd(REQ_CONF_BEEP))
	{
		result += strlen(REQ_CONF_BEEP) + 1;
		int beep;
		if (sscanf(result, "%d", &beep) == 1)
		{
			sprintf(ble_resp, "%s OK", REQ_CONF_BEEP);
			app_info_update_beep_sound(beep);
		}
		else
		{
			sprintf(ble_resp, "%s ERROR", REQ_CONF_BEEP);
		}
		response=ble_resp;
	}
	else if (__check_cmd(REQ_CONF_SL))
	{
		result += strlen(REQ_CONF_SL) + 1;
		int red, green, blue;
		if (sscanf(result, "%d %d %d", &red, &green, &blue) == 3)
		{
			sprintf(ble_resp, "%s OK", REQ_CONF_SL);
			app_info_update_sidelight(red, green, blue);
			light_control_restart();
		}
		else
		{
			sprintf(ble_resp, "%s ERROR", REQ_CONF_SL);
		}
		response=ble_resp;
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
			sprintf(ble_resp, "%s OK", REQ_CONF_SN);
		}
		else
		{
			sprintf(ble_resp, "%s ERRO", REQ_CONF_SN);
		}
		response=ble_resp;
	}
	if(response!=NULL)
	{
		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)response, strlen(response));
		ioctl_beepbeep(1, 500);
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
	}
	else if(msg[0]==BLE_CMD_SYNC_INFO)
	{
		nina_b1_send1(HOST_BLE_RES_INFO, (uint8_t *)host_ble_info, sizeof(host_ble_info_t));
		debug("response to ble sync cmd\n");
	}
	else if(msg[0]==BLE_STATE_DISCONNECTED)
	{
		debug("Ble disconnected\n");
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
	}
}

static void testing_callback(uint8_t *packet, size_t len)
{
	if(packet[0]==HOST_COMM_BLE_MSG)
	{
		debug("Host ble message\n");
		host_comm_ble_msg_handle(packet+1, len-1);
	}
	else if(packet[0]==HOST_COMM_UI_MSG)
	{
		host_comm_ui_msg_handle(packet+1, len-1);
//		nina_b1_send1(HOST_COMM_UI_MSG, (uint8_t *)packet+1, 10);
	}
}

void app_ble_init(void)
{
	nina_b1_init();
	ble_request_cmd(HOST_CMD_SYNC_INFO);
}

void app_ble_task(void)
{
	nina_b1_polling(testing_callback);
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
