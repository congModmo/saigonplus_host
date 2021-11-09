/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/

#define __DEBUG__ 4
#include <stdio.h>
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "retarget.h"
#include "flash_fota.h"
#include "bsp.h"
#include "log_sys.h"
#include "slip/slip.h"
#include "crc/crc16.h"
#include "fota_comm.h"
void SystemClock_Config(void);

#define RS232_FOTA_PING 0

typedef  void (*pFunction)(void);
pFunction JumpToApplication;
uint32_t JumpAddress;
host_app_fota_info_t fotaInfo;
void program_jump(uint32_t addr)
{
  if (((*(__IO uint32_t *) addr) & 0x2FFE0000) == 0x20000000)
  {
    HAL_Delay(100);
#ifdef DEBUG
    HAL_UART_DeInit(&huart5);
#endif
    HAL_SPI_DeInit(&hspi1);
    HAL_DeInit();
    SysTick->CTRL  &= ~(SysTick_CTRL_CLKSOURCE_Msk |
              SysTick_CTRL_TICKINT_Msk   |
              SysTick_CTRL_ENABLE_Msk);

    /* Jump to user application */
    JumpAddress = *(__IO uint32_t *) (addr + 4);
    JumpToApplication = (pFunction) JumpAddress;

    /* Initialize user application's Stack Pointer */
    __set_MSP((*(__IO uint32_t *) addr));
    JumpToApplication();
  }
}

bool upgrade_app(){
	if (!flash_fota_erase_mcu_flash_app()){
		error("Erase mcu flash failed\n");
		return false;
	}
	int retry=3;
	while(retry--){
		if(flash_fota_fw_upgrade(&fotaInfo)){
			return true;
		}
	}
	error("It SHOULD NOT GO HERE\n");
	return true;
}

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_SPI1_Init();
#ifdef DEBUG
	MX_USART5_UART_Init();
	RetargetInit(&huart5);
#else
	MX_IWDG_Init();
#endif

	debug("***************************\n");
	debug("*  Hello 1st bootloader  *\n");
	debug("***************************\n");

  	if (!flash_fota_check_valid_app_upgrade(&fotaInfo)){
  		goto __app_jump;
  	}
  	debug("New app available\n");
	int retry=3;
	while(retry--){
		if(upgrade_app()){
			debug("App Update successfully!\n");
			//Jump to bootloader to let it signal that successfully
			//Clear the fw info in second bootloader too.
			break;
		}
	}
	__app_jump:
	program_jump(APPLICATION_START_ADDR);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
