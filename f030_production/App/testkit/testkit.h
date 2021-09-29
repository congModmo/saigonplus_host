/*
 * testkit.h
 *
 *  Created on: Jul 5, 2021
 *      Author: thanhcong
 */

#ifndef TESTKIT_TESTKIT_H_
#define TESTKIT_TESTKIT_H_
#include "bsp.h"
#include "uart_ui_comm.h"
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

#endif /* TESTKIT_TESTKIT_H_ */
