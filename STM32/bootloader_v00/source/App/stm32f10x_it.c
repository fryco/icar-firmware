/**
  ******************************************************************************
  * @file    SysTick/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    10/15/2010
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "main.h"

extern struct ICAR_DEVICE my_icar;

/** @addtogroup STM32F10x_StdPeriph_Examples
  * @{
  */

/** @addtogroup SysTick
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/*
void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}
*/

/* Replace by
   DCD     OS_CPU_PendSVHandler                        ; 14, PendSV Handler
   DCD     OS_CPU_SysTickHandler                       ; 15, uC/OS-II Tick ISR Handler

   Check startup.s
   By cn0086@139.com, 2011-9-21
*/

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		if(!my_icar.stm32_u1_rx.full) { //buffer no full
			//
	   		//*(my_icar.stm32_u1_rx.in_last)= USART_ReceiveData(USART1);
			*(my_icar.stm32_u1_rx.in_last) = USART1->DR & (uint16_t)0x01FF ;
	    	my_icar.stm32_u1_rx.in_last++;
	     	my_icar.stm32_u1_rx.empty = false; 

		   	if (my_icar.stm32_u1_rx.in_last==my_icar.stm32_u1_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u1_rx.in_last=my_icar.stm32_u1_rx.buf;//地址到顶部回到底部
			}

    		if (my_icar.stm32_u1_rx.in_last==my_icar.stm32_u1_rx.out_last)	{
				my_icar.stm32_u1_rx.full = true;  //set buffer full flag
			}
		}
		else { //buffer full, lost data
			my_icar.stm32_u1_rx.lost_data = true ;
			USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		}
	}

	//Transmit
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{
		if ( !my_icar.stm32_u1_tx.empty ) {
			USART1->DR = (u8) *(my_icar.stm32_u1_tx.out_last);
			my_icar.stm32_u1_tx.out_last++ ;
			if ( my_icar.stm32_u1_tx.out_last == my_icar.stm32_u1_tx.buf + TX_BUF_SIZE ) {
				//地址到顶部回到底部
				my_icar.stm32_u1_tx.out_last  = my_icar.stm32_u1_tx.buf;
			}
			if ( my_icar.stm32_u1_tx.out_last == my_icar.stm32_u1_tx.in_last ) {
				//all buffer had been sent
				my_icar.stm32_u1_tx.empty = true ;
			}
		}
		else {
		  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}

}

/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		if(!my_icar.stm32_u2_rx.full) { //buffer no full
			//
	   		//*(my_icar.stm32_u2_rx.in_last)= USART_ReceiveData(USART2);
			*(my_icar.stm32_u2_rx.in_last) = USART2->DR & (uint16_t)0x01FF ;
	    	my_icar.stm32_u2_rx.in_last++;
	     	my_icar.stm32_u2_rx.empty = false; 

		   	if (my_icar.stm32_u2_rx.in_last==my_icar.stm32_u2_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u2_rx.in_last=my_icar.stm32_u2_rx.buf;//地址到顶部回到底部
			}

    		if (my_icar.stm32_u2_rx.in_last==my_icar.stm32_u2_rx.out_last)	{
				my_icar.stm32_u2_rx.full = true;  //set buffer full flag
			}
		}
		else { //buffer full, lost data
			my_icar.stm32_u2_rx.lost_data = true ;
			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
		}
	}

	//Transmit
	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{
		if ( !my_icar.stm32_u2_tx.empty ) {
			USART2->DR = (u8) *(my_icar.stm32_u2_tx.out_last);
			my_icar.stm32_u2_tx.out_last++ ;
			if ( my_icar.stm32_u2_tx.out_last == my_icar.stm32_u2_tx.buf + TX_BUF_SIZE ) {
				//地址到顶部回到底部
				my_icar.stm32_u2_tx.out_last  = my_icar.stm32_u2_tx.buf;
			}
			if ( my_icar.stm32_u2_tx.out_last == my_icar.stm32_u2_tx.in_last ) {
				//all buffer had been sent
				my_icar.stm32_u2_tx.empty = true ;
			}
		}
		else {
		  USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}
	}
}

void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		if(!my_icar.stm32_u3_rx.full) { //buffer no full

			*(my_icar.stm32_u3_rx.in_last) = USART3->DR & (uint16_t)0x01FF ;
	    	my_icar.stm32_u3_rx.in_last++;
	     	my_icar.stm32_u3_rx.empty = false; 

		   	if (my_icar.stm32_u3_rx.in_last==my_icar.stm32_u3_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u3_rx.in_last=my_icar.stm32_u3_rx.buf;//地址到顶部回到底部
			}

    		if (my_icar.stm32_u3_rx.in_last==my_icar.stm32_u3_rx.out_last)	{
				my_icar.stm32_u3_rx.full = true;  //set buffer full flag
			}
		}
		else { //buffer full, lost data
			my_icar.stm32_u3_rx.lost_data = true ;
			USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		}
	}

	//Transmit
	if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)
	{
		if ( !my_icar.stm32_u3_tx.empty ) {
			USART3->DR = (u8) *(my_icar.stm32_u3_tx.out_last);
			my_icar.stm32_u3_tx.out_last++ ;
			if ( my_icar.stm32_u3_tx.out_last == my_icar.stm32_u3_tx.buf + TX_BUF_SIZE ) {
				//地址到顶部回到底部
				my_icar.stm32_u3_tx.out_last  = my_icar.stm32_u3_tx.buf;
			}
			if ( my_icar.stm32_u3_tx.out_last == my_icar.stm32_u3_tx.in_last ) {
				//all buffer had been sent
				my_icar.stm32_u3_tx.empty = true ;
			}
		}
		else {
		  USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		}
	}
}

void DMA1_Channel4_IRQHandler(void)
{
	/* Uart1 TX
	//if(DMA_GetITStatus(DMA1_IT_TC4)){
	if(DMA1->ISR&DMA1_IT_TC4){//通道4传输完成中断

		//DMA_Cmd(DMA1_Channel4, DISABLE);
		DMA1_Channel4->CCR &= (uint16_t)(~DMA_CCR1_EN);

		if ( my_icar.stm32_u1_tx.use_buf1 ) {
			my_icar.stm32_u1_tx.buf1_cnt = 0;
			my_icar.stm32_u1_tx.use_buf1 = false;

			if (( !my_icar.stm32_u1_tx.use_buf2 ) && ( my_icar.stm32_u1_tx.buf2_cnt > 0 )){
				//start DMA with buf2
				my_icar.stm32_u1_tx.use_buf2 = true;

				//write DMA Channelx CMAR to configure BaseAddr
				DMA1_Channel4->CMAR = (u32)my_icar.stm32_u1_tx.buf2;
				//Write to DMA Channelx CNDTR to configure buffer size
				DMA1_Channel4->CNDTR = my_icar.stm32_u1_tx.buf2_cnt;
				//DMA_Cmd(DMA1_Channel4, ENABLE);
				DMA1_Channel4->CCR |= DMA_CCR1_EN;
			}
		}
		else {
			if ( my_icar.stm32_u1_tx.use_buf2 ) {
				my_icar.stm32_u1_tx.buf2_cnt = 0;
				my_icar.stm32_u1_tx.use_buf2 = false;

				if (( !my_icar.stm32_u1_tx.use_buf1 ) && ( my_icar.stm32_u1_tx.buf1_cnt > 0 )){
					//start DMA with buf1
					my_icar.stm32_u1_tx.use_buf1 = true;

					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel4->CMAR = (u32)my_icar.stm32_u1_tx.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel4->CNDTR = my_icar.stm32_u1_tx.buf1_cnt;
					//DMA_Cmd(DMA1_Channel4, ENABLE);
					DMA1_Channel4->CCR |= DMA_CCR1_EN;
				}
			}
		}
	}
	End Uart1 TX */

	DMA_ClearITPendingBit(DMA1_IT_GL4|DMA1_IT_TC4|DMA1_IT_HT1);
}

void DMA1_Channel7_IRQHandler(void)
{
	/* Uart2 TX
	//if(DMA_GetITStatus(DMA1_IT_TC7)){
	if(DMA1->ISR&DMA1_IT_TC7){//通道7传输完成中断

		//DMA_Cmd(DMA1_Channel7, DISABLE);
		DMA1_Channel7->CCR &= (uint16_t)(~DMA_CCR1_EN);

		if ( my_icar.stm32_u2_tx.use_buf1 ) {
			my_icar.stm32_u2_tx.buf1_cnt = 0;
			my_icar.stm32_u2_tx.use_buf1 = false;

			if (( !my_icar.stm32_u2_tx.use_buf2 ) && ( my_icar.stm32_u2_tx.buf2_cnt > 0 )){
				//start DMA with buf2
				my_icar.stm32_u2_tx.use_buf2 = true;

				//write DMA Channelx CMAR to configure BaseAddr
				DMA1_Channel7->CMAR = (u32)my_icar.stm32_u2_tx.buf2;
				//Write to DMA Channelx CNDTR to configure buffer size
				DMA1_Channel7->CNDTR = my_icar.stm32_u2_tx.buf2_cnt;
				//DMA_Cmd(DMA1_Channel5, ENABLE);
				DMA1_Channel7->CCR |= DMA_CCR1_EN;
			}
		}
		else {
			if ( my_icar.stm32_u2_tx.use_buf2 ) {
				my_icar.stm32_u2_tx.buf2_cnt = 0;
				my_icar.stm32_u2_tx.use_buf2 = false;

				if (( !my_icar.stm32_u2_tx.use_buf1 ) && ( my_icar.stm32_u2_tx.buf1_cnt > 0 )){
					//start DMA with buf1
					my_icar.stm32_u2_tx.use_buf1 = true;

					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel7->CMAR = (u32)my_icar.stm32_u2_tx.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel7->CNDTR = my_icar.stm32_u2_tx.buf1_cnt;
					//DMA_Cmd(DMA1_Channel5, ENABLE);
					DMA1_Channel7->CCR |= DMA_CCR1_EN;
				}
			}
		}
	}
	End Uart2 TX */

	DMA_ClearITPendingBit(DMA1_IT_GL7|DMA1_IT_TC7|DMA1_IT_HT1);
}

/**
  * @brief  This function handles RTC global interrupt request.
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
  {
    /* Clear the RTC Second interrupt */
    RTC_ClearITPendingBit(RTC_IT_SEC);

    /* Enable time update */
    //TimeDisplay = 1;

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
  }
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
