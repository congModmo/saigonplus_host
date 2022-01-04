#ifndef __LARA_R2_HTTP_H_
#define __LARA_R2_HTTP_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "app_fota/lte_transport.h"
#include "app_fota/fota_core.h"
#define HTTP_PATH_LEN 128
#define LARA_R2_FILE_BLOCK_LEN 256
bool lara_r2_http_download_file_with_redirecting(http_file_info_t *http_file, fota_file_info_t *file);
bool lara_r2_http_get_file(char *link, fota_file_info_t *file, uint32_t timeout);
#ifdef __cplusplus
}
#endif
#endif
