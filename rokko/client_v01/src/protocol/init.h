#ifndef __INIT_H__
#define __INT_H__

#include "stm32f10x_lib.h"

#ifdef _VCI2
void RCC_Configuration(void);
void GPIO_Configuration(void);
void RTC_Configuration(void);
void RTC_Configuration(void);
void TIM2_Configuration(void);
void SysTick_Configuration(void);
#endif

void ADC1_Configuration(void);
void USART_Configuration(USART_TypeDef* USART,u32 bps,u16 databit,u16 paritybit);
void SetIoMode(GPIO_TypeDef* io,u16 pin,GPIOMode_TypeDef mode);
void NVIC_Configuration(u8 IRQChannel,u8 Priority);

#endif
