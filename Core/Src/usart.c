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
#include <string.h>

/* USER CODE BEGIN 0 */
static uint8_t g_rx_buffer[DEFAULT_RX_BUFFER_SIZE] = { 0 };
static uint8_t g_tx_buffer[DEFAULT_TX_BUFFER_SIZE] = { 0 };

/*
 * DMA 以循环模式持续写入 g_rx_buffer。完成一次整环后由 TC 中断累计生产量，
 * 主循环则维护独立的消费量；这样 head == tail 不再同时表示“空”和“整环未读”。
 */
static volatile uint32_t g_rx_dma_completed_bytes = 0;
static uint32_t g_rx_consumed_bytes = 0;
static uint32_t g_rx_overrun_bytes = 0;
static volatile uint32_t g_rx_dma_error_count = 0;

static uint32_t MX_USART1_UART_GetProducedBytes(void)
{
  uint32_t completed_before;
  uint32_t completed_after;
  uint16_t dma_position;
  uint8_t transfer_complete_pending;

  /* TC 中断可能与主循环同时更新 completed，读取不一致时重新采样。 */
  do
  {
    completed_before = g_rx_dma_completed_bytes;
    dma_position = DEFAULT_RX_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_2);
    transfer_complete_pending = LL_DMA_IsActiveFlag_TC2(DMA1) ? 1U : 0U;
    completed_after = g_rx_dma_completed_bytes;
  }
  while (completed_before != completed_after);

  if (transfer_complete_pending)
  {
    /* DMA 已经自动重装 NDTR，但 TC ISR 尚未计入本轮；重新读取环内位置。 */
    dma_position = DEFAULT_RX_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_2);
    completed_after += DEFAULT_RX_BUFFER_SIZE;
  }

  return completed_after + dma_position;
}
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

  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&USART1->RDR, (uint32_t)g_rx_buffer, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, DEFAULT_RX_BUFFER_SIZE);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_2);

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
  LL_USART_DisableDMAReq_RX(USART1);
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_ClearFlag_GI2(DMA1);

  g_rx_dma_completed_bytes = 0;
  g_rx_consumed_bytes = 0;
  g_rx_overrun_bytes = 0;
  g_rx_dma_error_count = 0;

  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2,
                         (uint32_t)&USART1->RDR,
                         (uint32_t)g_rx_buffer,
                         LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, DEFAULT_RX_BUFFER_SIZE);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
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

uint16_t MX_USART1_UART_GetReceived(uint8_t * buf, uint16_t maxlen)
{
  if (buf == NULL || maxlen == 0)
  {
    return 0;
  }

  uint32_t produced = MX_USART1_UART_GetProducedBytes();
  uint32_t available = produced - g_rx_consumed_bytes;

  if (available > DEFAULT_RX_BUFFER_SIZE)
  {
    uint32_t dropped = available - DEFAULT_RX_BUFFER_SIZE;
    g_rx_consumed_bytes += dropped;
    g_rx_overrun_bytes += dropped;
    available = DEFAULT_RX_BUFFER_SIZE;
  }

  uint16_t bytes_to_read = available < maxlen ? (uint16_t)available : maxlen;
  for (uint16_t i = 0; i < bytes_to_read; ++i)
  {
    buf[i] = g_rx_buffer[(g_rx_consumed_bytes + i) % DEFAULT_RX_BUFFER_SIZE];
  }
  g_rx_consumed_bytes += bytes_to_read;

  return bytes_to_read;
}

uint32_t MX_USART1_UART_GetRxOverrunCount(void)
{
  return g_rx_overrun_bytes;
}

uint32_t MX_USART1_UART_GetRxErrorCount(void)
{
  return g_rx_dma_error_count;
}

void MX_USART1_UART_RxDmaIRQHandler(void)
{
  if (LL_DMA_IsActiveFlag_TE2(DMA1))
  {
    LL_DMA_ClearFlag_TE2(DMA1);
    ++g_rx_dma_error_count;
  }

  if (LL_DMA_IsActiveFlag_TC2(DMA1))
  {
    LL_DMA_ClearFlag_TC2(DMA1);
    g_rx_dma_completed_bytes += DEFAULT_RX_BUFFER_SIZE;
  }
}

/* USER CODE END 1 */
