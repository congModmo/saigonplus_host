/*
 * test_led_group_runner.c
 *
 *  Created on: Mar 14, 2022
 *      Author: admin
 */

#include "unity_fixture.h"

TEST_GROUP_RUNNER(LedDriver)
{
	RUN_TEST_CASE(LedDriver, LedOffAfterCreate)
}
