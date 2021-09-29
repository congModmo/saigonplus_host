#ifndef __crc16_h__
#define __crc16_h__

#ifdef __cplusplus
 extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>
uint16_t crc16(uint8_t *nData, uint16_t wLength);
bool crc16_frame_check(uint8_t *frame, uint16_t frame_size);
uint16_t crc16_append(uint8_t *buf, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif

