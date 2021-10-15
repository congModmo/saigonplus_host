
#define __DEBUG__ 4
#include "jigtest.h"
#include "uart_ui_comm.h"
#include "jigtest_esp.h"
#include "ble/nina_b1.h"
#include "esp_host_comm.h"
#include "app_ble/app_ble.h"

uint8_t ble_connected=0;
uint8_t ble_data_buffer[64];

static struct{
	bool done;
	bool result;
}transceiver_test;

static void esp_polling_callback(uint8_t *frame, uint16_t len)
{
	if(frame[0]==ESP_BLE_CONNECT_STATE){
		ble_connection_state_t *state=(ble_connection_state_t *)frame;
		ble_connected=state->connected;
		int rssi=(int)state->rssi;
		if(state->connected){
			jigtest_report(UART_UI_RES_BLE_RSSI, (uint8_t *)&rssi, sizeof(int));
		}
	}
	else if(frame[0]== ESP_BLE_HOST_DATA)
	{
		transceiver_test.done=true;
		if((len-1)==sizeof(ble_data_buffer) && memcmp(ble_data_buffer, frame+1, len-1)==0){
			transceiver_test.result=true;
		}
		else transceiver_test.result=false;
	}
}

void nina_b1_polling_callback(uint8_t *data, size_t len)
{
	if(data[0]==HOST_COMM_UI_MSG){
		transceiver_test.done=true;
		if((len-1)==sizeof(ble_data_buffer) && memcmp(ble_data_buffer, data+1, len-1)==0){
			transceiver_test.result=true;
		}
		else transceiver_test.result=false;
	}
}

void jigtest_ble_hardware_test()
{
	//make sure that no ble app program exist yet
	nina_b1_init();
	nina_b1_reset();
	delay(200);
	if (ble_slip_ping(3))
	{
		jigtest_direct_report(UART_UI_RES_BLE_TXRX, 1);
	}
	else
	{
		jigtest_direct_report(UART_UI_RES_BLE_TXRX, 0);
		jigtest_direct_report(UART_UI_RES_BLE_RESET, 0);
		return;
	}
	//set ble to reset state, if it responses ping, reset pin does not work
	nina_b1_bsp_set_reset_pin(0);
	if (ble_slip_ping(3))
	{
		jigtest_direct_report(UART_UI_RES_BLE_RESET, 0);
	}
	else
	{
		jigtest_direct_report(UART_UI_RES_BLE_RESET, 1);
	}
	nina_b1_bsp_set_reset_pin(1);
}

static bool ble_mac_test()
{
	uint32_t tick=millis();
	app_ble_init();
	while(strlen(ble_mac)==0 && millis()-tick<5000)
	{
		app_ble_task();
		delay(5);
	}
	if(strlen(ble_mac)==0)
	{
		return false;
	}
}

static bool ble_connection_test()
{
	uint32_t tick;
	uart_esp_send(ESP_BLE_HOST_MAC, ble_mac, strlen(ble_mac));
	ble_connected=0;
	int retry=5;
	while(retry--&& ble_connected==0)
	{
		uart_esp_send_cmd(ESP_BLE_CONNECT_STATE);
		tick=millis();
		while(millis()-tick<1000 && !ble_connected)
		{
			delay(5);
			esp_uart_polling(esp_polling_callback);
		}
	}
	return ble_connected;
}


static bool ble_transceiver_test()
{
	uint32_t tick;
	for(uint8_t i=0; i<sizeof(ble_data_buffer); i++){
		ble_data_buffer[i]=i;
	}
	uart_esp_send_cmd(ESP_REDIRECT_DATA_TO_HOST);
	nina_b1_send1(HOST_COMM_UI_MSG,ble_data_buffer, sizeof(ble_data_buffer) );
	tick=millis();
	transceiver_test.done=false;
	transceiver_test.result=false;
	while(millis()-tick <1000 && !transceiver_test.done)
	{
		delay(5);
		esp_uart_polling(esp_polling_callback);
	}
	if(!transceiver_test.result){
		goto __exit;
	}
	uart_esp_send(ESP_BLE_HOST_DATA, ble_data_buffer, sizeof(ble_data_buffer));
	tick=millis();
	transceiver_test.done=false;
	transceiver_test.result=false;
	while(millis()-tick <1000 && !transceiver_test.done)
	{
		delay(5);
		nina_b1_polling(nina_b1_polling_callback);
	}
	__exit:
	uart_esp_send_cmd(ESP_REDIRECT_DATA_TO_GUI);
	return transceiver_test.result;
}

void jigtest_ble_function_test()
{
	jigtest_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_BLE );
}

void jigtest_ble_console_handle(char *result)
{
	if(__check_cmd("test mac")){
		if(ble_mac_test()){
			debug("ble mac: %s\n", ble_mac);
		}
		else{
			debug("ble mac null\n");
		}
	}
	else if(__check_cmd("test connection")){
		if(ble_connection_test()){
			debug("ble connected\n");
		}
		else{
			debug("ble not found\n");
		}
	}
	else if(__check_cmd("test transceiver")){
		if(ble_transceiver_test()){
			debug("transceiver ok\n");
		}
		else{
			debug("transceiver error\n");
		}
	}
}
