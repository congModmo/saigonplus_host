/*
 * bsp.h
 *
 *  Created on: Mar 18, 2021
 *      Author: thanhcong
 */

#ifndef BSP_H_
#define BSP_H_

#include "stm32f0xx_hal.h"
#include "usart.h"
#include "ext_flash/GD25Q16.h"
#include "ringbuf/ringbuf.h"
/*
 * System bsp
 */

#define millis() HAL_GetTick()
#define delay(x) HAL_Delay(x)

extern RINGBUF uartRingbuf;
void bsp_uart_fota_init();
#ifdef DEBUG
#define WDT_FEED()
#else
#include "iwdg.h"
#define WDT_FEED() HAL_IWDG_Refresh(&hiwdg)
#endif


#endif /* BSP_H_ */
