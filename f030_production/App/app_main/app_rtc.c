/*
 * app_rtc.c
 *
 *  Created on: Aug 9, 2021
 *      Author: thanhcong
 */

#define __DEBUG__ 4

#include "log_sys.h"
#include "app_common/common.h"
#include <time.h>
#include "rtc.h"

void set_rtc_from_at_string(char *str)
{
	 int year, month, day, hour, min, sec, tz;
	 char sign;
	 ASSERT_RET(sscanf(str, "+CCLK: \"%d/%d/%d,%d:%d:%d%c%d\"", &year, &month, &day, &hour, &min, &sec, &sign, &tz)==8, false, "sscanf");
	 if(sign=='+'){
		 hour=hour-(tz/4);
	 }
	 else{
		 hour=hour+(tz/4);
	 }
	 RTC_TimeTypeDef time={.Hours=hour, .Minutes=min, .Seconds=sec};
	 RTC_DateTypeDef date={.Date=day, .Month=month , .Year=year};
	 ASSERT_RET(HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN)==HAL_OK, false, "Set time");
	 ASSERT_RET(HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN)==HAL_OK, false, "Set_date");
	 rtc_print_date_time();
}

//	struct tm {
//	   int tm_sec;         /* seconds,  range 0 to 59          */
//	   int tm_min;         /* minutes, range 0 to 59           */
//	   int tm_hour;        /* hours, range 0 to 23             */
//	   int tm_mday;        /* day of the month, range 1 to 31  */
//	   int tm_mon;         /* month, range 0 to 11             */
//	   int tm_year;        /* The number of years since 1900   */
//	   int tm_wday;        /* day of the week, range 0 to 6    */
//	   int tm_yday;        /* day in the year, range 0 to 365  */
//	   int tm_isdst;       /* daylight saving time             */
//	};

uint32_t rtc_to_timestamp()
{
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	struct tm _tm;
	_tm.tm_sec=time.Seconds;
	_tm.tm_min=time.Minutes;
	_tm.tm_hour=time.Hours;
	_tm.tm_mday=date.Date;
	_tm.tm_mon= date.Month-1;
	_tm.tm_year=date.Year+100;
	return (uint32_t)mktime(&_tm);
}

void rtc_print_date_time()
{
	 RTC_TimeTypeDef time;
	 RTC_DateTypeDef date;
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	debug("Set time date utc to: %d/%d/%d %d:%d:%d\n", date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
	debug("Timestamp: %u\n", rtc_to_timestamp());
}
