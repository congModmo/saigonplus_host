#ifndef __TESTKIT_ESP_H_
#define __TESTKIT_ESP_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "slip/slip_v2.h"
#include "jigtest.h"
#include "protothread/pt.h"

void uart_esp_send_cmd(uint8_t cmd);
void uart_esp_send0(uint8_t *data, uint8_t len);
void uart_esp_send(uint8_t cmd, uint8_t *data, uint8_t len);

void jigtest_esp_uart_polling(frame_handler handler);
void jigtest_esp_test_init(task_complete_cb_t cb);
void jigtest_esp_test_process();
void jigtest_esp_create_cmd(uint8_t cmd, uint8_t expected, uint32_t interval, int retry);
void jigtest_esp_create_cmd1(uint8_t cmd, char *params, uint8_t expected, uint32_t interval, int retry);
int jigtest_esp_cmd_thread(struct pt *pt, bool *result, uint8_t **response);
#ifdef __cplusplus
}
#endif
#endif
