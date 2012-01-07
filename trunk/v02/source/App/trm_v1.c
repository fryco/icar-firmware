#include <stdio.h>
#include "stm32f10x.h"
#include "trm_v1.h"

USART_TypeDef* COM_USART[COMn] = {TRM_V1_COM1, TRM_V1_COM2}; 

GPIO_TypeDef* COM_TX_PORT[COMn] = {TRM_V1_COM1_TX_GPIO_PORT, TRM_V1_COM2_TX_GPIO_PORT};
 
GPIO_TypeDef* COM_RX_PORT[COMn] = {TRM_V1_COM1_RX_GPIO_PORT, TRM_V1_COM2_RX_GPIO_PORT};
 
const uint32_t COM_USART_CLK[COMn] = {TRM_V1_COM1_CLK, TRM_V1_COM2_CLK};

const uint32_t COM_TX_PORT_CLK[COMn] = {TRM_V1_COM1_TX_GPIO_CLK, TRM_V1_COM2_TX_GPIO_CLK};
 
const uint32_t COM_RX_PORT_CLK[COMn] = {TRM_V1_COM1_RX_GPIO_CLK, TRM_V1_COM2_RX_GPIO_CLK};

const uint16_t COM_TX_PIN[COMn] = {TRM_V1_COM1_TX_PIN, TRM_V1_COM2_TX_PIN};

const uint16_t COM_RX_PIN[COMn] = {TRM_V1_COM1_RX_PIN, TRM_V1_COM2_RX_PIN};

void TRM_V1_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clock */
  RCC_APB2PeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM] | RCC_APB2Periph_AFIO, ENABLE);

  /* Enable UART clock */
  if (COM == COM1)
  {
    RCC_APB2PeriphClockCmd(COM_USART_CLK[COM], ENABLE); 
  }
  else
  {
    /* Enable the USART2 Pins Software Remapping */
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(COM_USART_CLK[COM], ENABLE);
  }

  /* Configure USART Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);

  /* Configure USART Rx as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
  GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

  /* USART configuration */
  USART_Init(COM_USART[COM], USART_InitStruct);
    
  /* Enable USART */
  USART_Cmd(COM_USART[COM], ENABLE);
}


void uart1_init( void ) 
{
  USART_InitTypeDef USART1_InitStructure;

  /* USARTx configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART1_InitStructure.USART_BaudRate = 115200;
  USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART1_InitStructure.USART_StopBits = USART_StopBits_1;
  USART1_InitStructure.USART_Parity = USART_Parity_No;
  USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  TRM_V1_COMInit(COM1, &USART1_InitStructure);
}
