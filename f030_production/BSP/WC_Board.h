#ifndef __WC_Board_H__
#define __WC_Board_H__

#ifdef __cplusplus
 extern "C" {
#endif


// Define modules
// De init gsm va lay ccid, imei thoai...
#define LTE_MODULE
// Cho phep connect mqtt-sn to thingstream

/*
    Allow generate bike mock data then publish to server without wait for LOCK/UNLOCK pin activated
*/
// #define MOCK_BIKE_DATA
/*
    Allow data to be published to server even its data quality is not good ('A')
*/
// #define FORCE_PUBLISH_GPS
//#define WDT_ENABLE

#define FW_VERSION  "0.4.8"

// Define LED status
#define LED_1              GPIO_PIN_11  // GPIOA
#define LED_2              GPIO_PIN_10  // GPIOA
#define LED_3              GPIO_PIN_9   // GPIOA
#define LED_4              GPIO_PIN_8   // GPIOA

// Define Buzzer pin
#define BUZZER             GPIO_PIN_0   // GPIOB

// Define GPIO for control and read status
#define INPUT_1            GPIO_PIN_7  // GPIOC
#define INPUT_2            GPIO_PIN_6  // GPIOC
#define OUTPUT_1           GPIO_PIN_9  // GPIOC
#define OUTPUT_2           GPIO_PIN_8  // GPIOC

// RS232 PIN
#define RS232_TX_PIN       GPIO_PIN_4  // GPIOC
#define RS232_RX_PIN       GPIO_PIN_5  // GPIOC

// Define GPIO for GPS - EVA-M8M-O
#define GPS_RESET_PIN      GPIO_PIN_12 // GPIOB
#define GPS_EXTINT_PIN     GPIO_PIN_13 // GPIOB
#define GPS_BOOTMODE_PIN   GPIO_PIN_14 // GPIOB
#define GPS_TX_PIN         GPIO_PIN_10 // GPIOC
#define GPS_RX_PIN         GPIO_PIN_11 // GPIOC

// Define GPIO for GSM - LARA R211
#define GSM_LS_EN          GPIO_PIN_2  // GPIOC
#define GSM_DTR_PIN        GPIO_PIN_6  // GPIOB
#define GSM_DSR_PIN        GPIO_PIN_5  // GPIOB
#define GSM_DCD_PIN        GPIO_PIN_12 // GPIOA
#define GSM_RTS_PIN        GPIO_PIN_1  // GPIOB
#define GSM_CTS_PIN        GPIO_PIN_2  // GPIOB
#define GSM_RESET_PIN      GPIO_PIN_15 // GPIOB
#define GSM_PWR_ON_PIN     GPIO_PIN_3  // GPIOC
#define GSM_TX_PIN         GPIO_PIN_10 // GPIOB
#define GSM_RX_PIN         GPIO_PIN_11 // GPIOB

// Define GPIO for BLE - NINA B111
#define BLE_RESET_PIN      GPIO_PIN_1  // GPIOA
#define BLE_TX_PIN         GPIO_PIN_2  // GPIOA
#define BLE_RX_PIN         GPIO_PIN_3  // GPIOA

// Define GPIO for GD25Q16ETIGR
#define FLASH_HOLD         GPIO_PIN_0  // GPIOC
#define FLASH_WP           GPIO_PIN_0  // GPIOA
#define SPI1_CS_PIN        GPIO_PIN_4  // GPIOA
#define SPI1_SCK_PIN       GPIO_PIN_5  // GPIOA
#define SPI1_MISO_PIN      GPIO_PIN_6  // GPIOA
#define SPI1_MOSI_PIN      GPIO_PIN_7  // GPIOA



	  
#ifdef __cplusplus
}
#endif

#endif
