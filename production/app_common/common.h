#ifndef __common_h__
#define __common_h__

#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

void STM32_ParseStr(char str[], char delimit[], char *ptr[]);
uint8_t STM32_CompareByte(char source_buffer[], char dest_buffer[], uint16_t len);
void STM32_UpperToLower(char upper[], char lower[]);
uint16_t min(uint16_t a, uint16_t b);
uint8_t ascii_2_hex(uint8_t val);
uint8_t STM32_CompareWord(int32_t source_buffer[], int32_t dest_buffer[], uint16_t len);
uint8_t STM32_CRC8Cal(uint8_t data[], uint8_t len);
bool timeout_check(uint32_t *from, uint32_t interval);
#define timeout_init(timeout) (*timeout) = millis()

int hexStringToBinary(char *string, uint8_t *buf);
void binaryToHexString(uint8_t *hex, int len, char *string);

uint8_t dec_to_bcd(uint8_t n);
uint8_t bcd_to_dec(uint8_t n);


#ifdef __cplusplus
}
#endif

#endif

