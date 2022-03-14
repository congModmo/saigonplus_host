/*
 * main.c
 *
 *  Created on: Mar 14, 2022
 *      Author: admin
 */

#include <stdio.h>
#include "system.h"

#ifdef UNIT_TEST
#include "unit_test.h"
#include "unity_fixture.h"
#endif

int main(int argc, const char* argv[])
{
#if defined(UNIT_TEST)
	#if defined(UNIT_TEST_NATIVE)
    	return UnityMain(argc, argv, RunAllTests);
	#elif defined(UNIT_TEST_TARGET)
    	system_init();
    	return UnityMain(0, NULL, RunAllTests);
	#else
	#error "unit test error!!!"
	#endif
#else
    system_init();
    system_main();
#endif
}
