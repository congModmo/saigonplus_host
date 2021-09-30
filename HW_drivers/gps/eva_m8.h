#ifndef __gps_eva_m8m_h__
#define __gps_eva_m8m_h__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ringbuf/ringbuf.h"

extern uint8_t GLL_MSG[];
extern uint8_t GSA_MSG[];
extern uint8_t VTG_MSG[];
extern uint8_t GSV_MSG[];
extern uint8_t RMC_MSG[];

/*
* Implemented in bsp
*/
void eva_m8_bsp_send_data(uint8_t *data, uint8_t len);
void eva_m8_bsp_set_reset_pin(uint8_t value);
void eva_m8_bsp_uart_init(RINGBUF *rb);


/*
* Call from upper layer
*/
typedef void (* eva_m8_data_callback_t)(char *latitude, char *lat_dir, char *longitude, char *long_dir, char *hdop);

void eva_m8_reset(void);
void eva_m8_init( eva_m8_data_callback_t cb);
void eva_m8_process();

char *eva_m8_ringbuf_polling();
bool gps_frame_checksum(char *frame);
void parse_gga(char *frame);

#ifdef __cplusplus
}
#endif

#endif
