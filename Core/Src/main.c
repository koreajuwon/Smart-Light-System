/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t bh1750_data[2];
uint16_t lux = 0;
uint32_t prevTime = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void OLED_ShowStatus(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
 uint8_t rxData;
 char rxBuffer[50];
 uint8_t rxIndex = 0;
 volatile uint8_t rxFlag = 0;
 uint8_t ledState = 0;
 int brightness = 500;
 void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 {
	  if(huart->Instance == USART2)
	  {
		  if(rxData == '\r' || rxData == '\n')
		  {
			  if(rxIndex > 0)
			  {
			  rxBuffer[rxIndex] = '\0';
			  rxIndex = 0;
			  rxFlag = 1;
			  }

		  }
		  else
		  {
			  if(rxIndex < sizeof(rxBuffer) -1)
			  {
				  rxBuffer[rxIndex++] = rxData;
			  }
		  }
		  HAL_UART_Receive_IT(&huart2,&rxData,1);
	  }
 }
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  HAL_UART_Receive_IT(&huart2,&rxData,1);
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  OLED_ShowStatus();

  uint8_t cmd = 0x10;   // Continuous H-Resolution Mode
  HAL_I2C_Master_Transmit(&hi2c1, 0x23 << 1, &cmd, 1, 100);
  HAL_Delay(180);
  if (HAL_I2C_IsDeviceReady(&hi2c1, 0x23 << 1, 3, 100) == HAL_OK)
  {
      char msg[] = "BH1750 Connected\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
  }
  else
  {
      char msg[] = "BH1750 Not Found\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  if(HAL_I2C_Master_Receive(&hi2c1, 0x23 << 1, bh1750_data, 2, 100) == HAL_OK)
	  {
	      lux = ((bh1750_data[0] << 8) | bh1750_data[1]) / 1.2;
	  }
	  if(HAL_GetTick() - prevTime >= 500)
	  {
	      prevTime = HAL_GetTick();

	      char msg[50];
	      sprintf(msg, "Lux : %d\r\n", lux);
	      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
	  }
	  if(rxFlag)
	  {
		  rxFlag = 0;
		  if(strcmp(rxBuffer, "on") == 0)
		  {
			  ledState = 1;
			  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness);
			  OLED_ShowStatus();
			  char msg[] = "OK: LED ON\r\n";
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }
		  else if(strcmp(rxBuffer, "off") == 0)
		  {
			  ledState = 0;
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
			  OLED_ShowStatus();
			  char msg[] = "OK: LED OFF\r\n";
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

		  }
		  else if(strncmp(rxBuffer, "bright", 6) == 0)
		  {

			  sscanf(rxBuffer, "bright %d", &brightness);
			  if (brightness > 999) brightness = 999;
			  if (brightness < 0 )  brightness = 0;
			  if(ledState)
			      {
			          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness);
			      }
			  OLED_ShowStatus();
			  char msg[50];
			  sprintf(msg, "OK: Brightness: %d\r\n", brightness);
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }
		  else if(strcmp(rxBuffer, "status") == 0)
		  {
			  OLED_ShowStatus();
			  char msg[100];
			  sprintf(msg, "LED: %s\r\nBrightness: %d\r\n", ledState ? "ON" : "OFF", brightness);
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }
		  else if(strcmp(rxBuffer, "fade") == 0)
		  {
			  int i;
			  ledState = 1;
			  brightness = 0;
			  OLED_ShowStatus();
			  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
			  for(i=0; i<=999; i++)
			  {
				  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, i);
				  HAL_Delay(2);
				  if(i % 100 == 0)
				  	{
					  	  brightness = i;
					  	  OLED_ShowStatus();
				  	}
			  }
			  brightness = 999;
			  for(i=999; i>=0; i--)
			  {
				  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, i);
				  HAL_Delay(2);
				  if(i % 100 == 0)
				  	 {
				  		 brightness = i;
				  		 OLED_ShowStatus();
				  	 }
			  }
			  brightness = 0;
			  ledState = 0;
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
			  OLED_ShowStatus();
			  char msg[] = "OK: Fade Complete\r\n";
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }
		  else if(strcmp(rxBuffer, "help") == 0)
		  {
			  char msg[] =
			  "======== Smart Light System ========\r\n"
			  "on               : LED ON\r\n"
			  "off              : LED OFF\r\n"
			  "bright <0-999>   : Set Brightness\r\n"
			  "status           : Show LED Status\r\n"
			  "fade             : Fade In/Out Effect\r\n"
			  "help             : Show Command List\r\n";
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }

		  else
		  {
			  char msg[] = "ERROR: Unknown Command\r\n";
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		  }
	  }
  }
  /* USER CODE END 3 */
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
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void OLED_ShowStatus(void)
{
    char line1[20];
    char line2[20];

    ssd1306_Fill(Black);

    // LED 상태
    if(ledState)
        sprintf(line1, "LED: ON");
    else
        sprintf(line1, "LED: OFF");

    // PWM 값
    sprintf(line2, "PWM: %d", brightness);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(line1, Font_7x10, White);

    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString(line2, Font_7x10, White);

    ssd1306_UpdateScreen();
}
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
#ifdef USE_FULL_ASSERT
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
