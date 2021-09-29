/*
 * bsp.c
 *
 *  Created on: Mar 18, 2021
 *      Author: thanhcong
 */

#include "bsp.h"
#include "ext_flash/GD25Q16.h"
#include "spi.h"
#include "ringbuf/ringbuf.h"

#define UART_READ_BLOCK 20
/*****************************************************************************
 * UART FOTA BSP
 ****************************************************************************/

RINGBUF uartRingbuf;
uint8_t uart_ringbuf_buf[32];
uint8_t rx_char;


/*****************************************************************************
 * BSP FLASH
 ****************************************************************************/

void flash_chip_select(uint8_t state)
{
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, (state==0)?GPIO_PIN_RESET:GPIO_PIN_SET);
}

void flash_rw_bytes(uint8_t *txData, uint16_t txLen, uint8_t *rxData, uint16_t rxLen)
{
	HAL_SPI_Transmit(&hspi1, txData, txLen, 3000);
	HAL_SPI_Receive(&hspi1, rxData, rxLen, 3000);
}

