#include "jigtest.h"
#include "uart_ui_comm.h"
#include "jigtest_esp.h"
#include "ble/nina_b1.h"
#include "esp_host_comm.h"

uint8_t ble_connected=0;

void ble_get_status_handler(uint8_t *frame, uint16_t len)
{
	if(frame[0]==ESP_BLE_CONNECT_STATE){
		ble_connected=frame[1];
		if(ble_connected){
			jigtest_report(UART_UI_RES_BLE_RSSI, frame+2, sizeof(int));
		}
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

void jigtest_ble_function_test()
{
	uint32_t tick=millis();
//	while(strlen(bikeMainProps.mac)==0 && millis()-tick<5000)
//	{
//		wcb_ble_task();
//		delay(5);
//	}
//	if(strlen(bikeMainProps.mac)==0)
//	{
//		return;
//	}
//	uart_esp_send(ESP_BLE_HOST_MAC, bikeMainProps.mac, strlen(bikeMainProps.mac)+1);
	delay(1000);
	ble_connected=0;
	int retry=5;
	while(retry--&& ble_connected==0)
	{
		uart_esp_send_cmd(ESP_BLE_CONNECT_STATE);
		tick=millis();
		while(millis()-tick<1000)
		{
			delay(5);
			esp_uart_polling(ble_get_status_handler);
			if(ble_connected)
			{
				break;
			}
		}
	}
	jigtest_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_BLE );
}

