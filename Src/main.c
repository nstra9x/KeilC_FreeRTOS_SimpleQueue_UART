#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "event_groups.h"

#include "string.h"
#include "stdio.h"

UART_HandleTypeDef huart3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);

TaskHandle_t HPTHandler;
void Sender_HPT_TASK(void *pvParameter);

TaskHandle_t LPTHandler;
void Sender_MPT_TASK(void *pvParameter);

TaskHandle_t LPTHandler;
void Receiver_LPT_TASK(void *pvParameter);

uint8_t rx_data = 0;
int SendFromISR = 0;

QueueHandle_t SimpleQueue;

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART3_UART_Init();
	
	HAL_UART_Receive_IT(&huart3, &rx_data, 1);
	
	SimpleQueue = xQueueCreate(5, sizeof(int));
	if(SimpleQueue == NULL)
		HAL_UART_Transmit(&huart3, (uint8_t *) "Khong the tao Queue\n\r", 21, 100);
	else
		HAL_UART_Transmit(&huart3, (uint8_t *) "Tao Queue thanh cong\n\r", 21, 100);
	
	xTaskCreate(Sender_HPT_TASK, "Sender HPT Task", 128, NULL, 2, &HPTHandler);
	xTaskCreate(Sender_MPT_TASK, "Sender MPT Task", 128, NULL, 1, &LPTHandler);
	xTaskCreate(Receiver_LPT_TASK, "Receiver LPT Task", 128, NULL, 0, &LPTHandler);
	
	vTaskStartScheduler();
}

void Sender_HPT_TASK(void *pvParameter)
{
	int send_value = 0;
	char buff[30];
	while(1)
	{
		if(xQueueSend(SimpleQueue, &send_value, 500) == pdPASS)
		{
			char str[100] = "Gui queue HPT TASK\r";
			sprintf(buff, "Gia tri gui la: %d\r", send_value);
			strcat(str, buff);
			HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
		}
		else
		{
			char str[100] = "Loi khi gui queue\r";
			HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
		}
		send_value++;
		vTaskDelay(2000);
	}
}

void Sender_MPT_TASK(void *pvParameter)
{
	int send_value = 100;
	char buff[30];
	while(1)
	{
		if(xQueueSend(SimpleQueue, &send_value, 500) == pdPASS)
		{
			char str[100] = "Gui queue MPT TASK\r";
			sprintf(buff, "Gia tri gui la: %d\r", send_value);
			strcat(str, buff);
			HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
		}
		else
		{
			char str[100] = "Loi khi gui queue\r";
			HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
		}
		send_value--;
		vTaskDelay(2000);
	}
}

void Receiver_LPT_TASK(void *pvParameter)
{
	int receive_value = 0;
	char buff[30];
	while(1)
	{
	if(xQueueReceive(SimpleQueue, &receive_value, 500) == pdPASS)
	{
		char str[100] = "Nhan queue thanh cong\r ";
		sprintf(buff, "Gia tri nhan duoc la: %d\r", receive_value);
		strcat(str, buff);
		HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
	}
	else
		{
			char str[100] = "Loi khi nhan queue\r";
			HAL_UART_Transmit(&huart3, (uint8_t *) str, strlen(str), HAL_MAX_DELAY); 
		}
		vTaskDelay(1000);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	HAL_UART_Receive_IT(&huart3, &rx_data, 1);
	
	if(rx_data == 'r')
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if(xQueueSendToFrontFromISR(SimpleQueue, &SendFromISR, &xHigherPriorityTaskWoken) == pdPASS)
		{
			HAL_UART_Transmit(&huart3, (uint8_t *) "Gui du lieu tu ISR\n\r", 20, 500); 
			SendFromISR--;
		}
		
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
	}

}

static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

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

void Error_Handler(void)
{
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
