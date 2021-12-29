#define __DEBUG__ 4
#include "jigtest.h"
#include "app_main/uart_ui_comm.h"
#include "jigtest_esp.h"
#include "bsp.h"
#include "esp_host_comm.h"
#include "bsp.h"
#include "protothread/pt.h"

typedef struct
{
	RINGBUF rb;
	uint8_t outBuffer[32];
	uint8_t inBuffer[128];
	slip_t slip;
	uint32_t crc;
	bool completed;
} esp_tester_t;

static task_complete_cb_t callback;
static esp_tester_t esp_tester;
static struct pt esp_pt, block_pt;

void esp_uart_polling(frame_handler handler)
{
	while (RINGBUF_Available(&esp_tester.rb))
	{
		static uint8_t c;
		static uint16_t len;
		RINGBUF_Get(&esp_tester.rb, &c);
		len = slip_parse(&esp_tester.slip, c);
		if (len > 0 && handler != NULL)
		{
			handler(esp_tester.inBuffer, len);
		}
	}
}

void block_test_cb(uint8_t *data, uint16_t len)
{
	if (crc32_compute(esp_tester.inBuffer, 32, NULL) != esp_tester.crc)
	{
		return;
	}
	esp_tester.completed=true;
}

void jigtest_uart_esp_init()
{
	bsp_uart5_init(&esp_tester.rb);
	slip_init(&esp_tester.slip, true, esp_tester.inBuffer, 128, bsp_uart5_send_byte);
}

static int uart_esp_block_test(struct pt *pt)
{
	PT_BEGIN(pt);
	memset(esp_tester.inBuffer, 0, 128);
	RINGBUF_Flush(&esp_tester.rb);
	slip_send(&esp_tester.slip, esp_tester.outBuffer, 32, SLIP_FRAME_COMPLETE);
	static uint32_t tick;
	tick=millis();
	esp_tester.completed=false;
	while(millis()-tick<100 && !esp_tester.completed)
	{
		esp_uart_polling(block_test_cb);
		PT_YIELD(pt);
	}
	PT_END(pt);
}

static int esp_test_uart_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	for (uint8_t i = 1; i < 32; i++)
	{
		esp_tester.outBuffer[i] = i;
	}
	esp_tester.outBuffer[0] = ESP_BLE_HOST_ECHO;
	esp_tester.crc = crc32_compute(esp_tester.outBuffer, 32, NULL);
	for (uint8_t i = 0; i < 3; i++)
	{
		PT_SPAWN(pt, &block_pt, uart_esp_block_test(&block_pt));
		if(esp_tester.completed)
		{
			jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 1);
			break;
		}
	}
	callback();
	PT_END(pt);
}

void jigtest_esp_test_init(task_complete_cb_t cb)
{
	callback=cb;
	jigtest_uart_esp_init();
	PT_INIT(&esp_pt)
}

void jigtest_esp_test_process()
{
	esp_test_uart_thread(&esp_pt);
}

void uart_esp_send_cmd(uint8_t cmd){
	slip_send(&esp_tester.slip, &cmd, 1, SLIP_FRAME_COMPLETE);
}

void uart_esp_send(uint8_t cmd, uint8_t *data, uint8_t len){
	slip_send(&esp_tester.slip, &cmd, 1, SLIP_FRAME_BEGIN);
	slip_send(&esp_tester.slip, data, len, SLIP_FRAME_END);
}

void jigtest_esp_console_handle(char *result)
{
	if(__check_cmd("test esp"))
	{
//		if(jigtest_test_uart_esp())
//		{
//			debug("test esp ok\n");
//		}
//		else{
//			debug("test esp error\n");
//		}
	}
}
