/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"
#include "utils/cbuffer.h"

#include <string.h>

/* USER CODE BEGIN 0 */
static circular_buffer_t g_rx_buffer = { 0 };
static uint8_t g_tx_buffer[DEFAULT_TX_BUFFER_SIZE] = { 0 };
/* USER CODE END 0 */

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK1);

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**USART1 GPIO Configuration
  PA9   ------> USART1_TX
  PA10   ------> USART1_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART1 DMA Init */

  /* USART1_RX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_USART1_RX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_MEDIUM);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_CIRCULAR);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&USART1->RDR, (uint32_t)&g_rx_buffer.buffer, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, DEFAULT_RX_BUFFER_SIZE);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);

  /* USART1_TX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_USART1_TX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

  /* USART1 interrupt Init */
  // NVIC_SetPriority(USART1_IRQn, 0);
  // NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
  USART_InitStruct.BaudRate = 921600;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_DisableFIFO(USART1);
  LL_USART_ConfigAsyncMode(USART1);

  /* USER CODE BEGIN WKUPType USART1 */

  /* USER CODE END WKUPType USART1 */

  LL_USART_Enable(USART1);

  /* Polling USART1 initialisation */
  while((!(LL_USART_IsActiveFlag_TEACK(USART1))) || (!(LL_USART_IsActiveFlag_REACK(USART1))))
  {
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* USER CODE BEGIN 1 */

void MX_USART1_UART_StartReceive()
{
  LL_USART_EnableDMAReq_RX(USART1);
}

uint8_t MX_USART1_UART_CheckTXAvailability()
{
  if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_3))
  {
    if (!LL_DMA_IsActiveFlag_TC3(DMA1))
    {
      return 0;
    }
    else
    {
      LL_DMA_ClearFlag_TC3(DMA1);
    }

    if (!LL_USART_IsActiveFlag_TC(USART1))
    {
      return 0;
    }
    else
    {
      LL_USART_ClearFlag_TC(USART1);
    }
  }

  return 1;
}

void MX_USART1_UART_DMASend(const uint8_t * data, uint16_t len)
{
  uint16_t size = len >= DEFAULT_TX_BUFFER_SIZE ? DEFAULT_TX_BUFFER_SIZE : len;
  memcpy(g_tx_buffer, data, size);

  LL_USART_DisableDMAReq_TX(USART1);
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_3, (uint32_t)&g_tx_buffer, (uint32_t)&USART1->TDR, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, size);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
  LL_USART_EnableDMAReq_TX(USART1);
}

void MX_USART1_UART_Send(const uint8_t * data, uint16_t len)
{
  for (uint16_t i = 0; i < len; ++i)
  {
    while (!LL_USART_IsActiveFlag_TXE(USART1))
    {
        // Wait until TXE flag is set
    }

    LL_USART_TransmitData8(USART1, *data);
    ++data;
  }
}

void MX_USART1_UART_UpdateBufferHead()
{
  static uint16_t last_ndtr = DEFAULT_RX_BUFFER_SIZE;

  uint16_t current_ndtr = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_2);

  if (current_ndtr != last_ndtr)
  {
    uint16_t new_head = DEFAULT_RX_BUFFER_SIZE - current_ndtr;
    if (new_head < g_rx_buffer.head)
    {
      // TODO
    }

    g_rx_buffer.head = new_head;
    last_ndtr = current_ndtr;
  }
}

uint16_t MX_USART1_UART_GetReceived(uint8_t * buf, uint16_t maxlen)
{
  uint16_t bytes_read = 0;
  uint8_t byte;

  while (bytes_read < maxlen && circular_buffer_read_byte(&g_rx_buffer, &byte))
  {
    buf[bytes_read++] = byte;
  }

  return bytes_read;
}

void * MX_USART1_UART_GetRecvBuffer()
{
  return (void *)&g_rx_buffer;
}

/* USER CODE END 1 */
