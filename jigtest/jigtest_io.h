#ifndef __TESTKIT_IO_H_
#define __TESTKIT_IO_H_
#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
	bool light_on;
	bool completed;
	int led_front_count;
	int led_red_count;
	int led_green_count;
	int led_blue_count;
}io_test_t;

typedef struct{
	bool red;
	bool green;
	bool blue;
	bool head;
	bool lock;
}jigtest_io_result_t;

void jigtest_test_io();

#ifdef __cplusplus
}
#endif
#endif
