/*
 * testkit.h
 *
 *  Created on: Jul 5, 2021
 *      Author: thanhcong
 */

#ifndef TESTKIT_TESTKIT_H_
#define TESTKIT_TESTKIT_H_
#include "bsp.h"
#include "app_main/uart_ui_comm.h"

enum{
	JIGTEST_IDLE=0,
	JIGTEST_TESTING_HARDWARE,
	JIGTEST_TESTING_FUNCTION,
};

typedef struct{
	uint32_t timeout;
	uint32_t tick;
}jigtest_timer_t;

typedef void (* task_complete_cb_t)(void);
typedef void (* task_init_t)(task_complete_cb_t cb, void *params);
typedef void (* task_process_t)(void);

void testkit_report(uint8_t type, uint8_t *data, uint8_t len);
void testkit_console_handle(char *result);
void testkit_test_ble();

void testkit_ble_hardware_test();
void testkit_ble_function_test();

void testkit_lte_hardware_test();
void testkit_lte_function_test();

void testkit_gps_hardware_test();
void testkit_gps_function_test();

void testkit_test_io();

void jigtest_direct_report(uint8_t type, uint8_t status);
void jigtest_report(uint8_t type, uint8_t *data, uint8_t len);
bool jigtest_mail_direct_command(uint8_t command, osMessageQueueId_t mailBox);

#endif /* TESTKIT_TESTKIT_H_ */
