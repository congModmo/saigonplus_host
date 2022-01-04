/*
 * lteTask.h
 *
 *  Created on: May 19, 2021
 *      Author: thanhcong
 */

#ifndef LTE_LTETASK_H_
#define LTE_LTETASK_H_

#include "transport_interface.h"
#include "lte/lara_r2_socket.h"

struct NetworkContext
{
    lara_r2_socket_t *socket;
};

typedef struct{
	char imei[32];
	char ccid[32];
	char carrier[32];
	uint8_t rssi;
	network_type_t type;
	bool hardware_init;
	bool key;
	bool ready;
}lte_info_t;

extern __IO const lte_info_t *const lte_info;

void lte_task(void);
void jigtest_lte_task(void);

#endif /* APP_LTE_LTETASK_H_ */
