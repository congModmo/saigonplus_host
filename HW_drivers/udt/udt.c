/*
 * udt.c
 *
 *  Created on: Apr 21, 2021
 *      Author: thanhcong
 */

/***********************************************************************
 * uart timeout
 **********************************************************************/
#include "udt.h"

void uart_timeout_init(uart_timeout_t *udt,uart_timeout_mode_t mode, UART_HandleTypeDef *huart,  uint8_t *buff, size_t buff_len){
	udt->mode=mode;
	udt->rb=NULL;
	udt->huart=huart;
	udt->rbBuff=buff;
	udt->buff_len=buff_len;
	HAL_UART_ReceiverTimeout_Config(huart, 80);
	HAL_UART_EnableReceiverTimeout(huart);
	__HAL_UART_ENABLE_IT(huart, UART_IT_RTO);
}

void uart_timeout_start(uart_timeout_t *udt, RINGBUF *rb){
	udt->rb=rb;
	RINGBUF_Init(udt->rb, udt->rbBuff, udt->buff_len);
	if(udt->mode==UART_TIMEOUT_IT){
		HAL_UART_Abort_IT(udt->huart);
		HAL_UART_Receive_IT(udt->huart, udt->blockBuff, UART_READ_BLOCK);
	}
	else{
		HAL_UART_DMAStop(udt->huart);
		HAL_UART_Receive_DMA(udt->huart, udt->blockBuff, UART_READ_BLOCK);
	}
}

void uart_timeout_rx_cb(uart_timeout_t *udt){
	if(udt->rb!=NULL){
		static int i;
		for(i=0; i<UART_READ_BLOCK; i++){
			RINGBUF_Put(udt->rb, udt->blockBuff[i]);
		}
		if(udt->mode==UART_TIMEOUT_IT){
			HAL_UART_Receive_IT(udt->huart, udt->blockBuff, UART_READ_BLOCK);
		}
		else{
			HAL_UART_Receive_DMA(udt->huart, udt->blockBuff, UART_READ_BLOCK);
		}
	}
}

void uart_timeout_err_cb(uart_timeout_t *udt){
	if(udt->rb!=NULL){
		if(udt->mode==UART_TIMEOUT_IT && udt->huart->RxXferCount>0){
			static size_t size, i;
			size=   udt->huart->RxXferSize -udt->huart->RxXferCount;
			for(i=0; i<size; i++){
				RINGBUF_Put(udt->rb, udt->blockBuff[i]);
			}
		}
		else{
			if(__HAL_DMA_GET_COUNTER(udt->huart->hdmarx) != UART_READ_BLOCK && udt->huart->ErrorCode & HAL_UART_ERROR_RTO)
			{
				static size_t size, i;
				size= UART_READ_BLOCK - __HAL_DMA_GET_COUNTER(udt->huart->hdmarx);
				for(i=0; i<size; i++){
					RINGBUF_Put(udt->rb, udt->blockBuff[i]);
				}
			}
		}
		if(udt->mode==UART_TIMEOUT_IT){
			HAL_UART_Receive_IT(udt->huart, udt->blockBuff, UART_READ_BLOCK);
		}
		else{
			HAL_UART_Receive_DMA(udt->huart, udt->blockBuff, UART_READ_BLOCK);
		}
	}
}
