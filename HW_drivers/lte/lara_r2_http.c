#define __DEBUG__ 4
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "bsp.h"
#include "lara_r2.h"
//#include "app_fota/fota_core.h"
#include "lara_r2_http.h"
typedef struct {
    size_t httpFileSize;
    int statusCode;
    size_t contentLen; // actual file size
    size_t contentOffset;
} Http_Header_t;

typedef struct{
	char path[HTTP_PATH_LEN];
	char fullPath[HTTP_PATH_LEN];
	char host[HTTP_PATH_LEN];
	char buff[HTTP_PATH_LEN];
	bool secure;
	uint16_t port;
}http_get_file_t;

static http_get_file_t get_file;
static Http_Header_t header;

char * fota_at_get_file_block(const char *fileName, size_t offset, size_t blockLen)
{
	char *response;
    char *data_start;
    int len;
    ASSERT_RET(snprintf(get_file.buff, HTTP_PATH_LEN-1, "AT+URDBLOCK= \"%s\",%u,%u\r\n",fileName, offset, blockLen)>0, NULL, "snprinf");
    ASSERT_RET((len=gsm_send_at_command3(get_file.buff, "\"\r\nOK\r\n", 1000, 3, &response))>0, NULL, "AT+URDBLOCK");
    ASSERT_RET(snprintf(get_file.buff, HTTP_PATH_LEN-1, "+URDBLOCK: \"%s\",%u,\"",fileName, blockLen)>0, NULL, "snprinf");
    ASSERT_RET((data_start=strstr(response, get_file.buff))!=NULL, NULL, "data start");
    data_start+=strlen(get_file.buff);
    return data_start;
}

bool fota_parse_header_file(const char *fileName, Http_Header_t *header)
{
    char *at[4];
    char *s, *s0, *s1, *startOfContent;

    size_t fileSz;
    size_t offset = 0;
    uint32_t blockLen, remain;
    uint16_t retry;
    char *response;
    memset(header, 0, sizeof(Http_Header_t));

    // Get http response size
    retry = 0;
    while (retry < 3){
        at[0] = "AT+ULSTFILE=2,\"";
        at[1] = (char *)fileName;
        at[2] = "\"\r\n";
        if (!gsm_send_at_command2(at, 3, "OK", 1000, 3, &response)){
            retry++;
            continue;
        }
        s = strstr(response, "+ULSTFILE: ");
        ASSERT_RET(s, false, "Check +ULSTFILE: ");
        s += strlen("+ULSTFILE: ");
        ASSERT_RET(sscanf(s, "%u", &header->httpFileSize) == 1, false, "Scan file size");
        break;
    }
    ASSERT_RET(header->httpFileSize > 0, false, "Cannot get file size");

    while(offset < header->httpFileSize && (header->statusCode == 0 || header->contentLen == 0 || header->contentOffset == 0)){
        blockLen = (header->httpFileSize - offset) > LARA_R2_FILE_BLOCK_LEN ? LARA_R2_FILE_BLOCK_LEN : (header->httpFileSize - offset);
        startOfContent = fota_at_get_file_block(fileName, offset, blockLen);
        ASSERT_RET(startOfContent, false, "Get file block");
        remain = blockLen;
        // Read line
        s0 = startOfContent;
        s = strstr(s0, "\r\n");
        while (s){
            *s = 0;
            s += 2;
            // Scan tags
            if (strlen(s0) == 0){
                // Empty line ==> start of file content
                header->contentOffset = offset + (uint32_t)(s - s0);
                fileSz = header->httpFileSize - header->contentOffset;
                // double check vs content-len
                ASSERT_RET(fileSz == header->contentLen, false, "File size %u != content len %u\n", fileSz, header->contentLen);
                debug("Parse header OK\n");
                return true;
            }
            if ((s1 = strstr(s0, "HTTP/")) != NULL){
                if ((s1 = strchr(s1 + strlen("HTTP/"), ' ')) != NULL){
                    if (sscanf(s1 + 1, "%d", &header->statusCode) == 1){
                        debug("Status code: %d\n", header->statusCode);
                    }
                }
            }
            else if ((s1 = strstr(s0, "Content-Length: ")) != NULL){
                s1 += strlen("Content-Length: ");
                if (sscanf(s1, "%u", &header->contentLen) == 1){
                    debug("Content len: %u\n", header->contentLen);
                }
            }
            offset = offset + (uint32_t)(s - s0);
            remain = remain - (uint32_t)(s - s0);
            s0 = s;
            s = strstr(s0, "\r\n");
        }

        if ((offset + remain) >= header->httpFileSize){
            error("Reach end of fife without finishing parse header\n");
            break;
        }
    }
    return false;
}

bool fota_download_file(const char *host, int port, bool secure, const char *path, const char *fileName, uint32_t timeout)
{
    char *at[5];
    int i;
    char *response;
	ASSERT_RET(gsm_send_at_command("AT+UHTTP=0\r\n", "OK", 500, 3, NULL),false, "Reset HTTP profile");
	if (sscanf(host, "%d.%d.%d.%d", &i, &i, &i, &i) == 4){
		at[0] = "AT+UHTTP=0,0,\"";
	}
	else {
		at[0] = "AT+UHTTP=0,1,\"";
	}
	at[1] = (char *)host;
	at[2] = "\"\r\n";
	ASSERT_RET(gsm_send_at_command2(at, 3, "OK", 500, 3, NULL),false,  "Set HTTP hostname/IP");
//		if (port){
//			at[0] = "AT+UHTTP=0,5,";
//			sprintf(buf, "%d\r\n", port);
//			at[1] = buf;
//			ASSERT_GO(gsm_send_at_command2(at, 2, "OK", 500, 3, NULL), false, "Set HTTP port");
//		}
	if (secure) {
		ASSERT_RET(gsm_send_at_command("AT+UHTTP=0,6,1\r\n", "OK", 500, 3, NULL), false, "Set HTTP secure option");
	}
	else{
		ASSERT_RET(gsm_send_at_command("AT+UHTTP=0,6,0\r\n", "OK", 500, 3, NULL), false, "Set HTTP secure option");
	}
	at[0] = "AT+UHTTPC=0,1,\"";
	at[1] = (char *)path;
	at[2] = "\",\"";
	at[3] = (char *)fileName;
	at[4] = "\"\r\n";
	ASSERT_RET(gsm_send_at_command2(at, 5, "+UUHTTPCR: 0,1," /* "+UUHTTPCR: 0,1,1" */, timeout, 1, &response), false, "Execute HTTP get file");
	response=strstr(response, "+UUHTTPCR: 0,1,")+strlen("+UUHTTPCR: 0,1,");
	ASSERT_RET(response[0]=='1', false, "Http get file error");
	debug("HTTP response OK: %s\n", fileName);
	return true;

}

static bool download_http_file(char *host, int port, bool secure, char *path, const char *fileName, uint32_t timeout)
{
//    char *s, *s1;
//    int retry = 0;
//    while (retry++ < 5){
	ASSERT_RET(fota_download_file(host, port, secure, path, fileName, timeout), false, "Download fw failed");
	// Check for other status code rather than 200 (eg. 302)
	ASSERT_RET(fota_parse_header_file(fileName, &header), false, "Parse header");
	if (header.statusCode == 200)
		return true;
	return false;
}

static bool get_file_sector(char *file_name, uint32_t file_offset, uint16_t len, uint8_t *buff){
	uint32_t sector_offset=0;
	uint32_t page_len;
    debug("Get sector offset: 0x%x\n", file_offset);
	char *startOfContent;
	while(sector_offset < len){
		page_len = (len - sector_offset) > LARA_R2_FILE_BLOCK_LEN ? LARA_R2_FILE_BLOCK_LEN : (len - sector_offset);
		ASSERT_RET((startOfContent = fota_at_get_file_block(file_name, header.contentOffset +file_offset +sector_offset, page_len)) != NULL, false, "Get block");
		memcpy(buff+sector_offset, startOfContent, page_len);
        sector_offset += page_len;
	}
	return true;
}

static bool fota_save_file_to_flash(fota_file_info_t *file)
{
    size_t file_offset = 0;
    uint32_t sector_len;
    uint32_t flashAddr=file->flash_addr;
    // ASSERT_RET(fota_parse_header_file(file->file_name, &header), false, "Parse header");
    while(file_offset < header.contentLen){
        sector_len = (header.contentLen - file_offset) > FLASH_FOTA_SECTOR_SZ ? FLASH_FOTA_SECTOR_SZ : (header.contentLen - file_offset);
        ASSERT_RET(get_file_sector(file->file_name, file_offset, sector_len, fotaCoreBuff), false, "get sector");
        ASSERT_RET(GD25Q16_WriteAndVerifySector(flashAddr, fotaCoreBuff, sector_len), false, "write sector to flash");
        flashAddr += sector_len;
        file_offset += sector_len;
    }
    ASSERT_RET(GD25Q16_Crc32_check(file->flash_addr, file->len, file->crc), false, "crc check");
    debug("All block read OK\n");
    return true;
}

static bool parse_link(char *link, char *file_name){
	char *p=NULL;

	if(NULL!=(p=strstr(link, "http://"))){
		get_file.secure=false;
		get_file.port=80;
		p+=strlen("http://");
	}
	else if(NULL!=(p=strstr(link, "https://"))){
		get_file.secure=true;
		get_file.port=443;
		p+=strlen("https://");
	}
	else{
		return false;
	}
	char *pf=strchr(p, '/');
	if(pf==NULL){
		return false;
	}
	int host_len=(pf-p);
	if(host_len > (HTTP_PATH_LEN -1)){
		return false;
	}
	int path_len =strlen(p)-host_len;
	if(path_len > HTTP_PATH_LEN-strlen(file_name)-3){
		return false;
	}
	memcpy(get_file.host, p, host_len);
	get_file.host[host_len]='\0';
	if(link[strlen(link)-1]=='/'){
		strcpy(get_file.path, pf);
	}
	else{
		sprintf(get_file.path, "%s/", pf);
	}
	debug("host: %s\n", get_file.host);
	debug("path: %s\n", get_file.path);
	return true;
}

static void delete_lte_file(char  *name){
	char *at[3];
	at[0] = "AT+UDELFILE=\"";
	at[1] = name;
	at[2] = "\"\r\n";
	gsm_send_at_command2(at, 3, "OK", 500, 1, NULL);
}

static bool download_short_file(fota_file_info_t *file, uint32_t timeout){
	bool status =false;
	sprintf(get_file.fullPath, "%s%s", get_file.path, file->file_name);
	debug("file: %s\n", get_file.fullPath);
	ASSERT_RET(download_http_file(get_file.host, get_file.port, get_file.secure, get_file.fullPath, file->file_name, timeout), false, "downloade file");
	ASSERT_GO(fota_save_file_to_flash(file), "Save file");
	status=true;
	__exit:
	delete_lte_file(file->file_name);
	return status;
}

static bool download_long_file(fota_file_info_t *file, uint32_t timeout){
	static int len, offset, idx;
	len=0; idx=0; offset=0;
	static fota_file_info_t partFile;
	while(offset<file->len){
		len= ((file->len - offset)>FLASH_FOTA_SECTOR_SZ)? FLASH_FOTA_SECTOR_SZ: (file->len-offset);
		sprintf(partFile.file_name, "%s%d", file->file_name, idx);
		sprintf(get_file.fullPath, "%s%s", get_file.path, partFile.file_name);
		ASSERT_RET(download_http_file(get_file.host, get_file.port, get_file.secure, get_file.fullPath, partFile.file_name, timeout), false, "Download file");
		ASSERT_RET(get_file_sector(partFile.file_name, 0, len, fotaCoreBuff), false, "get sector");
		partFile.flash_addr=file->flash_addr+offset;
		ASSERT_RET(GD25Q16_WriteAndVerifySector(partFile.flash_addr, fotaCoreBuff, len), false, "write sector to flash");
		delete_lte_file(partFile.file_name);
		offset+=len;
		idx++;
	}
	ASSERT_RET(GD25Q16_Crc32_check(file->flash_addr, file->len, file->crc), false, "Crc error");
	return true;
}

bool lara_r2_http_get_file(char *link, fota_file_info_t *file, uint32_t timeout){

	if(!parse_link(link, file->file_name)){
		error("Parse link\n");
		return false;
	}
//	if(file->len <=FLASH_FOTA_SECTOR_SZ){
		return download_short_file(file, timeout);
//	 }
//	else{
//		return download_long_file(file, timeout);
//	}
}
