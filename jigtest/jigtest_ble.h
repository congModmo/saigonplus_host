/*
 * jigtest_ble.h
 *
 *  Created on: Oct 16, 2021
 *      Author: thanhcong
 */

#ifndef JIGTEST_BLE_H_
#define JIGTEST_BLE_H_

#include "jigtest.h"

typedef struct{
	bool reset;
	bool tx_rx;
}ble_hardware_test_result_t;

typedef struct{
	bool mac;
	bool connection;
	bool transceiver;
}ble_function_test_result_t;

ble_hardware_test_result_t *jigtest_ble_hardware_test();
ble_function_test_result_t *jigtest_ble_function_test();

void jigtest_ble_hardware_test_init(task_complete_cb_t cb, void *params);
void jigtest_ble_hardware_test_process();

void jigtest_ble_function_test_init(task_complete_cb_t cb, void *params);
void jigtest_ble_function_test_process();
#endif /* JIGTEST_BLE_H_ */
