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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
int calcOtsuThreshold(uint8_t* pIn, uint32_t imgSize);
void applyThreshold(uint8_t* pIn, uint8_t* pOut, int threshold, uint32_t imgSize);
void calcHistogram(uint8_t* pIn, uint32_t* pHist, uint32_t imgSize);
void extractChannel(uint8_t* pRGB, uint8_t* pOut, int channelOffset, uint32_t numPixels);
void insertChannel(uint8_t* pRGB, uint8_t* pIn, int channelOffset, uint32_t numPixels);
void dilate(uint8_t* pIn, uint8_t* pOut, int w, int h);
void erode(uint8_t* pIn, uint8_t* pOut, int w, int h);
void opening(uint8_t* pIn, uint8_t* pOut, int w, int h);
void closing(uint8_t* pIn, uint8_t* pOut, int w, int h);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t pImage[IMG_SIZE];
uint8_t pImage_Otsu[IMG_SIZE];
uint8_t pImage_RGB[IMG_WIDTH * IMG_HEIGHT * 3];
IMAGE_HandleTypeDef img_handle;

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

    /* Q1: Otsu's Threshold */
    int otsu_thresh = calculateOtsuThreshold((uint8_t*)pImage, IMG_SIZE);
    applyThreshold((uint8_t*)pImage, pImage_Otsu, otsu_thresh, IMG_SIZE);
    img_handle.pData = pImage_Otsu;
    LIB_SERIAL_IMG_Transmit(&img_handle);
    HAL_Delay(1000);

    // --- Q2 SETUP: Request RGB888 Image ---
	LIB_IMAGE_InitStruct(&img_handle, (uint8_t*)pImage_RGB, IMG_WIDTH, IMG_HEIGHT, IMAGE_FORMAT_RGB888);

	while (LIB_SERIAL_IMG_Receive(&img_handle) != SERIAL_OK){
	  HAL_Delay(100);
	}

	/* --- Q2: Color Otsu's Thresholding --- */
	// We iterate otsu's thresholding for each channel and put them back into image.
	/*for (int ch = 0; ch < 3; ch++) {
	  extractChannel((uint8_t*)pImage_RGB, (uint8_t*)pImage, ch, IMG_SIZE);
	  int thresh = calculateOtsuThreshold((uint8_t*)pImage, IMG_SIZE);
	  applyThreshold((uint8_t*)pImage, (uint8_t*)pImage, thresh, IMG_SIZE);
	  insertChannel((uint8_t*)pImage_RGB, (uint8_t*)pImage, ch, IMG_SIZE);
	}

	img_handle.pData = pImage_RGB;
	img_handle.format = IMAGE_FORMAT_RGB888;

	LIB_SERIAL_IMG_Transmit(&img_handle);
	HAL_Delay(1000);

	/* --- Q3: Morphological Operations --- */

	uint8_t* pOutputBuffer = (uint8_t*)pImage_RGB;
	img_handle.format = IMAGE_FORMAT_GRAYSCALE;

	// Dilation
	dilate(pImage_Otsu, pOutputBuffer, IMG_WIDTH, IMG_HEIGHT);
	img_handle.pData = pOutputBuffer;
	LIB_SERIAL_IMG_Transmit(&img_handle);
	HAL_Delay(5000);

	// Erosion
	erode(pImage_Otsu, pOutputBuffer, IMG_WIDTH, IMG_HEIGHT);
	img_handle.pData = pOutputBuffer;
	LIB_SERIAL_IMG_Transmit(&img_handle);
	HAL_Delay(5000);

	// Opening
	opening(pImage_Otsu, pOutputBuffer, IMG_WIDTH, IMG_HEIGHT);
	img_handle.pData = pOutputBuffer;
	LIB_SERIAL_IMG_Transmit(&img_handle);
	HAL_Delay(5000);

	// Closing
	closing(pImage_Otsu, pOutputBuffer, IMG_WIDTH, IMG_HEIGHT);
	img_handle.pData = pOutputBuffer;
	LIB_SERIAL_IMG_Transmit(&img_handle);
	HAL_Delay(5000);
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


int calcOtsuThreshold(uint8_t* pIn, uint32_t imgSize) {
    uint32_t histogram[256];
    calcHistogram(pIn, histogram, imgSize);

    float mean = 0;
    for (int i = 0; i < 256; ++i) {
        mean += i * histogram[i];
    }

    float weight_bg = 0;
    float sum_bg = 0;
    float maxVar = 0;
    int t_optimal = 0;

    for (int t = 0; t < 256; ++t) {
        weight_bg += histogram[t];

        if (weight_bg == 0) continue;

        float weight_fg = imgSize - weight_bg;
        if (weight_fg == 0) break;

        sum_bg += (float)(t * histogram[t]);

        float mean_bg = sum_bg / weight_bg;
        float mean_fg = (mean - sum_bg) / weight_fg;

        float variance_between = weight_bg * weight_fg * (mean_bg - mean_fg) * (mean_bg - mean_fg);

        if (variance_between > maxVar) {
            maxVar = variance_between;
            t_optimal = t;
        }
    }

    return t_optimal;
}

void applyThreshold(uint8_t* pIn, uint8_t* pOut, int threshold, uint32_t imgSize) {
    for (int i = 0; i < imgSize; i++) {
        if (pIn[i] > threshold) {
            pOut[i] = 255;
        } else {
            pOut[i] = 0;
        }
    }
}

void extractChannel(uint8_t* pRGB, uint8_t* pOut, int channelOffset, uint32_t numPixels) {
    for (uint32_t i = 0; i < numPixels; i++) {
        pOut[i] = pRGB[i * 3 + channelOffset];
    }
}

void insertChannel(uint8_t* pRGB, uint8_t* pIn, int channelOffset, uint32_t numPixels) {
    for (uint32_t i = 0; i < numPixels; i++) {
        pRGB[i * 3 + channelOffset] = pIn[i];
    }
}


void dilate(uint8_t* pIn, uint8_t* pOut, int w, int h) {
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            uint8_t pixelVal = 0;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    if (pIn[(y + ky) * w + (x + kx)] == 255) {
                        pixelVal = 255;
                        break;
                    }
                }
                if (pixelVal == 255) {
                    break;
                }
            }
            pOut[y * w + x] = pixelVal;
        }
    }
}

void erode(uint8_t* pIn, uint8_t* pOut, int w, int h) {
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            uint8_t pixelVal = 255;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    if (pIn[(y + ky) * w + (x + kx)] == 0) {
                        pixelVal = 0;
                        break;
                    }
                }
                if (pixelVal == 0) {
                    break;
                }
            }

            pOut[y * w + x] = pixelVal;
        }
    }
}


void opening(uint8_t* pIn, uint8_t* pOut, int w, int h) {

    erode(pIn, (uint8_t*)pImage, w, h);
    dilate((uint8_t*)pImage, pOut, w, h);
}

void closing(uint8_t* pIn, uint8_t* pOut, int w, int h) {
    dilate(pIn, (uint8_t*)pImage, w, h);
    erode((uint8_t*)pImage, pOut, w, h);
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
