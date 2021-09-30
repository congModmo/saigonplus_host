/*
 * bsp.c
 *
 *  Created on: Mar 18, 2021
 *      Author: thanhcong
 */

#include <gps/eva_m8.h>
#include "bsp.h"
#include "ext_flash/GD25Q16.h"
#include "spi.h"
#include "app_config.h"
#include "ble/nina_b1.h"
#include "stm32f0xx_hal_rcc.h"
#include "udt/udt.h"
#include "kxtj3/kxtj3.h"
#include "i2c.h"
#include "fota_comm.h"
#include "iwdg.h"
#include "lte/lara_r2.h"
/*****************************************************************************
 * interrupt vector remap
 ****************************************************************************/
#if (defined(__CC_ARM))
__IO uint32_t VectorTable[48] __attribute__((at(0x20000000)));
#elif (defined(__ICCARM__))
#pragma location = 0x20000000
__no_init __IO uint32_t VectorTable[48];
#elif defined(__GNUC__)
__IO uint32_t VectorTable[48] __attribute__((section(".RAMVectorTable")));
#elif defined(__TASKING__)
__IO uint32_t VectorTable[48] __at(0x20000000);
#endif

#define RCC_APB2Periph_SYSCFG RCC_APB2ENR_SYSCFGEN
#define IS_RCC_APB2_PERIPH(PERIPH) ((((PERIPH)&0xFFB8A5FE) == 0x00) && ((PERIPH) != 0x00))
#define SYSCFG_MemoryRemap_Flash ((uint8_t)0x00)
#define SYSCFG_MemoryRemap_SystemMemory ((uint8_t)0x01)
#define SYSCFG_MemoryRemap_SRAM ((uint8_t)0x03)

#define IS_SYSCFG_MEMORY_REMAP(REMAP) (((REMAP) == SYSCFG_MemoryRemap_Flash) ||        \
									   ((REMAP) == SYSCFG_MemoryRemap_SystemMemory) || \
									   ((REMAP) == SYSCFG_MemoryRemap_SRAM))

void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
	/* Check the parameters */
	assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
	assert_param(IS_FUNCTIONAL_STATE(NewState));
	if (NewState != DISABLE)
	{
		RCC->APB2RSTR |= RCC_APB2Periph;
	}
	else
	{
		RCC->APB2RSTR &= ~RCC_APB2Periph;
	}
}

void SYSCFG_MemoryRemapConfig(uint32_t SYSCFG_MemoryRemap)
{
	uint32_t tmpctrl = 0;
	/* Check the parameter */
	assert_param(IS_SYSCFG_MEMORY_REMAP(SYSCFG_MemoryRemap));
	/* Get CFGR1 register value */
	tmpctrl = SYSCFG->CFGR1;
	/* Clear MEM_MODE bits */
	tmpctrl &= (uint32_t)(~SYSCFG_CFGR1_MEM_MODE);
	/* Set the new MEM_MODE bits value */
	tmpctrl |= (uint32_t)SYSCFG_MemoryRemap;
	/* Set CFGR1 register with the new memory remap configuration */
	SYSCFG->CFGR1 = tmpctrl;
}

void system_vector_remap()
{
	for (int i = 0; i < 48; i++)
	{
		VectorTable[i] = *(__IO uint32_t *)(APPLICATION_START_ADDR + (i << 2));
	}
	/* Enable the SYSCFG peripheral clock*/
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	/* Remap SRAM at 0x00000000 */
	SYSCFG_MemoryRemapConfig(SYSCFG_MemoryRemap_SRAM);
}

void delay(uint32_t t)
{
	while (t > 0)
	{
		if (t > 10)
		{
			osDelay(10);
			t -= 10;
		}
		else
		{
			osDelay(t);
			t = 0;
		}
		WATCHDOG_FEED();
	}
}

/****************************************************************************
 * UART UI/DISPLAY
 ***************************************************************************/

#ifdef DISPLAY_ENABLE

#define DISPLAY_BUFF_LEN 128

static uart_timeout_t display_udt;
static uint8_t displayRbBuff[DISPLAY_BUFF_LEN];

void display_bsp_init(RINGBUF *rb)
{
	uart_timeout_init(&display_udt, UART_TIMEOUT_IT, &huart1, displayRbBuff, DISPLAY_BUFF_LEN);
	uart_timeout_start(&display_udt, rb);
}

void display_bsp_send_byte(uint8_t b)
{
	HAL_UART_Transmit(&huart1, &b, 1, 100);
}
#else

static uart_timeout_t ui_comm_udt;
static uint8_t ui_comm_buff[512];
void bsp_ui_comm_init(RINGBUF *rb)
{
	uart_timeout_init(&ui_comm_udt, UART_TIMEOUT_IT, &huart1, ui_comm_buff, 512);
	uart_timeout_start(&ui_comm_udt, rb);
}
void bsp_ui_comm_send_byte(uint8_t b)
{
	HAL_UART_Transmit(&huart1, &b, 1, 100);
}
#endif

/****************************************************************************
 * BSP DEBUG
 ***************************************************************************/
#ifndef RELEASE
RINGBUF debugRingbuf;
char debug_buffer[128];
static uint8_t debug_char[1];

void bsp_debug_uart_init()
{
	RINGBUF_Init(&debugRingbuf, (uint8_t *)debug_buffer, 128);
	HAL_UART_Receive_IT(&huart5, debug_char, 1);
}
#endif
/****************************************************************************
 * BSP BLE
 ***************************************************************************/
#ifdef BLE_ENABLE
#define BLE_BUFF_LEN 512
static uart_timeout_t ble_udt;
static uint8_t bleRbBuff[BLE_BUFF_LEN];

void nina_b1_bsp_send_buffer(uint8_t *data, uint16_t len)
{
	HAL_UART_Transmit(&huart2, data, len, 1000);
}

void nina_b1_bsp_send_byte(uint8_t b)
{
	HAL_UART_Transmit(&huart2, &b, 1, 100);
}

void nina_b1_bsp_init(RINGBUF *rb)
{
	uart_timeout_init(&ble_udt, UART_TIMEOUT_DMA, &huart2, bleRbBuff, BLE_BUFF_LEN);
	uart_timeout_start(&ble_udt, rb);
}

void nina_b1_bsp_set_reset_pin(uint8_t value)
{
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, value);
}
#endif

/*****************************************************************************
 * BSP FLASH
 ****************************************************************************/

void flash_chip_select(uint8_t state)
{
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, (state == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void flash_rw_bytes(uint8_t *txData, uint16_t txLen, uint8_t *rxData, uint16_t rxLen)
{
	if (txLen > 0 && txData != NULL)
	{
		HAL_SPI_Transmit(&hspi1, txData, txLen, 3000);
	}
	if (rxLen > 0 && rxData != NULL)
	{
		HAL_SPI_Receive(&hspi1, rxData, rxLen, 3000);
	}
}

/***************************************************************************
 * GPS_BSP
 ***************************************************************************/
#ifdef GPS_ENABLE
static uint8_t gps_ringbuf_buffer[GPS_BUF_SIZE];
static uint8_t gps_buff[UART_READ_BLOCK];
static RINGBUF *gpsRingbuf;

void bsp_gps_uart_init(RINGBUF *rb)
{
	gpsRingbuf = rb;
	RINGBUF_Init(gpsRingbuf, (uint8_t *)gps_ringbuf_buffer, GPS_BUF_SIZE);
	HAL_UART_Receive_DMA(&huart4, gps_buff, UART_READ_BLOCK);
}
void eva_m8_reset(void)
{
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_RESET);
	delay(500);
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_SET);
}

void eva_m8_send_data(uint8_t *data, uint8_t len)
{
	HAL_UART_Transmit(&UART_GPS, data, len, 100);
}
#endif
/***********************************************************************
 * LTE bsp
 **********************************************************************/
#ifdef LTE_ENABLE
static uint8_t lte_buff[LARA_R2_BUFF_SIZE];
static uint8_t lte_rb_buff[LARA_R2_BUFF_SIZE * 2];
static RINGBUF *lte_rb = NULL;
void uart_lte_bsp_init(RINGBUF *rb)
{
	lte_rb = rb;
	RINGBUF_Init(lte_rb, lte_rb_buff, LARA_R2_BUFF_SIZE * 2);
	HAL_UART_ReceiverTimeout_Config(&huart3, 40);
	HAL_UART_EnableReceiverTimeout(&huart3);
	__HAL_UART_ENABLE_IT(&huart3, UART_IT_RTO);
	HAL_UART_Receive_DMA(&huart3, lte_buff, LARA_R2_BUFF_SIZE);
}

static void lte_input_handle(uint8_t *buff, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++)
	{
		RINGBUF_Put(lte_rb, buff[i]);
	}
}
void gsm_send_string(char *str)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)str, strlen(str), 1000);
}
#endif

/***********************************************************************
 * Uart callbacks
 **********************************************************************/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart3)
	{
#ifdef LTE_ENABLE
		lte_input_handle(lte_buff, LARA_R2_BUFF_SIZE);
		HAL_UART_Receive_DMA(&huart3, lte_buff, LARA_R2_BUFF_SIZE);
#endif
	}
	else if (huart == &huart1)
	{
#ifdef DISPLAY_ENABLE
		uart_timeout_rx_cb(&display_udt);
#else
		uart_timeout_rx_cb(&ui_comm_udt);
#endif
	}
	else if (huart == &huart4)
	{
#ifdef GPS_ENABLE
		static uint8_t i;
		for (i = 0; i < UART_READ_BLOCK; i++)
		{
			RINGBUF_Put(gpsRingbuf, gps_buff[i]);
		}
		HAL_UART_Receive_DMA(&huart4, gps_buff, UART_READ_BLOCK);
#endif
	}
	if (huart == &huart2)
	{
#ifdef BLE_ENABLE
		uart_timeout_rx_cb(&ble_udt);
#endif
	}
	else if (huart == &huart5)
	{
#ifndef RELEASE
		RINGBUF_Put(&debugRingbuf, debug_char[0]);
		HAL_UART_Receive_IT(huart, debug_char, 1);
#endif
	}
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart3)
	{
#ifdef LTE_ENABLE
		if (__HAL_DMA_GET_COUNTER(huart->hdmarx) != LARA_R2_BUFF_SIZE && huart->ErrorCode & HAL_UART_ERROR_RTO)
		{
			uint32_t len = LARA_R2_BUFF_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
			lte_input_handle(lte_buff, len);
		}

		HAL_UART_Receive_DMA(&huart3, lte_buff, LARA_R2_BUFF_SIZE);
#endif
	}
	else if (huart == &huart1)
	{
#ifdef DISPLAY_ENABLE
		uart_timeout_err_cb(&display_udt);
#else
		uart_timeout_err_cb(&ui_comm_udt);
#endif
	}
	else if (huart == &huart4)
	{
#ifdef GPS_ENABLE
		HAL_UART_Receive_DMA(huart, gps_buff, UART_READ_BLOCK);
#endif
	}
	else if (huart == &huart2)
	{
#ifdef BLE_ENABLE
		uart_timeout_err_cb(&ble_udt);
#endif
	}
	else if (huart == &huart5)
	{
#ifndef RELEASE
		HAL_UART_Receive_IT(huart, debug_char, 1);
#endif
	}
}

/***************************************************************************
 * IMU BSP
 ***************************************************************************/
bool bsp_kxtj3_tx_rx(uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
	if (tx != NULL && tx_len > 0)
	{
		if (HAL_I2C_Master_Transmit(&hi2c1, KXTJ3_ADDR << 1, tx, tx_len, 1000) != HAL_OK)
		{
			return false;
		}
	}
	if (rx != NULL && rx_len > 0)
	{
		if (HAL_I2C_Master_Receive(&hi2c1, KXTJ3_ADDR << 1, rx, rx_len, 1000) != HAL_OK)
		{
			return false;
		}
	}
	return true;
}
