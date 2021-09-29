#ifndef __FOTA_H_
#define __FOTA_H_

#include <stdbool.h>
#include <stdint.h>
#include "crc32.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t httpFileSize;
    int statusCode;
    char location[512];
    size_t contentLen; // actual file size
    size_t contentOffset;
} Http_Header_t;

bool fota_init(void);
bool fota_import_cert(const char *certificate, size_t sz);
char * fota_at_get_file_block(const char *fileName, size_t offset, size_t blockLen);
bool fota_parse_header_file(const char *fileName, Http_Header_t *header);
bool fota_download_file(const char *host, int port, bool secure, const char *path, const char *fileName);
bool fota_download_file_with_redirecting(char *host, int port, bool secure, char *path, const char *fileName);
bool fota_save_file_to_flash(const char *fileName, uint32_t flashAddr, uint32_t maxSz, uint32_t *outFileSz, uint32_t *outCrc32);

#ifdef __cplusplus
}
#endif

#endif
