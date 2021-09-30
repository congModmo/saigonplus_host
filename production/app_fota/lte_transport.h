#ifndef __FOTA_H_
#define __FOTA_H_

#include <stdbool.h>
#include <stdint.h>
#include "fota_core.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "fota_core.h"

bool lte_transport_init(void);

#define HTTP_PATH_LEN 256
typedef struct{
    char host[HTTP_PATH_LEN];
	char path[HTTP_PATH_LEN];
	bool secure;
}http_file_info_t;
extern const fota_transport_t lte_transport;

typedef struct{
	fota_file_info_t *info;
	char *link;
	uint32_t sector_timeout;
}lte_get_file_t;

#ifdef __cplusplus
}
#endif

#endif
