/*
 * jigtest_ble.h
 *
 *  Created on: Oct 16, 2021
 *      Author: thanhcong
 */

#ifndef JIGTEST_BLE_H_
#define JIGTEST_BLE_H_

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

#endif /* JIGTEST_BLE_H_ */
