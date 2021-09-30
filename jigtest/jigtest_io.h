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

typedef struct __packed
{
	uint8_t front_light;
	uint8_t led_red;
	uint8_t led_green;
	uint8_t led_blue;
} host_io_value_t;

#ifdef __cplusplus
}
#endif
#endif
