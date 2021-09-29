/*
 * lara_r2_socket.h
 *
 *  Created on: May 17, 2021
 *      Author: thanhcong
 */

#ifndef LARA_R2_SOCKET_H_
#define LARA_R2_SOCKET_H_

#include "lara_r2.h"
#include "ringbuf/ringbuf.h"

enum{
	SOCKET_NULL=0,
	SOCKET_TCP,
	SOCKET_UDP
};

enum{
	SOCKET_IDLE,
	SOCKET_CONNECTED,
};

#define SOCKET_BUF_LEN 512

typedef struct{
	__IO uint8_t type;
	__IO uint8_t state;
	__IO char *host;
	__IO uint16_t host_port;
	__IO bool secure;
	__IO bool connect_request;
	__IO bool close_request;
	__IO int port;
	RINGBUF rx_rb;
	__IO int tx_len;
	__IO int tx_result;
	__IO const uint8_t *tx_buf;
	uint8_t rx_buf[SOCKET_BUF_LEN];
}lara_r2_socket_t;

//APIs for user (such as mqtt)
/*
 * Init resource
 * type: TCP or UDP
 */
bool lara_r2_socket_init(lara_r2_socket_t *socket, uint8_t type);

/*
 * Release resoure
 */
void lara_r2_socket_delete(lara_r2_socket_t *socket);

/*
 * Connect to server
 */
bool lara_r2_socket_connect(lara_r2_socket_t *socket, char *host, uint16_t port, bool secure);

/*
 * Send
 */
int lara_r2_socket_send(lara_r2_socket_t *socket, const uint8_t *data, uint16_t len);

int32_t lara_r2_socket_read(lara_r2_socket_t *socket, uint8_t *data, size_t maxlen);
//to check if module is ready to connect to network
bool lara_r2_socket_check();
//APIs for server (lte manage thread)

void lara_r2_socket_process();
void lara_r2_socket_urc_handle(char *urc);
#endif /* LTE_LARA_R2_SOCKET_H_ */
