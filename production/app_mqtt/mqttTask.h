/*
 * mqttTask.h
 *
 *  Created on: May 19, 2021
 *      Author: thanhcong
 */

#ifndef APP_LTE_MQTTTASK_H_
#define APP_LTE_MQTTTASK_H_

bool mqtt_is_ready();

#ifdef JIGTEST
extern __IO bool mqtt_test_done;
extern __IO bool mqtt_test_result;
#endif

#endif /* APP_LTE_MQTTTASK_H_ */
