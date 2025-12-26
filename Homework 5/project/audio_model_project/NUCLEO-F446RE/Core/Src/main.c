/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Keyword Spotting via UART
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
#include "stai.h"
#include "network.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYNC_BYTE           0xAA
#define INPUT_SIZE          260    /* 13 MFCC coefficients * 20 frames */
#define INPUT_SIZE_BYTES    1040   /* 260 * 4 bytes (float32) */
#define OUTPUT_SIZE         10     /* 10 classes (digits 0-9) */
#define OUTPUT_SIZE_BYTES   40     /* 10 * 4 bytes (float32) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Network context */
STAI_ALIGNED(32) static uint8_t network_context[STAI_NETWORK_CONTEXT_SIZE];

/* Activation buffer */
STAI_ALIGNED(32) static uint8_t activations[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];
static stai_ptr activation_buffers[] = { activations };

/* Input/Output buffers */
STAI_ALIGNED(32) static float input_buffer[INPUT_SIZE];
STAI_ALIGNED(32) static float output_buffer[OUTPUT_SIZE];
static stai_ptr input_ptrs[] = { (stai_ptr)input_buffer };
static stai_ptr output_ptrs[] = { (stai_ptr)output_buffer };

/* State buffer (if needed) */
STAI_ALIGNED(32) static uint8_t states_buffer[4];
static stai_ptr state_ptrs[] = { states_buffer };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static int AI_Init(void);
static int AI_Run(void);
static uint8_t FindArgMax(float *data, int size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Initialize the neural network
  * @retval 0 on success, -1 on error
  */
static int AI_Init(void)
{
    stai_return_code ret;
    
    /* Initialize network context */
    ret = stai_network_init((stai_network*)network_context);
    if (ret != STAI_SUCCESS) {
        return -1;
    }
    
    /* Set activation buffers */
    ret = stai_network_set_activations((stai_network*)network_context, 
                                       activation_buffers, 
                                       STAI_NETWORK_ACTIVATIONS_NUM);
    if (ret != STAI_SUCCESS) {
        return -1;
    }
    
    /* Set state buffers */
    ret = stai_network_set_states((stai_network*)network_context,
                                  state_ptrs,
                                  1);
    if (ret != STAI_SUCCESS) {
        /* States might not be needed, continue anyway */
    }
    
    /* Set input buffers */
    ret = stai_network_set_inputs((stai_network*)network_context, 
                                  input_ptrs, 
                                  STAI_NETWORK_IN_NUM);
    if (ret != STAI_SUCCESS) {
        return -1;
    }
    
    /* Set output buffers */
    ret = stai_network_set_outputs((stai_network*)network_context, 
                                   output_ptrs, 
                                   STAI_NETWORK_OUT_NUM);
    if (ret != STAI_SUCCESS) {
        return -1;
    }
    
    return 0;
}

/**
  * @brief Run neural network inference
  * @retval 0 on success, -1 on error
  */
static int AI_Run(void)
{
    stai_return_code ret;
    
    ret = stai_network_run((stai_network*)network_context, STAI_MODE_SYNC);
    if (ret != STAI_SUCCESS) {
        return -1;
    }
    
    return 0;
}

/**
  * @brief Find the index of the maximum value in an array
  * @param data Pointer to float array
  * @param size Size of array
  * @retval Index of maximum value
  */
static uint8_t FindArgMax(float *data, int size)
{
    uint8_t max_idx = 0;
    float max_val = data[0];
    
    for (int i = 1; i < size; i++) {
        if (data[i] > max_val) {
            max_val = data[i];
            max_idx = i;
        }
    }
    
    return max_idx;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    uint8_t sync_byte;
    uint8_t result;
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
    /* Initialize the neural network */
    if (AI_Init() != 0) {
        /* Initialization failed - blink LED rapidly */
        while (1) {
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            HAL_Delay(100);
        }
    }
    
    /* LED on to indicate ready */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* Wait for sync byte */
        if (HAL_UART_Receive(&huart2, &sync_byte, 1, HAL_MAX_DELAY) == HAL_OK) {
            if (sync_byte == SYNC_BYTE) {
                /* Turn off LED during processing */
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
                
                /* Receive input features (260 float32 = 1040 bytes) */
                if (HAL_UART_Receive(&huart2, (uint8_t*)input_buffer, INPUT_SIZE_BYTES, 5000) == HAL_OK) {
                    /* Run inference */
                    if (AI_Run() == 0) {
                        /* Find predicted digit */
                        result = FindArgMax(output_buffer, OUTPUT_SIZE);
                        
                        /* Send result back */
                        HAL_UART_Transmit(&huart2, &result, 1, 100);
                    } else {
                        /* Inference failed - send 0xFF */
                        result = 0xFF;
                        HAL_UART_Transmit(&huart2, &result, 1, 100);
                    }
                }
                
                /* Turn LED back on */
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
            }
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
void SystemClock_Config(void)
{
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Activate the Over-Drive mode
    */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
    huart2.Init.BaudRate = 115200;
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
