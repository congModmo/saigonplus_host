/*
 * lara_r2_socket.c
 *
 *  Created on: May 17, 2021
 *      Author: thanhcong
 */
#define __DEBUG__ 3

#include "bsp.h"
#include "lara_r2.h"
#include "lara_r2_socket.h"
#include <string.h>
#include "log_sys.h"

#define SOCKET_TX_RX_BLOCK 128
static lara_r2_socket_t *socket_manager[MAX_MODEM_SOCKET]={NULL};
static uint8_t socket_count=0;
static char at_buff[SOCKET_TX_RX_BLOCK*2+32];

int socket_read_block(lara_r2_socket_t *socket, int block_len){
	int len, port;
	char *response;
	static uint8_t data_buff[SOCKET_TX_RX_BLOCK];
	sprintf(at_buff, "AT+USORD=%d,%d\r\n", socket->port, block_len);
	ASSERT_RET(gsm_send_at_command(at_buff, "OK", 2000, 1, &response), 0, "AT+USORD");
	response=strstr(response, "+USORD: ");
	ASSERT_RET(sscanf(response, "+USORD: %d,%d", &port, &len)==2, false, "Parse param");
	info("actual len: %d\n", len);
	char *hexString=strtok(response, "\"");
	hexString=strtok(NULL, "\"");
	debug("hex string: %s\n", hexString);
	hexStringToBinary(hexString, data_buff);
	for(int i=0; i<len; i++){
		RINGBUF_Put(&socket->rx_rb, data_buff[i]);
	}
	return len;
}

bool socket_urc_read_handle(char *urc){
	int port;
	int len, block_len;
	bool result=false;
	ASSERT_RET(sscanf(urc,"+UUSORD: %d,%d", &port, &len)==2, false, "scanf");
	info("Handle urc to read socket: %d, len: %d\n",port, len);
	uint8_t socket_pos=MAX_MODEM_SOCKET;
	for(uint8_t i=0; i<MAX_MODEM_SOCKET;i++){
		if((socket_manager[i]!=NULL) && (socket_manager[i]->port==port)){
			socket_pos=i;
			break;
		}
	}
	ASSERT_RET(socket_pos<MAX_MODEM_SOCKET, false, "Socket pos");
	LTE_RINGBUF_LOCK();
	while(len >0){
		block_len=(len>SOCKET_TX_RX_BLOCK)?SOCKET_TX_RX_BLOCK:len;
		ASSERT_GO(socket_read_block(socket_manager[socket_pos], block_len)==block_len ,"Read block");
		len-=block_len;
	}
	result=true;
	__exit:
	LTE_RINGBUF_UNLOCK();
	return result;
}

static int network_write_block(uint8_t port, const void *data, uint8_t len){

	info("send block len: %d\n", len);
	sprintf(at_buff, "AT+USOWR=%d,%d,\"", port, len);
	binaryToHexString(data, len, at_buff+strlen(at_buff));
	strcpy(at_buff+strlen(at_buff), "\"\r\n");
	ASSERT_RET(gsm_send_at_command(at_buff, "OK", 2000, 3, NULL), 0, "AT+USOWR");
	return len;
}

static bool handle_tx_request(lara_r2_socket_t *socket){
	socket->tx_result=0;
	int tx_len=socket->tx_len;
	while(tx_len>0){
		int block_len= (tx_len > SOCKET_TX_RX_BLOCK)?SOCKET_TX_RX_BLOCK:tx_len;
		ASSERT_GO(network_write_block(socket->port, socket->tx_buf, block_len)==block_len, "Write block");
		tx_len-=block_len;
	}
	__exit:
	socket->tx_result=socket->tx_len-tx_len;
	socket->tx_len=0;
	return true;
}

static void socket_urc_close_handle(char *urc){
	int port;
	sscanf(urc, "+UUSOCL: %d", &port);
	for(uint8_t i=0; i<MAX_MODEM_SOCKET;i++){
		if((socket_manager[i]!=NULL) && (socket_manager[i]->port==port)){
			socket_manager[i]->state=SOCKET_IDLE;
			socket_manager[i]->port=MAX_MODEM_SOCKET;
		}
	}
}

void lara_r2_socket_urc_handle(char *urc){
	char *pos=strstr(urc, "+UUSOCL: ");
	if(pos!=NULL){
		socket_urc_close_handle(pos);
	}
	pos=strstr(urc, "+UUSORD: ");
	if(pos!=NULL){
		socket_urc_read_handle(pos);
	}
	pos=strstr(urc, "+UUPSDD: ");
	if(pos!=NULL){
		info("All socket are closed\n");
		for(uint8_t i=0; i<MAX_MODEM_SOCKET; i++){
			if(socket_manager[i]!=NULL){
				socket_manager[i]->state=SOCKET_IDLE;
			}
		}
	}
}


static void handle_connect_request(lara_r2_socket_t *socket){
	bool result=false;
	char *at[3];
	char *response;
	at[0]="AT+UDNSRN=0,\"";
	at[1]=socket->host;
	at[2]="\"\r\n";
	ASSERT_GO(gsm_send_at_command2(at, 3, "+UDNSRN: ", 2000, 3, &response), "AT+UDNSRN");
	response=strstr(response,"+UDNSRN:");
	char *ip=strtok(response, "\"");
	ip=strtok(NULL, "\"");
	debug("host ip: %s\n", ip);
	char ip_str[20];
	strcpy(ip_str, ip);
	if(socket->type==SOCKET_TCP){
		ASSERT_GO(gsm_send_at_command("AT+USOCR=6\r\n", "+USOCR: ", 2000, 3, &response), NULL, "AT+USOCR=6");
	}
	else if(socket->type==SOCKET_UDP){
		ASSERT_GO(gsm_send_at_command("AT+USOCR=17\r\n", "+USOCR: ", 2000, 3, &response), NULL, "AT+USOCR=17");
	}
	response=strstr(response, "+USOCR: ");
	sscanf(response, "+USOCR: %d", &socket->port);
	debug("socket port: %d\n", socket->port);
	if(socket->secure){
		sprintf(at_buff, "AT+USOSEC=%d,1,0\r\n", socket->port);
		ASSERT_GO(gsm_send_at_command(at_buff, "OK", 2000, 3, NULL), "AT+USOSEC");
	}
	sprintf(at_buff, "AT+USOCO=%d,\"%s\",%d\r\n", socket->port, ip_str, socket->host_port);
	ASSERT_GO(gsm_send_at_command(at_buff, "OK", 10000, 3, NULL), "AT+USOCO");
	socket->state=SOCKET_CONNECTED;
	RINGBUF_Flush(&socket->rx_rb);
	__exit:
	socket->connect_request=false;
}
static void handle_close_request(lara_r2_socket_t *socket){
	sprintf(at_buff, "AT+USOCL=%d\r\n", socket->port);
	gsm_send_at_command(at_buff, "OK", 500, 2, NULL);
	socket->close_request=false;
}
void lara_r2_socket_process(){
	for(uint8_t i=0; i<MAX_MODEM_SOCKET; i++){
		if(socket_manager[i]!=NULL){
			if(socket_manager[i]->connect_request){
				handle_connect_request(socket_manager[i]);
			}
			if(socket_manager[i]->tx_len){
				handle_tx_request(socket_manager[i]);
			}
			if(socket_manager[i]->close_request){
				handle_close_request(socket_manager[i]);
			}
		}
	}
}

int lara_r2_socket_send(lara_r2_socket_t *socket, const uint8_t *data, uint16_t len){
	if(socket->state!=SOCKET_CONNECTED){
		return -1;
	}
	socket->tx_buf=data;
	socket->tx_len=len;
	while(socket->tx_len){
		delay(5);
	}
	return socket->tx_result;
}

int32_t lara_r2_socket_read(lara_r2_socket_t *socket, uint8_t *data, size_t maxlen){
	if(socket->state!=SOCKET_CONNECTED){
		return -1;
	}
	size_t received=0;
	uint8_t c;
	LTE_RINGBUF_LOCK();
	while(RINGBUF_Available(&socket->rx_rb) && received<maxlen){
		RINGBUF_Get(&socket->rx_rb, &c);
		data[received++]=c;
	}
	LTE_RINGBUF_UNLOCK();
	return received;
}

bool lara_r2_socket_connect(lara_r2_socket_t *socket, char *host, uint16_t port, bool secure){
	socket->host=host;
	socket->host_port=port;
	socket->secure=secure;
	socket->connect_request=true;
	socket->state=SOCKET_IDLE;
	while(socket->connect_request){
		delay(5);
	}
	if(socket->state==SOCKET_CONNECTED){
		return true;
	}
	return false;
}

bool lara_r2_socket_close(lara_r2_socket_t *socket){
	socket->close_request=true;
	while(socket->close_request){
		delay(5);
	}
}

bool lara_r2_socket_init(lara_r2_socket_t *socket, uint8_t type){
	ASSERT_RET(socket_count<MAX_MODEM_SOCKET, NULL, "socket over flow");
	ASSERT_RET(socket!=NULL, NULL, "Socket malloc");
	socket->type=type;
	socket->port=MAX_MODEM_SOCKET; //invalid value, port goes from 0 to MAX_MODEM_SOCKET-1
	socket->state=SOCKET_IDLE;
	RINGBUF_Init(&socket->rx_rb, socket->rx_buf, SOCKET_BUF_LEN);
	socket->tx_result=false;
	for(uint8_t i=0; MAX_MODEM_SOCKET; i++ ){
		if(socket_manager[i]==NULL){
			socket_manager[i]=socket;
			socket_count++;
			return true;
		}
	}
	return false;
}

//void lara_r2_socket_delete(lara_r2_socket_t *socket){
//	if(socket==NULL){
//		return;
//	}
//	for(uint8_t i=0; i<MAX_MODEM_SOCKET; i++){
//		if(socket_manager[i]==socket){
//			socket_manager[i]=NULL;
//			socket_count--;
//			break;
//		}
//	}
//	vPortFree(socket);
//}

bool lara_r2_socket_check(){
	char *response;
	int port=MAX_MODEM_SOCKET;
	ASSERT_RET(gsm_send_at_command("AT+USOCR=6\r\n", "+USOCR: ", 2000, 1, &response), false, "AT+USOCR=6");
	delay(10);
	response=strstr(response, "+USOCR: ");
	sscanf(response, "+USOCR: %d", &port);
	sprintf(at_buff,"AT+USOCL=%d\n\r", port);
	ASSERT_RET(gsm_send_at_command(at_buff, "OK", 2000, 1, &response), false, "AT+USOCL");
	return true;
}

void lara_r2_socket_close_all(){
	for(uint8_t i=0; i<MAX_MODEM_SOCKET; i++){
		sprintf(at_buff, "AT+USOCL=%d\r\n", i);
		gsm_send_at_command(at_buff, "OK", 100, 1, NULL);
		if(socket_manager[i]!=NULL){
			socket_manager[i]->port=MAX_MODEM_SOCKET;
			socket_manager[i]->state=SOCKET_IDLE;
		}
	}
}

void lara_r2_socket_close_active(){
	for(uint8_t i=0; i<MAX_MODEM_SOCKET; i++){
		if(socket_manager[i]->state==SOCKET_CONNECTED){
			sprintf(at_buff, "AT+USOCL=%d\r\n", i);
			gsm_send_at_command(at_buff, "OK", 1000, 2, NULL);
			socket_manager[i]->state=SOCKET_IDLE;
		}
	}
}
