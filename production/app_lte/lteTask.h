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

typedef enum{
	CELLULAR_MODE_4G_2G,
	CELLULAR_MODE_2G,
	CELLULAR_MODE_4G
}cellular_mode_t;

struct NetworkContext
{
    lara_r2_socket_t *socket;
};

extern const network_type_t * const network_type;
extern const char *const lteImei;
extern const int *const lteRssi;
extern const char *const lteCcid;
extern const char *const lteCarrier;
#endif /* APP_LTE_LTETASK_H_ */
