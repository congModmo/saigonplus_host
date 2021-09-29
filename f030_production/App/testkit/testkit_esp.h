#ifndef __TESTKIT_ESP_H_
#define __TESTKIT_ESP_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "slip/slip_v2.h"
enum
{
    ESP_BLE_HOST_ECHO,
    ESP_BLE_HOST_IO_VALUE,
    ESP_BLE_HOST_BLINK,
    ESP_BLE_HOST_RSSI,
    ESP_BLE_HOST_MAC,
    ESP_BLE_CONNECT_STATE,
};

void uart_esp_send_cmd(uint8_t cmd);
void uart_esp_send(uint8_t cmd, uint8_t *data, uint8_t len);
void uart_esp_polling(frame_handler handler);
#ifdef __cplusplus
}
#endif
#endif
