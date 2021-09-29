#ifndef __gps_eva_m8m_h__
#define __gps_eva_m8m_h__

#include "target.h"
#include "WC_Board.h"
#include "gpio.h"

#ifdef __cplusplus
 extern "C" {
#endif

#include "bsp.h"


 /*
  * Implement in bsp
  */
 void eva_m8_reset(void);
 void eva_m8_send_data(uint8_t *data, uint8_t len);
 // Call from isr
 void eva_m8_isr_message_handler(uint8_t *message, uint16_t size);

 /*
  * Call from app
  */
 typedef void (* eva_m8_data_callback_t)(char *latitude, char *lat_dir, char *longitude, char *long_dir, char *hdop);
 void eva_m8_init();
 void eva_m8_process();


#ifdef __cplusplus
}
#endif

#endif
