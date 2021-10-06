#ifndef __log_sys_h__
#define __log_sys_h__

#ifdef __cplusplus9
extern "C" {
#endif

#include "bsp.h"
#include <stdio.h>
#include <string.h>
	 
#define error(fmt, ...)  do {} while (0)
#define warning(fmt, ...)  do {} while (0)
#define info(fmt, ...)  do {} while (0)
#define debug(fmt, ...)  do {} while (0)
#define debugx(fmt, ...)  do {} while (0)
#define DEBUG_LOCK() do {} while (0)
#define DEBUG_UNLOCK() do {} while (0)

#ifndef RELEASE

#ifdef USE_CMSIS_OS
	extern osMutexId debugMutexHandle;
	#undef DEBUG_LOCK
	#undef DEBUG_UNLOCK
	#define DEBUG_LOCK() osMutexWait(debugMutexHandle, osWaitForever)
	#define DEBUG_UNLOCK() osMutexRelease(debugMutexHandle)
#endif

#define _printf(...)\
		do{DEBUG_LOCK();\
			printf(__VA_ARGS__);\
			DEBUG_UNLOCK();}while(0)

#define _printf_format(header, fmt, ...) _printf("[%s] %s - Line: %d: " fmt, header, __FILENAME__, __LINE__,## __VA_ARGS__)
#if __DEBUG__>0
#undef error
#define error(fmt, ...) _printf_format("ERROR", fmt, ## __VA_ARGS__)
#endif

#if __DEBUG__>1
#undef warning
#define warning(fmt, ...) _printf_format("WARNG", fmt, ## __VA_ARGS__)
#endif

#if __DEBUG__>2
#undef info
#define info(fmt, ...) _printf_format("INFO", fmt, ## __VA_ARGS__)
#endif

#if __DEBUG__>3
#undef debug
#undef debugx
#define debug(fmt, ...) _printf_format("DEBUG", fmt, ## __VA_ARGS__)
#define debugx(...) _printf(__VA_ARGS__)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
