#ifndef __ble_nina_b111_h__
#define __ble_nina_b111_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "ringbuf/ringbuf.h"
#include <string.h>
#include <stdbool.h>
#include "app_fota/serial_transport.h"
#include "slip/slip_v2.h"

    extern const serial_interface_t ble_serial;

    bool nina_b1_send_cmd(uint8_t type, uint8_t *data, size_t len, int timeout, int retry);
    void nina_b1_init(void);
    void nina_b1_polling(frame_handler handler);
    void nina_b1_send(uint8_t type, uint8_t *data, size_t len);
    bool nina_b1_ask(uint8_t type, uint8_t *data, size_t len, uint8_t expected, int timeout, int retry);
    bool nina_b1_ask_get_response(uint8_t *cmd, size_t cmd_len, uint8_t **resp, uint32_t *resp_len, size_t timeout, int retry);
    //implement in bsp
    void nina_b1_bsp_send_byte(uint8_t b);
    void nina_b1_bsp_send_buffer(uint8_t *data, uint16_t len);
    void nina_b1_bsp_init(RINGBUF *rb);
    void nina_b1_bsp_reset();
    void nina_b1_send1(uint8_t type, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
