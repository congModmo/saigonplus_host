/*
 * jigtest_lte.h
 *
 *  Created on: Oct 16, 2021
 *      Author: thanhcong
 */

#ifndef JIGTEST_LTE_H_
#define JIGTEST_LTE_H_
#include "jigtest.h"

void jigtest_lte_cert_handle(uint8_t type, uint8_t idx, uint8_t *data, uint8_t data_len);
void jigtest_lte_import_key_handle(void);

void jigtest_lte_hardware_init(task_complete_cb_t cb, void *params);
void jigtest_lte_hardware_process();

void jigtest_lte_function_init(task_complete_cb_t cb, void *params);
void jigtest_lte_function_process();
#endif /* JIGTEST_LTE_H_ */
