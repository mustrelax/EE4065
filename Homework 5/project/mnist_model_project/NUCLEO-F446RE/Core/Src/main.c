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
#include "network.h"
#include "stai.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYNC_BYTE 0xBB
#define IMG_SIZE 28
#define IMG_BYTES 784
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

static const float feat_mean[7] = {0.334226f, 0.044768f, 0.008187f, 0.001415f,
                                   0.000007f, 0.000156f, -0.000006f};
static const float feat_std[7] = {0.084046f, 0.063766f, 0.014061f, 0.002583f,
                                  0.000082f, 0.000721f, 0.000063f};

/* Network buffers */
STAI_ALIGNED(8) static uint8_t net_ctx[STAI_NETWORK_CONTEXT_SIZE];
STAI_ALIGNED(4) static uint8_t acts[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];
static stai_ptr act_ptrs[] = {acts};

STAI_ALIGNED(4) static float in_buf[7];
STAI_ALIGNED(4) static float out_buf[10];
static stai_ptr in_ptrs[] = {(stai_ptr)in_buf};
static stai_ptr out_ptrs[] = {(stai_ptr)out_buf};

static uint8_t img_buf[IMG_BYTES];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void ComputeHuMoments(uint8_t *img, float *hu);
static void Normalize(float *f);
static int AI_Init(void);
static uint8_t ArgMax(float *d, int n);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void ComputeHuMoments(uint8_t *img, float *hu) {
  double m00 = 0, m10 = 0, m01 = 0, m20 = 0, m11 = 0, m02 = 0, m30 = 0, m21 = 0,
         m12 = 0, m03 = 0;
  int x, y;

  for (y = 0; y < IMG_SIZE; y++) {
    for (x = 0; x < IMG_SIZE; x++) {
      double p = (img[y * IMG_SIZE + x] > 0) ? 1.0 : 0.0;
      m00 += p;
      m10 += x * p;
      m01 += y * p;
      m20 += x * x * p;
      m11 += x * y * p;
      m02 += y * y * p;
      m30 += x * x * x * p;
      m21 += x * x * y * p;
      m12 += x * y * y * p;
      m03 += y * y * y * p;
    }
  }

  if (m00 < 1e-10) {
    for (int i = 0; i < 7; i++)
      hu[i] = 0;
    return;
  }

  double xc = m10 / m00, yc = m01 / m00;

  double mu20 = m20 - xc * m10;
  double mu11 = m11 - xc * m01;
  double mu02 = m02 - yc * m01;
  double mu30 = m30 - 3 * xc * m20 + 2 * xc * xc * m10;
  double mu21 = m21 - 2 * xc * m11 - yc * m20 + 2 * xc * xc * m01;
  double mu12 = m12 - 2 * yc * m11 - xc * m02 + 2 * yc * yc * m10;
  double mu03 = m03 - 3 * yc * m02 + 2 * yc * yc * m01;

  double n2 = m00 * m00;
  double n3 = n2 * sqrt(m00);

  double nu20 = mu20 / n2, nu11 = mu11 / n2, nu02 = mu02 / n2;
  double nu30 = mu30 / n3, nu21 = mu21 / n3, nu12 = mu12 / n3, nu03 = mu03 / n3;

  double t0 = nu30 + nu12, t1 = nu21 + nu03;

  hu[0] = (float)(nu20 + nu02);
  hu[1] = (float)((nu20 - nu02) * (nu20 - nu02) + 4 * nu11 * nu11);
  hu[2] = (float)((nu30 - 3 * nu12) * (nu30 - 3 * nu12) +
                  (3 * nu21 - nu03) * (3 * nu21 - nu03));
  hu[3] = (float)(t0 * t0 + t1 * t1);
  hu[4] = (float)((nu30 - 3 * nu12) * t0 * (t0 * t0 - 3 * t1 * t1) +
                  (3 * nu21 - nu03) * t1 * (3 * t0 * t0 - t1 * t1));
  hu[5] = (float)((nu20 - nu02) * (t0 * t0 - t1 * t1) + 4 * nu11 * t0 * t1);
  hu[6] = (float)((3 * nu21 - nu03) * t0 * (t0 * t0 - 3 * t1 * t1) -
                  (nu30 - 3 * nu12) * t1 * (3 * t0 * t0 - t1 * t1));
}

static void Normalize(float *f) {
  for (int i = 0; i < 7; i++) {
    float s = (feat_std[i] < 1e-10f) ? 1.0f : feat_std[i];
    f[i] = (f[i] - feat_mean[i]) / s;
  }
}

static int AI_Init(void) {
  if (stai_network_init((stai_network *)net_ctx) != STAI_SUCCESS)
    return -1;
  if (stai_network_set_activations((stai_network *)net_ctx, act_ptrs, 1) !=
      STAI_SUCCESS)
    return -1;
  if (stai_network_set_inputs((stai_network *)net_ctx, in_ptrs, 1) !=
      STAI_SUCCESS)
    return -1;
  if (stai_network_set_outputs((stai_network *)net_ctx, out_ptrs, 1) !=
      STAI_SUCCESS)
    return -1;
  return 0;
}

static uint8_t ArgMax(float *d, int n) {
  uint8_t m = 0;
  for (int i = 1; i < n; i++)
    if (d[i] > d[m])
      m = i;
  return m;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */
  uint8_t sync, res;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
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
  if (AI_Init() != 0) {
    while (1) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      HAL_Delay(100);
    }
  }
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    if (HAL_UART_Receive(&huart2, &sync, 1, HAL_MAX_DELAY) == HAL_OK &&
        sync == SYNC_BYTE) {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

      if (HAL_UART_Receive(&huart2, img_buf, IMG_BYTES, 5000) == HAL_OK) {
        ComputeHuMoments(img_buf, in_buf);
        Normalize(in_buf);

        if (stai_network_run((stai_network *)net_ctx, STAI_MODE_SYNC) ==
            STAI_SUCCESS) {
          res = ArgMax(out_buf, 10);
        } else {
          res = 0xFF;
        }
        HAL_UART_Transmit(&huart2, &res, 1, 100);
      }
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
   */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
void MX_USART2_UART_Init(void) {

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
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
static void MX_GPIO_Init(void) {
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

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
