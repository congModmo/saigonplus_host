/*
 * app_rtc.h
 *
 *  Created on: Aug 9, 2021
 *      Author: thanhcong
 */

#ifndef APP_MAIN_APP_RTC_H_
#define APP_MAIN_APP_RTC_H_

#include <stdint.h>

void set_rtc_from_at_string(char *str);
uint32_t rtc_to_timestamp();
void rtc_print_date_time();

#endif /* APP_MAIN_APP_RTC_H_ */
