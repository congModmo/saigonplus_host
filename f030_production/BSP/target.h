/*
	Target specific include header file

	HieuNT
	20/3/2017
*/

#ifndef __TARGET_SPEC_H_
#define __TARGET_SPEC_H_

#ifdef __cplusplus
extern "C" {
#endif

// types
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>


#include "stm32f0xx_hal.h"

 //#define TARGET_ENV_RTOS

#if defined (TARGET_ENV_RTOS)
#include "cmsis_os.h"
#define delay(x) osDelay(x)
// #define _malloc(x) pvPortMalloc(x)
// #define _free(x) vPortFree(x)
#endif

// HieuNT: add for convenient
#define _sizeof(x) (sizeof(x)-1)
#define _malloc(x) malloc(x)
#define _free(x) free(x)
#define millis() HAL_GetTick()


uint32_t micros(void);
void delay_us(uint32_t us);
#define delayMicroseconds(us) delay_us(us)

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
