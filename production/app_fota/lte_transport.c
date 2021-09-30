
#define __DEBUG__ 4

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "bsp.h"
#include "lte/lara_r2.h"
#include "flash_fota.h"
#include "lte_transport.h"
#include "log_sys.h"
#include "fota_comm.h"
#include "app_main/app_info.h"
#include "app_main/app_main.h"
#include "app_lte/lteTask.h"
extern osMessageQueueId_t lteMailHandle;
extern osMessageQueueId_t mainMailHandle;


bool lte_transport_init(void)
{
    return true;
}

enum{
	TRANSPORT_IDLE,
	TRANSPORT_GET_FILE,
	TRANSPORT_SAVE_FILE,
};
typedef struct{
	uint8_t state;
	char *link;
	fota_file_info_t *file;
	transport_get_file_cb_t file_cb;
	transport_err_cb_t err;
	uint32_t tick;
	uint32_t timeout;
	uint8_t *buff;
	size_t len;
}lte_transport_t;

static lte_transport_t transport;

static bool transport_init(uint8_t *buff, size_t len, void *params, transport_err_cb_t cb){
	transport.state=TRANSPORT_IDLE;
	transport.buff=buff;
	transport.len=len;
	ASSERT_RET(network_is_ready(), false, "Network not ready");
	debug("fota init done\n");
	transport.link=(char *)params;
	transport.err=cb;
	return true;
}

static bool transport_get_file(fota_file_info_t *info, transport_get_file_cb_t cb){
	transport.file=info;
	transport.file_cb=cb;
	debug("get file: %s\n", transport.file->file_name);

	lte_get_file_t * get_file_info = (lte_get_file_t *)malloc(sizeof(lte_get_file_t));
	ASSERT_RET(get_file_info!=NULL, false, "Malloc");
	get_file_info->info=info;
	get_file_info->link=transport.link;
	get_file_info->sector_timeout=60000; //60s timeout for each sector
	transport.timeout=(info->len/4096 + (info->len % 4096)?1:0)*60000+10000; //prefer lte mail response than timeout
	mail_t mail={.type=MAIL_LTE_HTTP_GET_FILE, .data=(uint8_t *)get_file_info, .len=sizeof(lte_get_file_t)};
	if(osMessageQueuePut(lteMailHandle, &mail, 0, 10)!=osOK){
		error("Mail put failed");
		free(get_file_info);
	}
	transport.state=TRANSPORT_GET_FILE;
	transport.tick=millis();

	return true;
}

static void transport_process(void){
	if(transport.state==TRANSPORT_GET_FILE){
		if(millis()-transport.tick>transport.timeout){
			transport.state=TRANSPORT_IDLE;
			transport.file_cb(false);
			error("Get file timeout\n");
		}
		static mail_t mail;
		if(osOK==osMessageQueueGet(mainMailHandle, &mail, NULL, 0)){
			if(mail.type==MAIL_LTE_HTTP_GET_FILE){
				transport.file_cb(mail.data[0]);
				transport.state=TRANSPORT_IDLE;
				debug("Get file return status: %s\n", mail.data[0]?"success":"failure");
			}
			if(mail.data!=NULL){
				free(mail.data);
				mail.data=NULL;
			}
		}
	}
}

static void transport_fota_signal(bool status){
//	send_mqtt_data(MQTT_DATA_FIRMWARE_INFO, firmware_version, strlen(firmware_version)+1, MAIL_MQTT_V2_PUBLISH);
}

const fota_transport_t lte_transport ={.source=FOTA_OVER_LTE,\
										.init=transport_init,\
										.get_file=transport_get_file,\
										.process=transport_process,\
										.signal=transport_fota_signal};
