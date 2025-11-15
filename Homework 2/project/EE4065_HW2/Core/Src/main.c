/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include "lib_image.h"
#include "lib_serialimage.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define IMG_WIDTH 128
#define IMG_HEIGHT 128
#define IMG_SIZE (IMG_WIDTH * IMG_HEIGHT)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint32_t histogram_data[256];
uint32_t histogram_data_equalized[256];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void calcHistogram(uint8_t* pIn, uint32_t* pHist, uint32_t imgSize);
void equalHistogram(uint8_t* pIn, uint8_t* pOut, uint32_t* pHist_CDF, uint32_t imgSize);
void convolve(uint8_t* pIn, uint8_t* pOut, int w, int h, const float* kernel, float divisor);
void medianFilter(uint8_t* pIn, uint8_t* pOut, int w, int h);
void sort(uint8_t arr[9]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t pImage[IMG_SIZE];
uint8_t pImage_Equalized[IMG_SIZE];
uint8_t pImage_LowPass[IMG_SIZE];
uint8_t pImage_HighPass[IMG_SIZE];
uint8_t pImage_Median[IMG_SIZE];
IMAGE_HandleTypeDef img_handle;

const float KERNEL_LPF[9] = { 1, 1, 1,
                              1, 1, 1,
                              1, 1, 1 };
const float DIVISOR_LPF = 9.0f;

const float KERNEL_HPF[9] = { -1, -1, -1,
                              -1,  8, -1,
                              -1, -1, -1 };
const float DIVISOR_HPF = 1.0f;
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  LIB_IMAGE_InitStruct(&img_handle, (uint8_t*)pImage, IMG_WIDTH, IMG_HEIGHT, IMAGE_FORMAT_GRAYSCALE);

  while (LIB_SERIAL_IMG_Receive(&img_handle) != SERIAL_OK){
		HAL_Delay(100);
  }

  // Q1: Histogram
  calcHistogram((uint8_t*)pImage, histogram_data, IMG_SIZE);

  // Q2: Histogram Equalization
  equalHistogram((uint8_t*)pImage, pImage_Equalized, histogram_data, IMG_SIZE);
  calcHistogram(pImage_Equalized, histogram_data_equalized, IMG_SIZE);
  img_handle.pData = pImage_Equalized;
  LIB_SERIAL_IMG_Transmit(&img_handle);
  HAL_Delay(2000);

  // Q3: Convolution
  convolve((uint8_t*)pImage, pImage_LowPass, IMG_WIDTH, IMG_HEIGHT, KERNEL_LPF, DIVISOR_LPF);
  img_handle.pData = pImage_LowPass;
  LIB_SERIAL_IMG_Transmit(&img_handle);
  HAL_Delay(2000);
  convolve((uint8_t*)pImage, pImage_HighPass, IMG_WIDTH, IMG_HEIGHT, KERNEL_HPF, DIVISOR_HPF);
  img_handle.pData = pImage_HighPass;
  LIB_SERIAL_IMG_Transmit(&img_handle);
  HAL_Delay(2000);

  medianFilter((uint8_t*)pImage, pImage_Median, IMG_WIDTH, IMG_HEIGHT);
  img_handle.pData = pImage_Median;
  LIB_SERIAL_IMG_Transmit(&img_handle);
  HAL_Delay(2000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 2000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void calcHistogram(uint8_t* pIn, uint32_t* pHist, uint32_t imgSize){
    for (int i = 0; i < 256; i++) {
        pHist[i] = 0;
    }

    for (int i = 0; i < imgSize; i++) {
        pHist[pIn[i]]++;
    }
}

void equalHistogram(uint8_t* pIn, uint8_t* pOut, uint32_t* pHist_in, uint32_t imgSize){
	    calcHistogram(pIn, pHist_in, imgSize);

	    static uint32_t cdf[256]; // keep cdf away from stack
	    cdf[0] = pHist_in[0];
	    for (int i = 1; i < 256; i++) {
	        cdf[i] = cdf[i - 1] + pHist_in[i];
	    }

	    // Formula: h'(g_k) = ((L-1) / MN) * CDF(k)
	    // L-1 = 255
	    // MN = imgSize
	    float scale_factor = 255.0f / (float)imgSize;

	    uint8_t lut[256];
	    for (int k = 0; k < 256; k++) {
	        lut[k] = (uint8_t)(scale_factor * (float)cdf[k]);
	    }

	    for (int i = 0; i < imgSize; i++) {
	        pOut[i] = lut[pIn[i]];
	    }
}

void convolve(uint8_t* pIn, uint8_t* pOut, int w, int h, const float* kernel, float divisor){
    float sum;
    int pixel_pos;

    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {

            sum = 0.0f;

            sum += (float)pIn[(y-1)*w + (x-1)] * kernel[0];
            sum += (float)pIn[(y-1)*w + (x)  ] * kernel[1];
            sum += (float)pIn[(y-1)*w + (x+1)] * kernel[2];

            sum += (float)pIn[(y)*w   + (x-1)] * kernel[3];
            sum += (float)pIn[(y)*w   + (x)  ] * kernel[4];
            sum += (float)pIn[(y)*w   + (x+1)] * kernel[5];

            sum += (float)pIn[(y+1)*w + (x-1)] * kernel[6];
            sum += (float)pIn[(y+1)*w + (x)  ] * kernel[7];
            sum += (float)pIn[(y+1)*w + (x+1)] * kernel[8];

            sum = sum / divisor;

            // clamping olayi
            if (sum < 0)   sum = 0;
            if (sum > 255) sum = 255;

            pixel_pos = y*w + x;
            pOut[pixel_pos] = (uint8_t)sum;
        }
    }
}

void sort(uint8_t arr[9]) {
    for (int i = 0; i < 9 - 1; i++) {
        for (int j = 0; j < 9 - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                uint8_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void medianFilter(uint8_t* pIn, uint8_t* pOut, int w, int h)
{
    uint8_t window[9];

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {

            window[0] = pIn[(y-1)*w + (x-1)];
            window[1] = pIn[(y-1)*w + (x)  ];
            window[2] = pIn[(y-1)*w + (x+1)];
            window[3] = pIn[(y)*w   + (x-1)];
            window[4] = pIn[(y)*w   + (x)  ];
            window[5] = pIn[(y)*w   + (x+1)];
            window[6] = pIn[(y+1)*w + (x-1)];
            window[7] = pIn[(y+1)*w + (x)  ];
            window[8] = pIn[(y+1)*w + (x+1)];

            sort(window);

            pOut[y*w + x] = window[4];
        }
    }
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
