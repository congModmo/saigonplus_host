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

typedef enum{
	NETWORK_TYPE_NONE,
	NETWORK_TYPE_2G,
	NETWORK_TYPE_4G
}network_type_t;

extern const network_type_t * const network_type;
extern const char *const lteImei;
extern const int *const lteRssi;
extern const char *const lteCcid;
extern const char *const lteCarrier;
#endif /* APP_LTE_LTETASK_H_ */
