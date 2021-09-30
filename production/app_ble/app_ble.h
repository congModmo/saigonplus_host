#ifndef __wcb_ble_h__
#define __wcb_ble_h__


#ifdef __cplusplus
extern "C" {
#endif
#include "bsp.h"
#include "host_comm/host_comm.h"
#include "app_fota/serial_transport.h"

extern const serial_interface_t ble_serial;

 typedef struct{
 	int red;
 	int green;
 	int blue;
 }side_light_config_t;

 typedef struct{
 	bool beep_sound;
 	int head_light;
 	side_light_config_t side_light;
 	int imu_sensitivity;
 }user_config_t;

#define REQ_GET_IMEI_MAC 	"GET_IMEI_MAC"
#define REQ_SYS_INFO  		"SYS_INFO"
#define REQ_SYS_RESET  		"SYS_RESET"
#define REQ_CONF_HL 		"CFG_HL"
#define REQ_CONF_SL  		"CFG_SL"
#define REQ_CONF_BEEP 		"CFG_BS"
#define REQ_CONF_MSS 		"CFG_MSS"
#define REQ_CONF_GET  		"GET_CFG"

void wcb_ble_init(void);
void wcb_ble_task(void);
void wcb_ble_comm_req_set_string(uint8_t cmd, const uint8_t *s, uint8_t len);
void wcb_ble_comm_req_set_hw_info(void);
bool wcb_ble_update_time(char *s);
void wcb_ble_send_notify(void);
void wcb_ble_comm_reset(void);

extern const char *const ble_mac;

#ifdef __cplusplus
}
#endif

#endif
