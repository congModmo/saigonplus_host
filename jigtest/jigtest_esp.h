#ifndef __TESTKIT_ESP_H_
#define __TESTKIT_ESP_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "slip/slip_v2.h"
#include "jigtest.h"

void uart_esp_send_cmd(uint8_t cmd);
void uart_esp_send(uint8_t cmd, uint8_t *data, uint8_t len);
void uart_esp_polling(frame_handler handler);
void jigtest_esp_test_init(task_complete_cb_t cb);
void jigtest_esp_test_process();
#ifdef __cplusplus
}
#endif
#endif
