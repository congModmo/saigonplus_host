#define __DEBUG__ 4
#include "jigtest.h"
#include "uart_ui_comm.h"
#include "jigtest_esp.h"
#include "bsp.h"
#include "esp_host_comm.h"
#include "bsp.h"
typedef struct
{
	RINGBUF rb;
	uint8_t outBuffer[32];
	uint8_t inBuffer[128];
	slip_t slip;
	uint32_t crc;
	bool completed;
} esp_tester_t;

static esp_tester_t esp_tester;

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

bool uart_esp_block_test(){
	memset(esp_tester.inBuffer, 0, 128);
	RINGBUF_Flush(&esp_tester.rb);
	slip_send(&esp_tester.slip, esp_tester.outBuffer, 32, SLIP_FRAME_COMPLETE);
	uint32_t tick=millis();
	esp_tester.completed=false;
	while(millis()-tick<100 && !esp_tester.completed){
		esp_uart_polling(block_test_cb);
		delay(5);
	}
	return esp_tester.completed;
}

void jigtest_uart_esp_init(){
	bsp_uart5_init(&esp_tester.rb);
	slip_init(&esp_tester.slip, true, esp_tester.inBuffer, 128, bsp_uart5_send_byte);
}

bool jigtest_test_uart_esp()
{
	for (uint8_t i = 1; i < 32; i++)
	{
		esp_tester.outBuffer[i] = i;
	}
	esp_tester.outBuffer[0] = ESP_BLE_HOST_ECHO;
	esp_tester.crc = crc32_compute(esp_tester.outBuffer, 32, NULL);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (!uart_esp_block_test())
		{
			return false;
		}
	}

	jigtest_direct_report(UART_UI_RES_ESP_ADAPTOR, 1);
	return true;
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
		if(jigtest_test_uart_esp())
		{
			debug("test esp ok\n");
		}
		else{
			debug("test esp error\n");
		}
	}
}
