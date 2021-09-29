#include "testkit.h"
#include "uart_ui_comm.h"
#include "testkit_esp.h"

uint8_t ble_connected=0;

void ble_get_status_handler(uint8_t *frame, uint16_t len)
{
	if(frame[0]==ESP_BLE_CONNECT_STATE){
		ble_connected=frame[1];
		if(ble_connected){
			testkit_report(UART_UI_RES_BLE_RSSI, frame+2, sizeof(int));
		}
	}
}

void testkit_ble_hardware_test()
{
	nina_b1_init();
	//normal reset, ble must response ping
	nina_b1_bsp_reset();
	delay(200);
	if (ble_slip_ping(3))
	{
		testkit_direct_report(UART_UI_RES_BLE_TXRX, 1);
	}
	else
	{
		testkit_direct_report(UART_UI_RES_BLE_TXRX, 0);
		testkit_direct_report(UART_UI_RES_BLE_RESET, 0);
		return;
	}
	//set ble to reset state, if it responses ping, reset pin does not work
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, GPIO_PIN_RESET);
	if (ble_slip_ping(3))
	{
		testkit_direct_report(UART_UI_RES_BLE_RESET, 0);
	}
	else
	{
		testkit_direct_report(UART_UI_RES_BLE_RESET, 1);
	}
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, GPIO_PIN_SET);
}

void testkit_ble_function_test()
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
	uart_esp_send(ESP_BLE_HOST_MAC, bikeMainProps.mac, strlen(bikeMainProps.mac)+1);
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
	testkit_direct_report(UART_UI_RES_FINISH, UART_UI_CMD_TEST_BLE );
}

