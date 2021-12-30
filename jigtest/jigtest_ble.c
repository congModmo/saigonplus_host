
#define __DEBUG__ 4
#include "jigtest.h"
#include "app_main/uart_ui_comm.h"
#include "jigtest_esp.h"
#include "ble/nina_b1.h"
#include "esp_host_comm.h"
#include "app_ble/app_ble.h"
#include "app_fota/ble_dfu.h"
#include "protothread/pt.h"

uint8_t ble_data_buffer[64];
static ble_connection_state_t connection_test;

static struct{
	bool done;
	bool result;
}transceiver_test;

static task_complete_cb_t function_callback;
static struct pt function_pt, connection_pt, transceiver_pt, transceiver_loop_pt, mac_pt, command_pt;

static void esp_polling_callback(uint8_t *frame, size_t len)
{
	if(frame[0]==ESP_BLE_CONNECT_STATE){
		memcpy(&connection_test, frame, sizeof(ble_connection_state_t));
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

static void nina_b1_polling_callback(uint8_t *data, size_t len)
{
	if(data[0]==HOST_COMM_UI_MSG){
		transceiver_test.done=true;
		if((len-1)==sizeof(ble_data_buffer) && memcmp(ble_data_buffer, data+1, len-1)==0){
			transceiver_test.result=true;
		}
		else transceiver_test.result=false;
	}
}

static int transceiver_loop_test(struct pt *pt, bool *test_result)
{
	PT_BEGIN(pt);
	static uint32_t tick;
	*test_result=false;
	transceiver_test.done=false;
	transceiver_test.result=false;
	nina_b1_send1(HOST_COMM_UI_MSG,ble_data_buffer, sizeof(ble_data_buffer));
	tick=millis();
	while(millis()-tick <1000 && !transceiver_test.done)
	{
		jigtest_esp_uart_polling(esp_polling_callback);
		PT_YIELD(pt);
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
		nina_b1_polling(nina_b1_polling_callback);
		PT_YIELD(pt);
	}
	__exit:
	*test_result=transceiver_test.result;
	PT_END(pt);
}

static int ble_transceiver_test(struct pt *pt, bool *test_result)
{
	PT_BEGIN(pt);
	static bool result;
	*test_result=false;
	for(uint8_t i=0; i<sizeof(ble_data_buffer); i++)
	{
		ble_data_buffer[i]=i;
	}
	jigtest_esp_create_cmd(ESP_REDIRECT_DATA_TO_HOST, ESP_REDIRECT_DATA_TO_HOST, 100, 2);
	PT_SPAWN(pt, &command_pt, jigtest_esp_cmd_thread(&command_pt, &result, NULL));
	if(!result)
	{
		goto __exit;
	}
	for(uint8_t i=0; i<3; i++)
	{
		PT_SPAWN(pt, &transceiver_loop_pt, transceiver_loop_test(&transceiver_loop_pt, &result));
		if(result)
		{
			*test_result=true;
			jigtest_direct_report(UART_UI_RES_BLE_TRANSCEIVER, 1);
			break;
		}
	}
	__exit:
	jigtest_esp_create_cmd(ESP_REDIRECT_DATA_TO_GUI, ESP_REDIRECT_DATA_TO_GUI, 100, 2);
	PT_SPAWN(pt, &command_pt, jigtest_esp_cmd_thread(&command_pt, &result, NULL));
	PT_END(pt);
}

static int ble_connection_test(struct pt *pt, bool *test_result)
{
	PT_BEGIN(pt);
	static bool result;
	static uint8_t count;
	static uint32_t tick;
	static int retry;
	*test_result=false;
	jigtest_esp_create_cmd1(ESP_BLE_HOST_MAC, (char *)ble_mac, ESP_BLE_HOST_MAC, 1000, 2);
	PT_SPAWN(pt, &command_pt, jigtest_esp_cmd_thread(&command_pt, &result, NULL));
	if(!result)
		goto __exit;
	tick=millis();
	PT_WAIT_UNTIL(pt, (millis()-tick > 5000));
	count=0;
	retry=10;
	while(count< 3 && retry--)
	{
		uart_esp_send_cmd(ESP_BLE_CONNECT_STATE);
		tick=millis();
		while(millis()-tick<1000)
		{
			jigtest_esp_uart_polling(esp_polling_callback);
			PT_YIELD(pt);
		}
		if(connection_test.connected)
		{
			connection_test.connected=0;
			count++;
		}
		PT_YIELD(pt);
	}
	if(count>=3)
	{
		jigtest_direct_report(UART_UI_RES_BLE_RSSI, connection_test.rssi);
		jigtest_direct_report(UART_UI_RES_BLE_CONNECT, 1);
		*test_result=true;
	}
	__exit:
	do{ }while(0);
	PT_END(pt);
}

static int ble_mac_test(struct pt *pt, bool *test_result)
{
	PT_BEGIN(pt);
	static int count;
	static uint32_t tick;
	*test_result=false;
	count=0;
	tick=millis();
	while(strlen(ble_mac)==0 && count < 5)
	{
		if(millis()- tick > 1000)
		{
			ble_request_cmd(HOST_CMD_SYNC_INFO);
			tick=millis();
			count++;
		}
		app_ble_task();
		PT_YIELD(pt);
	}
	if(strlen(ble_mac)>0)
	{
		jigtest_report(UART_UI_RES_BLE_MAC, (uint8_t *)ble_mac, strlen(ble_mac)+1);
		*test_result=true;
	}
	PT_END(pt);
}

static int function_test_thread(struct pt *pt)
{
	PT_BEGIN(pt);
	static bool result;
	PT_SPAWN(pt, &mac_pt, ble_mac_test(&mac_pt, &result));
	if(!result)
	{
		goto __exit;
	}
	PT_SPAWN(pt, &connection_pt, ble_connection_test(&connection_pt, &result));
	if(!result)
	{
		goto __exit;
	}
	PT_SPAWN(pt, &transceiver_pt, ble_transceiver_test(&transceiver_pt, &result));
	__exit:
	function_callback();
	PT_END(pt);
}

void jigtest_ble_function_test_init(task_complete_cb_t cb)
{
	PT_INIT(&function_pt);
	function_callback=cb;
}

void jigtest_ble_function_test_process()
{
	function_test_thread(&function_pt);
}

/********************************************************************
 * Hardware test
 ********************************************************************/

static task_complete_cb_t hw_callback;

void jigtest_ble_hardware_test_init(task_complete_cb_t cb)
{
	hw_callback=cb;
}

void jigtest_ble_hardware_test_process()
{
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
		goto __exit;
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
	__exit:
	hw_callback();
}

void jigtest_ble_console_handle(char *result)
{
//	if(__check_cmd("test mac")){
//		if(ble_mac_test()){
//			debug("ble mac: %s\n", ble_mac);
//		}
//		else{
//			debug("ble mac null\n");
//		}
//	}
//	else if(__check_cmd("test connection")){
//		if(ble_connection_test()){
//			debug("ble connected\n");
//		}
//		else{
//			debug("ble not found\n");
//		}
//	}
//	else if(__check_cmd("test transceiver")){
//		if(ble_transceiver_test()){
//			debug("transceiver ok\n");
//		}
//		else{
//			debug("transceiver error\n");
//		}
//	}
}
