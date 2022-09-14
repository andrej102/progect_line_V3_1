/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "event_groups.h"
#include "semphr.h"
#include <stdlib.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SOFT_VER 01010000

#define NUM_PICES_PERIOD 8 // must equal power 2,  = 8, 16, 32, 64 ...
#define MIN_PICE_PERIOD 30


#define LINE_DUMMY 20
#define LINE_SIZE 1536
#define LINE_SIZE_WITH_DUMMY 1556

#define MUX_SIZE 16

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
COMP_HandleTypeDef hcomp1;
COMP_HandleTypeDef hcomp2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

__attribute__((section(".ram_d2"))) uint32_t BufferCOMP1[LINE_SIZE_WITH_DUMMY];
uint32_t BufferCOMP2[LINE_SIZE_WITH_DUMMY];

int8_t last_line[LINE_SIZE];
uint8_t NumObjectsInLastLine = 0;
uint16_t CurrentFrequencyFrame = 112;
uint32_t LCD_show_count_counter = 0;


uint16_t current_line[LINE_SIZE];
uint16_t NumObjectsInCurrentLine = 0;
line_object_t objects_current_line[LINE_SIZE / 2];
line_object_t *p_objects_current_line[LINE_SIZE /2];

uint32_t counter_num_extra_count = 0;
uint32_t numObjects = 0;

line_object_t *p_objects_last_line[LINE_SIZE /2];
line_object_t objects_last_line[LINE_SIZE / 2];

uint16_t Objects_area[1000];
uint32_t num_show_object_area = 0;
uint32_t max_area = 0, midle_area = 0;

uint8_t PositionLCD[8];
uint32_t pices_time[NUM_PICES_PERIOD];
uint32_t system_time = 0;
uint32_t pice_period = 0;

uint8_t start = 0;
uint8_t data_tx_buffer[256];
uint8_t num_data_tx = 0;

uint32_t uart_rx_timeout = 0;
uint8_t uart_rx_buffer[256];
uint16_t uart_rx_buffer_pointer;

#define USE_DEBUG_MODE 3

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_HS_USB_Init(void);
static void MX_COMP1_Init(void);
static void MX_COMP2_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */

xTaskHandle xTaskHandle_Scanner = NULL,
			xTaskHandle_Keyboard = NULL,
			xTaskHandle_ContainerDetect,
			xTaskHandle_LCD,
			xTaskHandle_USART_Service;

/*__attribute__((section(".ITCMRAM_code")))*/ void vTask_Scanner (void *pvParameters);
void vTask_Kyeboard (void *pvParameters);
void vTask_ContainerDetect (void *pvParameters);
void vTask_LCD (void *pvParameters);
void vTask_USART_Service (void *pvParameters);

void UARTsTunning(void);
void ComparatorsTuning(void);
void TimersTuning(void);
void SystemInterruptsTuning(void);
void DMATuning(void);

void Clear_Counter (void);

void ShowTextOnLCD (const char *text);
void ShowNumberOnLCD (uint32_t number, uint16_t num_areas);

void tft_show_nun_pices(uint16_t num_pices);
void tft_show_area_pices(uint16_t num_pice);
void tft_show_area_boundaries(void);
void tft_show_clear_mode(uint8_t on_off);
void tft_show_overcount(uint16_t state);
void tft_show_hide_counter(uint16_t state);

void Delay_us(uint32_t us);

EventGroupHandle_t xEventGroup_StatusFlags;
const EventBits_t Flag_Scanner_Busy =           	0x00000001;
const EventBits_t Flag_Over_Count =           		0x00000002;
const EventBits_t Flag_Mode_Transparent =           0x00000004;
const EventBits_t Flag_USART_TX =           		0x00000008;
const EventBits_t Flag_USART_RX =           		0x00000010;
const EventBits_t Flag_UART_RX_Buffer_Busy = 		0x00000020;
const EventBits_t Flag_Mode_Blue = 					0x00000040;
const EventBits_t Flag_Over_Count_Display =    		0x00000080;
const EventBits_t Flag_Container_Removed =    		0x00000100;
const EventBits_t Flag_Counter_Not_Visible =    	0x00000200;
const EventBits_t Flag_Touch_Key_Poling		 =    	0x00000400;

SemaphoreHandle_t xSemaphoreMutex_Pice_Counter;

uint8_t active_page = 0;

uint8_t num_param = 0;
char param_str[32] = {0};

uint32_t min_area = 25;
float k_1 = 1.85;
float k_2 = 1.85;
uint32_t div_12 = 800;

uint8_t change_num_param = 0;

void tft_show_param(void);
void service_page_0(uint8_t but, uint8_t val);
void service_page_1(uint8_t but, uint8_t val);

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
  //MX_USART3_UART_Init();
  //MX_USB_OTG_HS_USB_Init();
  MX_COMP1_Init();
  MX_COMP2_Init();
  //MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  RCC->AHB2ENR |= RCC_AHB2ENR_AHBSRAM1EN | RCC_AHB2ENR_AHBSRAM2EN;
  RCC->APB4ENR |= RCC_APB4ENR_COMP12EN;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM17_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  UARTsTunning();
  ComparatorsTuning();
  TimersTuning();
  DMATuning();
  SystemInterruptsTuning();

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  xSemaphoreMutex_Pice_Counter = xSemaphoreCreateMutex();

    xTaskCreate(vTask_Scanner,(char*)"Task Scanner", 1024, NULL, tskIDLE_PRIORITY + 4, &xTaskHandle_Scanner);
    xTaskCreate(vTask_Kyeboard,(char*)"Task Keyboard", 512, NULL, tskIDLE_PRIORITY + 3, &xTaskHandle_Keyboard);
 //   xTaskCreate(vTask_ContainerDetect,(char*)"Task Container Detect", 512, NULL, tskIDLE_PRIORITY + 3, &xTaskHandle_ContainerDetect);
    xTaskCreate(vTask_LCD,(char*)"Task LCD", 512, NULL, tskIDLE_PRIORITY + 3, &xTaskHandle_LCD);
    xTaskCreate(vTask_USART_Service,(char*)"USART Service", 512, NULL, tskIDLE_PRIORITY + 3, &xTaskHandle_USART_Service);

    xEventGroup_StatusFlags = xEventGroupCreate();

    TIM17->CR1 |= TIM_CR1_CEN;
    TIM3->CR1 |= TIM_CR1_CEN;

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_CSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 140;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief COMP1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_COMP1_Init(void)
{

  /* USER CODE BEGIN COMP1_Init 0 */

  /* USER CODE END COMP1_Init 0 */

  /* USER CODE BEGIN COMP1_Init 1 */

  /* USER CODE END COMP1_Init 1 */
  hcomp1.Instance = COMP1;
  hcomp1.Init.InvertingInput = COMP_INPUT_MINUS_IO1;
  hcomp1.Init.NonInvertingInput = COMP_INPUT_PLUS_IO2;
  hcomp1.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
  hcomp1.Init.Hysteresis = COMP_HYSTERESIS_NONE;
  hcomp1.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;
  hcomp1.Init.Mode = COMP_POWERMODE_HIGHSPEED;
  hcomp1.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
  hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_NONE;
  if (HAL_COMP_Init(&hcomp1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN COMP1_Init 2 */

  /* USER CODE END COMP1_Init 2 */

}

/**
  * @brief COMP2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_COMP2_Init(void)
{

  /* USER CODE BEGIN COMP2_Init 0 */

  /* USER CODE END COMP2_Init 0 */

  /* USER CODE BEGIN COMP2_Init 1 */

  /* USER CODE END COMP2_Init 1 */
  hcomp2.Instance = COMP2;
  hcomp2.Init.InvertingInput = COMP_INPUT_MINUS_IO1;
  hcomp2.Init.NonInvertingInput = COMP_INPUT_PLUS_IO2;
  hcomp2.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
  hcomp2.Init.Hysteresis = COMP_HYSTERESIS_NONE;
  hcomp2.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;
  hcomp2.Init.Mode = COMP_POWERMODE_HIGHSPEED;
  hcomp2.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
  hcomp2.Init.TriggerMode = COMP_TRIGGERMODE_NONE;
  if (HAL_COMP_Init(&hcomp2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN COMP2_Init 2 */

  /* USER CODE END COMP2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_HS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_HS_USB_Init(void)
{

  /* USER CODE BEGIN USB_OTG_HS_Init 0 */

  /* USER CODE END USB_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB_OTG_HS_Init 1 */

  /* USER CODE END USB_OTG_HS_Init 1 */
  /* USER CODE BEGIN USB_OTG_HS_Init 2 */

  /* USER CODE END USB_OTG_HS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_FS_PWR_EN_GPIO_Port, USB_FS_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_FS_PWR_EN_Pin */
  GPIO_InitStruct.Pin = USB_FS_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_FS_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TIM3_CH1_LINE_ST_Pin */
  GPIO_InitStruct.Pin = TIM3_CH1_LINE_ST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(TIM3_CH1_LINE_ST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TIM3_CH2_LIGTH_Pin */
     GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
     GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
     HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);

     /*Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin */
  	GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
  	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  	GPIO_InitStruct.Pull = GPIO_NOPULL;
  	HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
  	HAL_GPIO_WritePin(TIM3_CH2_LIGHT_BLUE_GPIO_Port, TIM3_CH2_LIGHT_BLUE_Pin, 0);

  /*Configure GPIO pins : LD1_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_FS_OVCR_Pin */
  GPIO_InitStruct.Pin = USB_FS_OVCR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_FS_OVCR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_FS_ID_Pin */
  GPIO_InitStruct.Pin = USB_FS_ID_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
  HAL_GPIO_Init(USB_FS_ID_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_FS_N_Pin USB_FS_P_Pin */
  GPIO_InitStruct.Pin = USB_FS_N_Pin|USB_FS_P_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : TIM17_CH1_LINE_CLK_Pin */
  GPIO_InitStruct.Pin = TIM17_CH1_LINE_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM17;
  HAL_GPIO_Init(TIM17_CH1_LINE_CLK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : S1_Pin */
  GPIO_InitStruct.Pin = S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  // UART1 pin config

  GPIO_InitStruct.Pin = U1_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = U1_RX_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
   GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/*
 *
 */

void vTask_LCD(void *pvParameters)
{
  /* Infinite loop */

	static uint32_t timer_over_count_signal_display = 0;
	static uint32_t timer_counter_flashing_display = 0;

	vTaskDelay(2000);

	for(;;)
	{
		if (!active_page)
		{
			tft_show_nun_pices(numObjects);

#ifdef USE_DEBUG_MODE
			tft_show_area_pices(num_show_object_area);
#endif

			//-----

			if (timer_over_count_signal_display)
			{
				timer_over_count_signal_display--;
				if (!timer_over_count_signal_display)
				{
					tft_show_overcount(0);
				}
			}

			if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Over_Count_Display)
			{
				xEventGroupClearBits( xEventGroup_StatusFlags, Flag_Over_Count_Display);

				if (!timer_over_count_signal_display)
				{
					tft_show_overcount(1);
				}

				timer_over_count_signal_display = 5;
			}

			//------

			if (timer_counter_flashing_display)
			{
				timer_counter_flashing_display--;
			}

			if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Container_Removed)
			{
				if (!timer_counter_flashing_display)
				{
					if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Counter_Not_Visible)
					{
						tft_show_hide_counter(1);
					}
					else
					{
						tft_show_hide_counter(0);
					}

					timer_counter_flashing_display = 5;
				}
			}
			else
			{
				if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Counter_Not_Visible)
				{
					tft_show_hide_counter(1);
				}
			}
		}
		else
		{
			while(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Touch_Key_Poling)
			{
				vTaskDelay(2);
			}

			xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Touch_Key_Poling);
			tft_show_param();
			xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Touch_Key_Poling);

		}

		vTaskDelay(100);
	}

}

/*
 *
 */

void service_page_0(uint8_t but, uint8_t val)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	switch(but)
	{
		case 16 :
		{
			if (val)
			{
				active_page = 1;
				num_param = 0;
				param_str[0] = 0;
				change_num_param = 0;
			}

			break;
		}

		case 2 :
		{
		  if (val)
		  {
			  Clear_Counter();
		  }

		  break;
		}

		case 5 :
		{
		  if (val)
		  {
			  xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Mode_Transparent);
			  Clear_Counter();
		  }
		  else if (!val)
		  {
			  xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Mode_Transparent);
			  Clear_Counter();
		  }

		  break;
		}

		case 4 :
		{
		  if (val)
		  {
			  if(num_show_object_area < (numObjects + 2))
			  {
				  num_show_object_area++;
			  }
			  else
			  {
				  num_show_object_area = 1;
			  }
		  }

		  break;
		}

		case 3 :
		{
		  if (val)
		  {
			  if(num_show_object_area > 1)
			  {
				  num_show_object_area--;
			  }
		  }

		  break;
		}

		case 0x14 :
		{
		  if (val)
		  {
			  xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Mode_Blue);

			  /*Configure GPIO pin : TIM3_CH2_LIGTH_Pin */
				GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
				GPIO_InitStruct.Pull = GPIO_NOPULL;
				HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);
				HAL_GPIO_WritePin(TIM3_CH2_LIGHT_GPIO_Port, TIM3_CH2_LIGHT_Pin, 0);

				 /*Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin */
				GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStruct.Pull = GPIO_NOPULL;
				GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
				GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
				HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
		  }
		  else if (!val)
		  {
			  /*Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin */
				 GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
				 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
				 GPIO_InitStruct.Pull = GPIO_NOPULL;
				 HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
				 HAL_GPIO_WritePin(TIM3_CH2_LIGHT_BLUE_GPIO_Port, TIM3_CH2_LIGHT_BLUE_Pin, 0);

				 /*Configure GPIO pin : TIM3_CH2_LIGTH_Pin */
				GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStruct.Pull = GPIO_NOPULL;
				GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
				GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
				HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);

				xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Mode_Blue);
		  }

		  break;
		}

		default : break;
	}
}

/*
 *
 */

void service_page_1(uint8_t but, uint8_t val)
{
	uint32_t len =0;

	len = strlen(param_str);

	if (num_param)
	{
		switch(but)
		{
			case 4 : if (val) if (len) {param_str[len] = '0'; param_str[len+1] = 0;} break;
			case 5 : if (val) param_str[len] = '1'; param_str[len+1] = 0; break;
			case 12 : if (val) param_str[len] = '2'; param_str[len+1] = 0; break;
			case 13 : if (val) param_str[len] = '3'; param_str[len+1] = 0; break;
			case 14 : if (val) param_str[len] = '4'; param_str[len+1] = 0; break;
			case 15 : if (val) param_str[len] = '5'; param_str[len+1] = 0; break;
			case 16 : if (val) param_str[len] = '6'; param_str[len+1] = 0; break;
			case 17 : if (val) param_str[len] = '7'; param_str[len+1] = 0; break;
			case 18 : if (val) param_str[len] = '8'; param_str[len+1] = 0; break;
			case 19 : if (val) param_str[len] = '9'; param_str[len+1] = 0; break;
			case 21 :
				if (val)
				{
					if(len > 1)
					{
						param_str[len-1] = 0;
					}
					else if(len == 1)
					{
						param_str[len-1] = '0';
					}
				}
				break;
			case 20 : if (val) param_str[len] = '.'; param_str[len+1] = 0; break;

			default : break;
		}
	}

	switch(but)
	{
		case 1 : if (val) active_page = 1; break;
		case 2 : if (val) sprintf(param_str,"%u", min_area); change_num_param = 1; break;
		case 3 : if (val) sprintf(param_str,"%f", k_1); change_num_param = 2; break;
		case 6 : if (val) sprintf(param_str,"%f", k_2); change_num_param = 3; break;
		case 7 : if (val) sprintf(param_str,"%u", div_12); change_num_param = 4; break;
		case 22 :
			if (val)
			{
				switch(num_param)
				{
					case 1 : min_area = atoi(param_str); change_num_param = 1; break;
					case 2 : k_1 = atof(param_str); change_num_param = 2; break;
					case 3 : k_2 = atof(param_str); change_num_param = 3; break;
					case 4 : div_12 = atoi(param_str); change_num_param = 4; break;
					default : break;
				}
			}
			break;

			default : break;
		}

}

/*
 *
 */

/*
 *
 */

void vTask_Kyeboard(void *pvParameters)
{

  for(;;)
  {
	  // defining button events

	  if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_UART_RX_Buffer_Busy)
	  {
		  if (uart_rx_buffer_pointer == 7)
		  {
			  if (uart_rx_buffer[0] == 0x65)
			  {
				  if (uart_rx_buffer[1] == 0x00)
				  {
					  service_page_0(uart_rx_buffer[2], uart_rx_buffer[3]);
				  }
				  else if (uart_rx_buffer[1] == 0x01)
				  {
					  service_page_1(uart_rx_buffer[2], uart_rx_buffer[3]);
				  }
			  }
		  }

		  uart_rx_buffer_pointer = 0;

		  xEventGroupClearBits(xEventGroup_StatusFlags, Flag_UART_RX_Buffer_Busy);
	  }

	  osDelay(100);

	  /*if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_UART_RX_Buffer_Busy)
	  {
		  if (uart_rx_buffer_pointer == 7)
		  {
			  if (uart_rx_buffer[0] == 0x65)
			  {
				  switch(uart_rx_buffer[2])
				  {
				  	  case 2 :
					  {
						  if (uart_rx_buffer[3] == 0x01)
						  {
							  Clear_Counter();
						  }

						  break;
					  }

				  	  case 5 :
					  {
						  if (uart_rx_buffer[3] == 0x01)
						  {
							  xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Mode_Transparent);
							  Clear_Counter();
						  }
						  else if (uart_rx_buffer[3] == 0x00)
						  {
							  xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Mode_Transparent);
							  Clear_Counter();
						  }

						  break;
					  }

				  	  case 4 :
					  {
						  if (uart_rx_buffer[3] == 0x01)
						  {
							  if(num_show_object_area < numObjects)
							  {
								  num_show_object_area++;
							  }
							  else
							  {
								  num_show_object_area = 0;
							  }
						  }

						  break;
					  }

				  	  case 3 :
					  {
						  if (uart_rx_buffer[3] == 0x01)
						  {
							  if(num_show_object_area > 0)
							  {
								  num_show_object_area--;
							  }
						  }

						  break;
					  }

				  	  case 0x14 :
					  {
						  if (uart_rx_buffer[3] == 0x01)
						  {
							  xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Mode_Blue);

							  //Configure GPIO pin : TIM3_CH2_LIGTH_Pin
								GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
								GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
								GPIO_InitStruct.Pull = GPIO_NOPULL;
								HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);
								HAL_GPIO_WritePin(TIM3_CH2_LIGHT_GPIO_Port, TIM3_CH2_LIGHT_Pin, 0);

							     //Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin
								GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
								GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
								GPIO_InitStruct.Pull = GPIO_NOPULL;
								GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
								GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
								HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
						  }
						  else if (uart_rx_buffer[3] == 0x00)
						  {
							  //Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin
								 GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
								 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
								 GPIO_InitStruct.Pull = GPIO_NOPULL;
								 HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
								 HAL_GPIO_WritePin(TIM3_CH2_LIGHT_BLUE_GPIO_Port, TIM3_CH2_LIGHT_BLUE_Pin, 0);

								//Configure GPIO pin : TIM3_CH2_LIGTH_Pin								GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
								GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
								GPIO_InitStruct.Pull = GPIO_NOPULL;
								GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
								GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
								HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);

								xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Mode_Blue);
						  }

						  break;
					  }

				  	  default : break;
				  }
			  }
		  }

		  uart_rx_buffer_pointer = 0;

		  xEventGroupClearBits(xEventGroup_StatusFlags, Flag_UART_RX_Buffer_Busy);
	  }

	  osDelay(100);*/
  }
}

/*
 *
 */

void vTask_ContainerDetect(void *pvParameters)
{
	static uint8_t previous_state = 1, event_state = 0, timer_over_count_signal = 0;

	/* Infinite loop */
  for(;;)
  {
	 /* if (COMP12->SR & COMP_SR_C2VAL)
	  {

	  }

	  if (HAL_GPIO_ReadPin(CONTAINER_DETECT_GPIO_Port, CONTAINER_DETECT_Pin))
	  {
		  if(previous_state)
		  {
			  if(!event_state)
			  {
				  xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Container_Removed);
				  event_state = 1;

				  Clear_Counter();
			  }
		  }
		  else
		  {
			  event_state = 0;
		  }

		  previous_state = 1;
	  }
	  else
	  {
		  if(!previous_state)
		  {
			  if(!event_state)
			  {
				  xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Container_Removed);
				  event_state = 1;
			  }
		  }
		  else
		  {
			  event_state = 0;
		  }

		  previous_state = 0;
	  }

	 // Over count service

	  if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Over_Count)
	  	  {
	  		  xEventGroupClearBits( xEventGroup_StatusFlags, Flag_Over_Count);
	  		  timer_over_count_signal = 5;
	  		  OVER_COUNT_GPIO_Port->BSRR |=  OVER_COUNT_Pin; // Set OVER CURRENT
	  	  }

	  	  if (timer_over_count_signal)
	  	  {
	  		  timer_over_count_signal--;
	  		  if (!timer_over_count_signal)
	  		  {
	  			  OVER_COUNT_GPIO_Port->BSRR |=  (OVER_COUNT_Pin << 16); // Clear OVER CURRENT
	  		  }
	  	  }*/

	  osDelay(100);
  }

}

/**
  * @brief  Function implementing the Scaner Task thread.
  * @param  argument: Not used
  * @retval None
  */
void vTask_Scanner(void *pvParameters)
{
	uint32_t j, p, i;
	uint8_t lastbit = 0;
	uint32_t numObjects_temp =0;
	uint8_t transparent_object_start = 0;
	uint32_t transparent_object_overtime = 0;
	uint32_t transparent_object_current_line_overtime = 0;

	/* Infinite loop */
	for(;;)
	{

		xEventGroupWaitBits( xEventGroup_StatusFlags, Flag_Scanner_Busy, pdFALSE, pdFALSE, portMAX_DELAY );

		HAL_GPIO_WritePin(S1_GPIO_Port, S1_Pin, 1);

		if (!active_page)
		{
			for (uint32_t y=LINE_DUMMY, z=0; y < (LINE_SIZE + LINE_DUMMY); y +=4, z +=1)
			{
				BufferCOMP2[z] = BufferCOMP1[y] ;
			}


			NumObjectsInCurrentLine = 0;
			lastbit = 0;

			for (j = 0; j < (LINE_SIZE/4); j++)
			{
				if(BufferCOMP2[j] & COMP_SR_C1VAL)
				{
					current_line[j] = 0;
					lastbit = 0;
				}
				else
				{
					if(!lastbit)
					{
						NumObjectsInCurrentLine++;

						p_objects_current_line[NumObjectsInCurrentLine-1] = &objects_current_line[NumObjectsInCurrentLine-1];

						p_objects_current_line[NumObjectsInCurrentLine-1]->area = 0;
					}

					current_line[j] = NumObjectsInCurrentLine;
					p_objects_current_line[NumObjectsInCurrentLine-1]->area++;
					lastbit = 1;

					if(last_line[j])
					{
						p_objects_last_line[last_line[j]-1]->cont = 1;

						if (!p_objects_last_line[last_line[j]-1]->sl)
						{
							p_objects_last_line[last_line[j]-1]->sl = current_line[j];
							p_objects_current_line[current_line[j]-1]->area += p_objects_last_line[last_line[j]-1]->area;
						}
						else
						{
							if (p_objects_current_line[p_objects_last_line[last_line[j]-1]->sl - 1] != p_objects_current_line[current_line[j]-1])
							{
								p_objects_current_line[p_objects_last_line[last_line[j]-1]->sl-1]->area += p_objects_current_line[current_line[j]-1]->area;

								p_objects_current_line[current_line[j]-1] = p_objects_current_line[p_objects_last_line[last_line[j]-1]->sl - 1];
							}
						}
					}
				}

				// we analyze the connectivity of the objects of the current line with the objects of the previous line
				// and arrange the corresponding signs (connectivity and continuation)

				last_line[j] = current_line[j];
			}

			if (xSemaphoreTake(xSemaphoreMutex_Pice_Counter, 1) == pdTRUE)
			{
				// scan result analysis

				if (xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_Mode_Transparent)
				{
					// Transparent mode

					if (!transparent_object_start)
					{
						if (NumObjectsInCurrentLine)
						{
							transparent_object_start = 1;
							Objects_area[numObjects] = 0;
							transparent_object_overtime = HAL_GetTick();
							transparent_object_current_line_overtime = HAL_GetTick();
						}
					}

					if(transparent_object_start)
					{
						if ((HAL_GetTick() - transparent_object_overtime > TRANSPARENT_OBJECT_OVERTIME) || ((HAL_GetTick() - transparent_object_current_line_overtime > TRANSPARENT_OBJECT_CURRENT_LINE_OVERTIME)))
						{
							transparent_object_start = 0;
							numObjects++;
						}
						else
						{
							if (NumObjectsInCurrentLine)
							{
								transparent_object_current_line_overtime = HAL_GetTick();

								for(j = 0; j < NumObjectsInCurrentLine; j++)
								{
									Objects_area[numObjects] += p_objects_current_line[j]->area;
								}
							}
						}
					}
				}
				else
				{
							// Common mode

					// check if there are completed objects on the previous line

					line_object_t * previous_p_objects_last_line = NULL;

					for (j=0; j < NumObjectsInLastLine; j++)
					{
						if (!p_objects_last_line[j]->cont)
						{
							if (p_objects_last_line[j] == previous_p_objects_last_line)
							{
								continue;
							}

							numObjects_temp = numObjects;

							if(p_objects_last_line[j]->area < 3000)
							{
								if (max_area)
								{
									while (p_objects_last_line[j]->area)
									{
										if (p_objects_last_line[j]->area > max_area)
										{
											//Objects_area[numObjects] = max_area/2;
											//p_objects_last_line[j]->area -= (max_area/2);
											Objects_area[numObjects] = max_area;
											p_objects_last_line[j]->area -= max_area;
											numObjects++;
										}
										else if (p_objects_last_line[j]->area < min_area)
										{
											p_objects_last_line[j]->area = 0;
										}
										else
										{
											Objects_area[numObjects] = p_objects_last_line[j]->area;
											p_objects_last_line[j]->area = 0;
											numObjects++;
										}

									}
								}
								else
								{
									if (p_objects_last_line[j]->area < min_area)
									{
										p_objects_last_line[j]->area = 0;
									}
									else
									{
										Objects_area[numObjects] = p_objects_last_line[j]->area;
										p_objects_last_line[j]->area = 0;
										numObjects++;
									}
								}

								if (numObjects_temp != numObjects)
								{
									for (p=1; p < NUM_PICES_PERIOD; p++)
									{
										pices_time[p-1] = pices_time[p];
									}

									pices_time[NUM_PICES_PERIOD - 1]  = HAL_GetTick();

									if (numObjects > (NUM_PICES_PERIOD - 1))
									{
										pice_period = (pices_time[NUM_PICES_PERIOD - 1] - pices_time[0]) / NUM_PICES_PERIOD;
										if (pice_period < MIN_PICE_PERIOD)
										{
											counter_num_extra_count++;
											xEventGroupSetBits( xEventGroup_StatusFlags, Flag_Over_Count | Flag_Over_Count_Display);

											for (p=0; p < NUM_PICES_PERIOD; p++)
											{
												pices_time[p] = 0;
											}
										}
									}
								}
							}
							else
							{
								p_objects_last_line[j]->area = 0;
							}

							previous_p_objects_last_line = p_objects_last_line[j];
						}

						if (numObjects == 10)
						{
							midle_area = 0;

							for (i=0;i<10;i++)
							{
								midle_area += Objects_area[i];
							}
							midle_area /=10;

							if(midle_area < div_12)
							{
								max_area = midle_area * k_1;
							}
							else
							{
								max_area = midle_area * k_2;
							}
						}
					}
				}

				if(numObjects > 1000)
				{
					numObjects = 0;
				}

				xSemaphoreGive(xSemaphoreMutex_Pice_Counter);
			}

			// перено�?им объекты текущей линии в предыдущую

			for (j=0; j < NumObjectsInCurrentLine; j++)
			{
				p_objects_last_line[j] = &objects_last_line[0] + (p_objects_current_line[j] - &objects_current_line[0]);

				p_objects_last_line[j]->area = p_objects_current_line[j]->area;
				p_objects_last_line[j]->cont = 0;
				p_objects_last_line[j]->sl = 0;
			}

			NumObjectsInLastLine = NumObjectsInCurrentLine;
		}

		xEventGroupClearBits( xEventGroup_StatusFlags, Flag_Scanner_Busy);

		HAL_GPIO_WritePin(S1_GPIO_Port, S1_Pin, 0);


		taskYIELD();
  }

}

/*
 *
 */

void vTask_USART_Service (void *pvParameters)
{
	uint32_t tx_timeout = 0;
	uint8_t num_data_send = 0;

	/* Infinite loop */
	for(;;)
	{
		xEventGroupWaitBits(xEventGroup_StatusFlags, Flag_USART_TX | Flag_USART_RX, pdFALSE, pdFALSE, portMAX_DELAY );

		if(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_RX)
		{

			xEventGroupClearBits(xEventGroup_StatusFlags, Flag_USART_RX);
		}

		if(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX)
		{
			tx_timeout = xTaskGetTickCount();
			num_data_send = 0;

			while(num_data_tx)
			{
				vTaskDelay(1);

				while ((USART1->ISR & USART_ISR_TXE_TXFNF) && num_data_tx)
				{
					USART1->TDR = *(data_tx_buffer + num_data_send);
					num_data_tx--;
					num_data_send++;

					while(!(USART1->ISR & USART_ISR_TC))
					{
						vTaskDelay(1);
					}

					USART1->ICR |= USART_ICR_TCCF;
				}


			}

			/*while(!(USART1->ISR & USART_ISR_TC))
			{
				vTaskDelay(1);
			}*/


			xEventGroupClearBits(xEventGroup_StatusFlags, Flag_USART_TX);
		}
	}
}

/*
 *
 */

void add_end_command(uint8_t * data, uint8_t * init_index)
{
	uint8_t index = *init_index;

	index += strlen(data + index);
	data[index] = 0xff;
	data_tx_buffer[index + 1] = 0xff;
	data_tx_buffer[index + 2] = 0xff;
	index +=3;

	*init_index = index;
}

/*
 *
 */

void tft_show_param(void)
{
	uint32_t protect_counter = HAL_GetTick();

	while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
	{
		vTaskDelay(10);
	}

	if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
	{
		num_data_tx =0;

		if ((change_num_param > 0) && (change_num_param < 5))
		{
			if(num_param)
			{
				if(num_param != change_num_param)
				{// деактивируем текущее поле и активируем новое
					switch(num_param)
					{
						case 1 : sprintf(data_tx_buffer + num_data_tx, "page1.n0.bco=65535"); break;
						case 2 : sprintf(data_tx_buffer + num_data_tx, "page1.n2.bco=65535"); break;
						case 3 : sprintf(data_tx_buffer + num_data_tx, "page1.n3.bco=65535"); break;
						case 4 : sprintf(data_tx_buffer + num_data_tx, "page1.n4.bco=65535"); break;
					}
					add_end_command(data_tx_buffer, &num_data_tx);

					switch(change_num_param)
					{
						case 1 : sprintf(data_tx_buffer + num_data_tx, "page1.n0.bco=65504"); break;
						case 2 : sprintf(data_tx_buffer + num_data_tx, "page1.n2.bco=65504"); break;
						case 3 : sprintf(data_tx_buffer + num_data_tx, "page1.n3.bco=65504"); break;
						case 4 : sprintf(data_tx_buffer + num_data_tx, "page1.n4.bco=65504"); break;
					}
					add_end_command(data_tx_buffer, &num_data_tx);

					num_param = change_num_param;
				}
				else // если деактивируем поле
				{
					switch(change_num_param)
					{
						case 1 : sprintf(data_tx_buffer + num_data_tx, "page1.n0.bco=65535"); break;
						case 2 : sprintf(data_tx_buffer + num_data_tx, "page1.n2.bco=65535"); break;
						case 3 : sprintf(data_tx_buffer + num_data_tx, "page1.n3.bco=65535"); break;
						case 4 : sprintf(data_tx_buffer + num_data_tx, "page1.n4.bco=65535"); break;
					}
					add_end_command(data_tx_buffer, &num_data_tx);

					num_param = 0;
				}
			}
			else // если небыло активно поле, то активируем
			{
				switch(change_num_param)
				{
					case 1 : sprintf(data_tx_buffer + num_data_tx, "page1.n0.bco=65504"); break;
					case 2 : sprintf(data_tx_buffer + num_data_tx, "page1.n2.bco=65504"); break;
					case 3 : sprintf(data_tx_buffer + num_data_tx, "page1.n3.bco=65504"); break;
					case 4 : sprintf(data_tx_buffer + num_data_tx, "page1.n4.bco=65504"); break;
				}
				add_end_command(data_tx_buffer, &num_data_tx);

				num_param = change_num_param;
			}

			change_num_param = 0;
		}

		if(num_param == 1) sprintf(data_tx_buffer + num_data_tx, "page1.n0.val=%s", param_str); else sprintf(data_tx_buffer + num_data_tx, "page1.n0.val=%d", min_area);
		add_end_command(data_tx_buffer, &num_data_tx);
		if(num_param == 2) sprintf(data_tx_buffer + num_data_tx, "page1.n2.val=%s", param_str); else sprintf(data_tx_buffer + num_data_tx, "page1.n2.val=%f", k_1);
		add_end_command(data_tx_buffer, &num_data_tx);
		if(num_param == 3) sprintf(data_tx_buffer + num_data_tx, "page1.n3.val=%s", param_str); else sprintf(data_tx_buffer + num_data_tx, "page1.n3.val=%f", k_2);
		add_end_command(data_tx_buffer, &num_data_tx);
		if(num_param == 4) sprintf(data_tx_buffer + num_data_tx, "page1.n4.val=%s", param_str); else sprintf(data_tx_buffer + num_data_tx, "page1.n4.val=%d", div_12);
		add_end_command(data_tx_buffer, &num_data_tx);

		xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
	}
}

/*
 *
 */

void tft_show_clear_mode(uint8_t on_off)
{

}

/*
 *
 */

void tft_show_overcount(uint16_t state)
{
	uint32_t protect_counter = HAL_GetTick();

	while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
	{
		vTaskDelay(10);
	}

	if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
	{
		if(state)
		{
			sprintf(data_tx_buffer, "page0.n0.bco=63488");
		}
		else
		{
			sprintf(data_tx_buffer, "page0.n0.bco=65520");
		}
		num_data_tx = strlen(data_tx_buffer);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;
		xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
	}

	if(state)
	{
		while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
		{
			vTaskDelay(10);
		}

		if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
		{
			sprintf(data_tx_buffer, "page0.wav0.en=1");

			num_data_tx = strlen(data_tx_buffer);
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0x00;
			xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
		}

	}
}

/*
 *
 */

void tft_show_hide_counter(uint16_t state)
{
	uint32_t protect_counter = HAL_GetTick();

	while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
	{
		vTaskDelay(10);
	}

	if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
	{
		if(state)
		{
			sprintf(data_tx_buffer, "page0.n0.pco=0");
			xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Counter_Not_Visible);
		}
		else
		{
			sprintf(data_tx_buffer, "page0.n0.pco=65520");
			xEventGroupSetBits(xEventGroup_StatusFlags, Flag_Counter_Not_Visible);
		}
		num_data_tx = strlen(data_tx_buffer);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;
		xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
	}
}

/*
 *
 */

void tft_show_nun_pices(uint16_t num_pices)
{
	uint32_t protect_counter = HAL_GetTick();

	while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
	{
		vTaskDelay(10);
	}

	if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
	{
		sprintf(data_tx_buffer, "page0.n0.val=%d", num_pices);
		num_data_tx = strlen(data_tx_buffer);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;
		xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
	}
}

void tft_show_area_pices(uint16_t num_pice)
{
	uint32_t protect_counter = HAL_GetTick();

	while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
	{
		vTaskDelay(10);
	}

	if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
	{
		if(!num_pice)
		{
			sprintf(data_tx_buffer, "page0.n1.val=%d", 0);
		}
		else
		{
			sprintf(data_tx_buffer, "page0.n1.val=%d", Objects_area[num_pice-1]);
		}
		num_data_tx = strlen(data_tx_buffer);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		sprintf(data_tx_buffer + num_data_tx, "page0.n2.val=%d", num_pice);
		num_data_tx += strlen(data_tx_buffer + num_data_tx);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;
		sprintf(data_tx_buffer + num_data_tx, "page0.n3.val=%d", max_area);
		num_data_tx += strlen(data_tx_buffer + num_data_tx);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;
		sprintf(data_tx_buffer + num_data_tx, "page0.n4.val=%d", midle_area);
		num_data_tx += strlen(data_tx_buffer + num_data_tx);
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0xff;
		num_data_tx++;
		data_tx_buffer[num_data_tx] = 0x00;

		xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
	}
}

void tft_show_area_boundaries(void)
{

}


/*
 * *************** General Purpose Functions ***************
 */

void Clear_Counter (void)
{
	uint8_t p;
	GPIO_InitTypeDef GPIO_InitStruct = {0};

 	if (xSemaphoreTake(xSemaphoreMutex_Pice_Counter, 10) == pdTRUE)
	{
		counter_num_extra_count = 0;
		numObjects = 0;
		num_show_object_area = 0;
		max_area = 0;
		midle_area = 0;

		for (p=0; p < NUM_PICES_PERIOD; p++)
		{
			pices_time[p] = 0;
		}

		xSemaphoreGive(xSemaphoreMutex_Pice_Counter);

		 /*Configure GPIO pin : TIM3_CH2_LIGTH_BLUE_Pin */
		 GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_BLUE_Pin;
		 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		 GPIO_InitStruct.Pull = GPIO_NOPULL;
		 HAL_GPIO_Init(TIM3_CH2_LIGHT_BLUE_GPIO_Port, &GPIO_InitStruct);
		 HAL_GPIO_WritePin(TIM3_CH2_LIGHT_BLUE_GPIO_Port, TIM3_CH2_LIGHT_BLUE_Pin, 0);

		 /*Configure GPIO pin : TIM3_CH2_LIGTH_Pin */
		GPIO_InitStruct.Pin = TIM3_CH2_LIGHT_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
		HAL_GPIO_Init(TIM3_CH2_LIGHT_GPIO_Port, &GPIO_InitStruct);

		xEventGroupClearBits(xEventGroup_StatusFlags, Flag_Mode_Blue);

		uint32_t protect_counter = HAL_GetTick();

		while ((xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX) && ((HAL_GetTick() - protect_counter) < 1000))
		{
			vTaskDelay(10);
		}

		if (!(xEventGroupGetBits(xEventGroup_StatusFlags) & Flag_USART_TX))
		{
			sprintf(data_tx_buffer, "page0.bt1.val=0");
			num_data_tx = strlen(data_tx_buffer);
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0xff;
			num_data_tx++;
			data_tx_buffer[num_data_tx] = 0x00;
			xEventGroupSetBits(xEventGroup_StatusFlags, Flag_USART_TX);
		}

	}
}

/*
 *
 */

void TimersTuning(void)
{
    TIM3->PSC = 279;
    TIM3->ARR = 400;
    TIM3->CCR1 = 50;
    TIM3->CCR2 = 60;
    TIM3->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC1FE | TIM_CCMR1_OC2FE;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->DIER = TIM_DIER_CC1IE;

    TIM17->PSC = 0;
    TIM17->ARR = 56;
    TIM17->CCR1 = 31;
    TIM17->CCMR1 = TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1FE;
    TIM17->CCER = TIM_CCER_CC1E;
    TIM17->BDTR = TIM_BDTR_MOE;
    TIM17->DIER = TIM_DIER_CC1DE;
}

/*
 *
 */

void UARTsTunning(void)
{
	USART1->BRR = 14583; 			//  140 MHz / 9600 bout  = 10000
	USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE| USART_CR1_FIFOEN | USART_CR1_RXNEIE_RXFNEIE /*| TCIE*/;
}

/*
 *
 */

void ComparatorsTuning(void)
{
    COMP1->CFGR = COMP_CFGRx_EN | COMP_CFGRx_HYST_1 | COMP_CFGRx_INMSEL_1 | COMP_CFGRx_INMSEL_2 | COMP_CFGRx_INPSEL ; //COMP1 enable , HighSpeed, Medium hysteresis, PB2 +, P1 -
    COMP2->CFGR = COMP_CFGRx_EN | COMP_CFGRx_HYST_1 | COMP_CFGRx_INMSEL_1 | COMP_CFGRx_INMSEL_2 | COMP_CFGRx_INPSEL ; //COMP1 enable , HighSpeed, Medium hysteresis, PB2 +, P1 -
}

/*
 *
 */

void SystemInterruptsTuning(void)
{


    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, 10);

    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 11);

    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    NVIC_SetPriority(DMA1_Stream0_IRQn, 10);
}

/*
 *
 */

void DMATuning(void)
{
    // for change addresses DMA chanal must disable !!!

	DMA1_Stream0->PAR = (uint32_t)&COMP12->SR;
	DMA1_Stream0->M0AR = (uint32_t)BufferCOMP1;
	DMA1_Stream0->NDTR = LINE_SIZE_WITH_DUMMY;
	DMA1_Stream0->CR = DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_TCIE | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
	DMAMUX1_Channel0->CCR = ( 111 << DMAMUX_CxCR_DMAREQ_ID_Pos);
}

/*
 *
 */

void TIM3_IRQHandler(void)
{
	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM3->SR &= ~TIM_SR_CC1IF;

    DMA1_Stream0->NDTR = LINE_SIZE_WITH_DUMMY;
    DMA1_Stream0->CR |= DMA_SxCR_EN;
    TIM3->CR1 |= TIM_CR1_CEN;
}

/*
 *
 *
 */

void DMA1_Stream0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

    DMA1->LIFCR |= DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0;

    xHigherPriorityTaskWoken = pdFALSE;

    if (!(xEventGroupGetBitsFromISR(xEventGroup_StatusFlags) & Flag_Scanner_Busy))
    {
		xResult = xEventGroupSetBitsFromISR(xEventGroup_StatusFlags, Flag_Scanner_Busy, &xHigherPriorityTaskWoken);

		if( xResult != pdFAIL )
		{
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
    }
}

/*
 *
 */

void USART1_IRQHandler(void)
{
	uint8_t temp_rx_variable;
	BaseType_t xHigherPriorityTaskWoken= pdFALSE;

	if (USART1->ISR & USART_ISR_RXNE_RXFNE)
	{
		if (!(xEventGroupGetBitsFromISR(xEventGroup_StatusFlags) & Flag_UART_RX_Buffer_Busy))
		{
			if (!uart_rx_timeout)
			{
				uart_rx_timeout = 1;
				uart_rx_buffer_pointer = 0;
			}

			while(USART1->ISR & USART_ISR_RXNE_RXFNE)
			{
				uart_rx_buffer[uart_rx_buffer_pointer] = USART1->RDR;

				if (uart_rx_buffer[uart_rx_buffer_pointer] == 0xff)
				{
					uart_rx_timeout++;
				}

				if (uart_rx_buffer_pointer < sizeof(uart_rx_buffer))
				{
					uart_rx_buffer_pointer++;
				}
			}

			if (uart_rx_timeout >= 4)
			{
				uart_rx_timeout = 0;
				xEventGroupSetBitsFromISR(xEventGroup_StatusFlags, Flag_UART_RX_Buffer_Busy, &xHigherPriorityTaskWoken);
			}
		}
		else
		{
			while(USART1->ISR & USART_ISR_RXNE_RXFNE)
			{
				temp_rx_variable = USART1->RDR;
			}
		}

	}
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
