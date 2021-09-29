/*
 * udt.h
 *
 *  Created on: Apr 21, 2021
 *      Author: thanhcong
 */

#ifndef UDT_UDT_H_
#define UDT_UDT_H_
#include "bsp.h"

#define UART_READ_BLOCK 32
#define RINGBUF_BUF_LEN 256
typedef enum{
	UART_TIMEOUT_IT,
	UART_TIMEOUT_DMA
}uart_timeout_mode_t;
typedef struct{
	UART_HandleTypeDef *huart;
	uart_timeout_mode_t mode;
	RINGBUF *rb;
	uint8_t *rbBuff;
	size_t buff_len;
	uint8_t blockBuff[UART_READ_BLOCK];
}uart_timeout_t;
void uart_timeout_init(uart_timeout_t *udt, uart_timeout_mode_t mode, UART_HandleTypeDef *huart, uint8_t *buff, size_t buff_len);
void uart_timeout_start(uart_timeout_t *udt, RINGBUF *rb);
void uart_timeout_rx_cb(uart_timeout_t *udt);
void uart_timeout_err_cb(uart_timeout_t *udt);

#endif /* UDT_UDT_H_ */
