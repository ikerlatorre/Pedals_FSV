/* USER CODE BEGIN Header */
/**
 * Pedals
 *
 * IN1 -> ACC 2
 * IN2 -> ACC 1
 * IN3 -> VOL 2
 * IN4 -> VOL 1
 * IN5 -> BRK 2
 * IN6 -> BRK 1
 *
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//#include "miPrueba.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct misDatosPedales{
	uint8_t	acelerador;
	uint8_t	freno;
	uint8_t direccion;
}misDatosPedales;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SPI_Timeout 10
#define ID_msgPedals 0x0C
#define CAN_length	3
#define VCC	4.7
#define Vmax1	3500
#define Vmin1	700
#define Vmax2	4100
#define Vmin2	1600


#define	vPres0	550
#define	vPres1	4600
#define iPres0	0
#define	iPres1	160
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
volatile uint8_t miTimer;

volatile uint16_t vACC_1, vACC_2, vVOL_1, vVOL_2, vBRK_1, vBRK_2;
volatile uint8_t pBRK_1, pBRK_2, pACC_1, pACC_2, pVOL_1, pVOL_2;
volatile uint8_t reparto;
volatile uint8_t acelerador, freno;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
uint8_t mapValores(uint16_t entrada, uint16_t v0, uint16_t v1, uint16_t y0, uint16_t y1);
uint8_t checkRedundancy(uint8_t in_1, uint8_t in_2);
uint16_t miVoltaje(uint8_t* datos);
uint8_t porcentaje(uint8_t in1, uint8_t in2);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
CAN_TxHeaderTypeDef TxCAN_Pedals;
uint8_t BufferTxCAN_Pedals[CAN_length];
uint32_t txMailBox;

CAN_RxHeaderTypeDef RxCAN;
CAN_FilterTypeDef sFilterConfig;
uint8_t BufferRxCAN[8];

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
  MX_CAN_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  //Configurar filtro TX CAN
  TxCAN_Pedals.IDE	= CAN_ID_STD;//11 bits for ID
  TxCAN_Pedals.RTR	= CAN_RTR_DATA; //Data frame
  TxCAN_Pedals.StdId= ID_msgPedals; //Msg ID
  TxCAN_Pedals.DLC	= CAN_length; //Number of bytes to TX
  TxCAN_Pedals.TransmitGlobalTime = DISABLE;

  //Configurar filtro RX ToDo
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale= CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0<<5; //Mirar manual
  sFilterConfig.FilterIdLow = 0xFF;
  sFilterConfig.FilterMaskIdHigh = 0xFF;
  sFilterConfig.FilterMaskIdLow = 0xFF;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 0 o 1
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.FilterBank = 0; //De 0 a 13

  HAL_CAN_ConfigFilter(&hcan,&sFilterConfig);

  HAL_GPIO_WritePin(SPI_CS1_GPIO_Port, SPI_CS1_Pin, 1);
  uint8_t spiTx[2];
  uint8_t spiRx[2];
  misDatosPedales	pasarAcan;
  uint16_t misDatos[6];

  /*
  uint16_t vACC_1, vACC_2, vVOL_1, vVOL_2, vBRK_1, vBRK_2;
  uint8_t pBRK_1, pBRK_2, pACC_1, pACC_2, pVOL_1, pVOL_2;
*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  spiTx[1] = 0;
  miTimer = 0;
  HAL_CAN_Start(&hcan);
  HAL_TIM_Base_Start_IT(&htim2);

  while (1)
  {
	  if(miTimer){
		  //Ask for voltages
		  for(uint8_t iteracion = 0; iteracion < 6; iteracion++){
			  spiTx[0] = iteracion << 3;
			  HAL_GPIO_WritePin(SPI_CS1_GPIO_Port, SPI_CS1_Pin, 0);
			  HAL_SPI_TransmitReceive(&hspi1, spiTx, spiRx, 2, SPI_Timeout);
			  HAL_GPIO_WritePin(SPI_CS1_GPIO_Port, SPI_CS1_Pin, 1);
			  misDatos[iteracion] = miVoltaje(spiRx);
			  HAL_Delay(1);
		  }
		  HAL_GPIO_WritePin(SPI_CS1_GPIO_Port, SPI_CS1_Pin, 0);
		  HAL_SPI_TransmitReceive(&hspi1, spiTx, spiRx, 2, SPI_Timeout);
		  HAL_GPIO_WritePin(SPI_CS1_GPIO_Port, SPI_CS1_Pin, 1);
		  misDatos[0] = miVoltaje(spiRx);

		  vACC_1 = misDatos[1];
		  vACC_2 = misDatos[2];

		  //vVOL_2 = misDatos[3];
		  //vVOL_1 = misDatos[4];

		  vBRK_2 = misDatos[5];
		  vBRK_1 = misDatos[0];

		  pACC_1 = mapValores(vACC_1, Vmin1, Vmax1, 0, 100);
		  pACC_2 = mapValores(vACC_2, Vmin2, Vmax2, 0, 100);

		  pVOL_1 = mapValores(vVOL_1, 0, 5000, 0, 100);
		  pVOL_2 = mapValores(vVOL_2, 0, 5000, 0, 100);

		  pBRK_1 = mapValores(vBRK_1, vPres0, vPres1, iPres0, iPres1);
		  pBRK_2 = mapValores(vBRK_2, vPres0, vPres1, iPres0, iPres1);

		  reparto= porcentaje(pBRK_1,pBRK_2);

		  acelerador = checkRedundancy(pACC_1, pACC_2);

		  if(pBRK_1<pBRK_2){
			  freno = pBRK_1;
		  }else{
			  freno = pBRK_2;
		  }





		  pasarAcan.acelerador	=	checkRedundancy(pACC_1, pACC_2);
		  pasarAcan.freno	=	freno;
		  pasarAcan.direccion	=	checkRedundancy(pVOL_1, pVOL_2);

		  //Send CAN msg

		  memcpy(&BufferTxCAN_Pedals, &pasarAcan, CAN_length);
		  do{
		  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)>0){
			  HAL_CAN_AddTxMessage(&hcan, &TxCAN_Pedals, BufferTxCAN_Pedals, &txMailBox);
			  miTimer = 0;
		  }
		  }while(miTimer);

	  }else{
		  //do nothing
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

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8000-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = tiempoCiclo-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI_CS2_Pin|SPI_CS1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SPI_CS2_Pin SPI_CS1_Pin */
  GPIO_InitStruct.Pin = SPI_CS2_Pin|SPI_CS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
uint8_t mapValores(uint16_t entrada, uint16_t v0, uint16_t v1, uint16_t y0, uint16_t y1){
	float malda = ((float)y1 - (float)y0) / ((float)v1 - (float)v0);
	float salida = y0 + malda * (entrada - v0);
	if(salida < y0){
		salida = y0;
	}else if(salida>y1){
		salida = y1;
	}
	return (uint8_t) salida;
}

uint8_t checkRedundancy(uint8_t in_1, uint8_t in_2){
	uint8_t A, B;

	if(in_1 > in_2){
		A = in_1;
		B = in_2;
	}else{
		A = in_2;
		B = in_1;
	}

	if(B < (float)A * 0.7){
		B = 0;
	}

	return B;
}

uint16_t miVoltaje(uint8_t* datos){
	uint8_t arApoyo[2];
	uint16_t mi_16t;
	float f_calculo = 0;

	arApoyo[0] = datos[1];
	arApoyo[1] = datos[0];
	memcpy(&mi_16t, &arApoyo, 2);

	f_calculo = mi_16t * VCC *1000;
	f_calculo = f_calculo / 4096;
	mi_16t = (uint16_t) f_calculo;

	return mi_16t;
}

uint8_t porcentaje(uint8_t in1, uint8_t in2){
	uint16_t suma = in1+in2;
	float fFin = (float)in1/(float)suma;
	fFin=fFin*100;
	return (uint8_t)fFin;
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
