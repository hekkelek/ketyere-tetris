/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "types.h"
#include "display.h"
#include "buttons.h"
#include "tetris.h"

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // Turn on LCD backlight
  HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_SET );

  /*
  // Test vibration motor
  for( uint8_t u8Cycles = 0; u8Cycles < 4; u8Cycles++ )
  {
    HAL_GPIO_WritePin( VIBRATION_GPIO_Port, VIBRATION_Pin, GPIO_PIN_SET );
    HAL_Delay( 100 );
    HAL_GPIO_WritePin( VIBRATION_GPIO_Port, VIBRATION_Pin, GPIO_PIN_RESET );
    HAL_Delay( 100 );
  }
 
  // Turn on internal audio amplifier
  HAL_GPIO_WritePin( AMP_SHUTDOWN_GPIO_Port, AMP_SHUTDOWN_Pin, GPIO_PIN_RESET );
 */
  
  // Enable SPI
  LL_SPI_Enable( SPI1 );

  // Initialize LCD
  LCD_Init();
  
  // Initialize buttons
  Buttons_Init();

  // Initialize game
  Tetris_Init();
  
/*
  // Set SPI1 DMA
  LL_SPI_Disable( SPI1 );
  LL_SPI_EnableDMAReq_RX( SPI1 );
  LL_DMA_SetPeriphAddress( DMA2, LL_DMA_STREAM_0, (U32)&(SPI1->DR) );
  //LL_DMA_SetMemoryAddress( DMA2, LL_DMA_STREAM_0, (U32)pu8Buffer );  // RX stream
  LL_DMA_SetDataLength( DMA2, LL_DMA_STREAM_0, 0u );
  //LL_DMA_EnableStream( DMA2, LL_DMA_STREAM_0 );  // RX
  LL_SPI_EnableDMAReq_TX( SPI1 );
  LL_DMA_SetPeriphAddress( DMA2, LL_DMA_STREAM_3, (U32)&(SPI1->DR) );
  //LL_DMA_SetMemoryAddress( DMA2, LL_DMA_STREAM_3, (U32)pu8Buffer );  // TX stream
  LL_DMA_SetDataLength( DMA2, LL_DMA_STREAM_3, 0u );
  //LL_DMA_EnableStream( DMA2, LL_DMA_STREAM_3 );  // TX
  LL_SPI_Enable( SPI1 );
*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Clear screen
    memset( gau8LCDFrameBuffer, 0, sizeof( gau8LCDFrameBuffer ) );
    
    // Run game logic that also generates graphics
    Tetris_Cycle();
    
    // Write to LCD
    LCD_Update();

    // Check menu button
    if( BUTTON_ACTIVE == gaeButtonsState[ BUTTON_MENU ] )
    {
      //TODO: stop game and show OS menu
      
      // Turn power off
      HAL_GPIO_WritePin( POWER_OFF_GPIO_Port, POWER_OFF_Pin, GPIO_PIN_SET );
    }

/*
    static BOOL bMenuButtonEdge = FALSE;
    if( BUTTON_ACTIVE == gaeButtonsState[ BUTTON_MENU ] )
    {
      if( FALSE == bMenuButtonEdge )
      {
        HAL_GPIO_TogglePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin );
        bMenuButtonEdge = TRUE;
      }
    }
    else
    {
      bMenuButtonEdge = FALSE;
    }

    static U8 u8Contrast = 0x42u;
    static BOOL bFireAButtonEdge = FALSE;
    if( BUTTON_ACTIVE == gaeButtonsState[ BUTTON_FIRE_A ] )
    {
      if( FALSE == bFireAButtonEdge )
      {
        // Decrease contrast
        u8Contrast--;
        LCD_SetContrast( u8Contrast );
        bFireAButtonEdge = TRUE;
      }
    }
    else
    {
      bFireAButtonEdge = FALSE;
    }
    static BOOL bFireBButtonEdge = FALSE;
    if( BUTTON_ACTIVE == gaeButtonsState[ BUTTON_FIRE_B ] )
    {
      if( FALSE == bFireBButtonEdge )
      {
        // Inrease contrast
        u8Contrast++;
        LCD_SetContrast( u8Contrast );
        bFireBButtonEdge = TRUE;
      }
    }
    else
    {
      bFireBButtonEdge = FALSE;
    }
*/

    
    
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
