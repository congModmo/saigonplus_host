/*
 * bsp.h
 *
 *  Created on: Mar 18, 2021
 *      Author: thanhcong
 */

#ifndef BSP_H_
#define BSP_H_

#include "stm32f0xx_hal.h"
#include "cmsis_os.h"
#include "usart.h"
#include "ringbuf/ringbuf.h"
#include "log_sys.h"
#include "ext_flash/GD25Q16.h"
#include "ringbuf/ringbuf.h"
#include "kxtj3/kxtj3.h"
#include "fota_comm.h"
#include "log_sys.h"

#define ASSERT_RET(cond, errRet, ...) \
    if (!(cond))                      \
    {                                 \
        error(__VA_ARGS__);   \
        debugx("\n");          \
        return errRet;                \
    }
#define ASSERT_GO(cond, errRet, ...) \
    if (!(cond))                      \
    {                                 \
        error(__VA_ARGS__);   \
        debugx("\n");          \
        goto __exit;                \
    }


/*
 * System bsp
 */
extern uint8_t fwVersion[];
char *get_system_build_config(void);
#define millis() HAL_GetTick()
#define timeout_init(timeout) (*timeout) = millis()
bool timeout_check(uint32_t *from, uint32_t interval);
#define __check_cmd_at_pos(x,p) (strncmp(&result[p], x, strlen(x)) == 0)
#define __check_cmd(x) __check_cmd_at_pos(x,0)
#define __param_pos(x)	((char *)(&result[strlen(x)]))
void delay(uint32_t t);

void system_vector_remap();
extern osMutexId appResourceHandle;
#define RESOURCE_LOCK() osMutexWait(appResourceHandle, osWaitForever)

#define RESOURCE_UNLOCK() osMutexRelease(appResourceHandle)
#define EXT_FLASH_LOCK() osMutexWait(flashMutexHandle, osWaitForever)

#define EXT_FLASH_UNLOCK() osMutexRelease(flashMutexHandle)

#ifdef JIGTEST
#define JIGTEST_LOCK() osMutexWait(jigtestMutexHandle, osWaitForever)
#define JIGTEST_UNLOCK() osMutexRelease(jigtestMutexHandle)
#endif

#ifdef RELEASE
#define WATCHDOG_FEED() HAL_IWDG_Refresh(&hiwdg);
#else
#define WATCHDOG_FEED()
#endif

typedef struct{
	uint8_t *data;
	uint32_t len;
}heap_data_t;

/*
 * Buzzer bps
 */
extern osMutexId buzzerMutexHandle;
#define BUZZER_LOCK() osMutexWait(buzzerMutexHandle, osWaitForever)
#define BUZZER_UNLOCK() osMutexRelease(buzzerMutexHandle)
/*
 * GPS bsp
 * gps_buff_idx >0 mean there is a RMC message in buffer, clear that variable when message was handled. new message every 1s.
 *
 */
#define GPS_BUF_SIZE 128
#define UART_GPS huart4
void bsp_gps_uart_init();
/*
 * Debug bsp
 */
#define DEBUG_BUF_SIZE 128

#define bsp_uart_console_init(x) bsp_uart5_init(x)
/*
 * BLE bsp
 */
void bsp_ble_uart_init(RINGBUF *rb);
void ble_send_buffer(uint8_t *data, uint16_t len);
void bsp_ble_send_byte(uint8_t b);
/*
 * LTE bsp
 */
void uart_lte_bsp_init();
extern osMutexId lteRingbufMutexHandle;
#define LTE_RINGBUF_LOCK() osMutexWait(lteRingbufMutexHandle, osWaitForever)
#define LTE_RINGBUF_UNLOCK() osMutexRelease(lteRingbufMutexHandle)
/*
 * display bps
 */

#define display_bsp_init(x) bsp_uart1_init(x)
#define bsp_ui_comm_init(x) bsp_uart1_init(x)
#define bsp_ui_comm_send_byte bsp_uart1_send_byte

void bsp_ui_comm_init(RINGBUF *rb);
void bsp_display_uart_init(RINGBUF *rb);
void display_bsp_send_byte(uint8_t b);
void uart_ui_bsp_send_byte(uint8_t c);
void bsp_ui_comm_send_byte(uint8_t b);
void bsp_uart5_send_byte(uint8_t b);

#define KXTJ3_ADDR 0x0E

#endif /* BSP_H_ */
