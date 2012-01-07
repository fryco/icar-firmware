/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TRM_V1_H
#define __TRM_V1_H

#include "stm32f10x.h"

#ifdef __cplusplus
 extern "C" {
#endif


#define COMn                             2

/**
 * @brief Definition for COM port1, connected to USART1
 */ 
#define TRM_V1_COM1                        USART1
#define TRM_V1_COM1_CLK                    RCC_APB2Periph_USART1
#define TRM_V1_COM1_TX_PIN                 GPIO_Pin_9
#define TRM_V1_COM1_TX_GPIO_PORT           GPIOA
#define TRM_V1_COM1_TX_GPIO_CLK            RCC_APB2Periph_GPIOA
#define TRM_V1_COM1_RX_PIN                 GPIO_Pin_10
#define TRM_V1_COM1_RX_GPIO_PORT           GPIOA
#define TRM_V1_COM1_RX_GPIO_CLK            RCC_APB2Periph_GPIOA
#define TRM_V1_COM1_IRQn                   USART1_IRQn

/**
 * @brief Definition for COM port2, connected to USART2 (USART2 pins remapped on GPIOD)
 */ 
#define TRM_V1_COM2                        USART2
#define TRM_V1_COM2_CLK                    RCC_APB1Periph_USART2
#define TRM_V1_COM2_TX_PIN                 GPIO_Pin_2
#define TRM_V1_COM2_TX_GPIO_PORT           GPIOA
#define TRM_V1_COM2_TX_GPIO_CLK            RCC_APB2Periph_GPIOA
#define TRM_V1_COM2_RX_PIN                 GPIO_Pin_3
#define TRM_V1_COM2_RX_GPIO_PORT           GPIOA
#define TRM_V1_COM2_RX_GPIO_CLK            RCC_APB2Periph_GPIOA
#define TRM_V1_COM2_IRQn                   USART2_IRQn

typedef enum 
{
  COM1 = 0,
  COM2 = 1
} COM_TypeDef;   

void TRM_V1_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);

void gpio_init( void ) ;
void uart1_init( void ) ;

#ifdef __cplusplus
}
#endif
  
#endif /* __TRM_V1_H */
