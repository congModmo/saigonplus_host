#ifndef __gsm_lara_r211_h__
#define __gsm_lara_r211_h__

#ifdef __cplusplus
 extern "C" {
#endif
#include "log_sys.h"

#include "bsp.h"

#define LARA_R2_BUFF_SIZE 512
typedef struct {
	char buf[LARA_R2_BUFF_SIZE];
	size_t n;
} GSM_AT_Resp_t;

typedef enum{
	NETWORK_TYPE_NONE,
	NETWORK_TYPE_2G,
	NETWORK_TYPE_4G
}network_type_t;

extern const char aws_trust_ca[];

#define MAX_MODEM_SOCKET 6
void gsm_clear_buffer(void);
void gsm_send_buffer(uint8_t data[], uint16_t len);
void gsm_send_string(char *str);

bool lara_r2_hardware_init(void);
bool lara_r2_software_init(void);
bool gsm_send_at_command(char *atCommand, const char *resOk, int timeout, int retry, char **resp);
bool gsm_send_at_command2(char **atCommand, size_t sz, const char *resOk, int timeout, int retry, char **resp);
bool gsm_at_poll_msg(char **resp);
bool lara_r2_get_network_csq(int *csq);
#ifdef __cplusplus
}
#endif

#endif
