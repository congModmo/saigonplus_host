#ifndef __ss_common_h__
#define __ss_common_h__


#include "stdbool.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void flash_ext_init(void);
void flash_ext_read_report_periodic(void);
void flash_ext_write_report_periodic(void);
void flash_ext_read_operation_mode(void);
void flash_ext_write_operation_mode(void);
void flash_ext_write_serialid(uint8_t _serialid[]);
uint16_t flash_ext_read_serialid(uint8_t _serialid[], uint16_t maxLen);
uint16_t flash_ext_read_bikeid(uint8_t _bikeid[], uint16_t maxLen);
void flash_ext_write_bikeid(uint8_t _bikeid[]);

#ifdef __cplusplus
}
#endif

#endif
