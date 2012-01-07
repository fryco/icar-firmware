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

extern u8 un_expect;

extern u8 *u1_tx_buf;//set to tx buffer before enable interrupt
extern u8 u1_rx_buf1[RX_BUF_SIZE];
extern u8 u1_rx_buf2[RX_BUF_SIZE];
extern u8 u1_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u1_rx_lost_data;//1: lost data
extern u8 u1_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u1_tx_cnt, u1_rx_cnt1, u1_rx_cnt2 ;

extern u8 *u2_tx_buf;//set to tx buffer before enable interrupt
extern u8 u2_rx_buf1[RX_BUF_SIZE];
extern u8 u2_rx_buf2[RX_BUF_SIZE];
extern u8 u2_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u2_rx_lost_data;//1: lost data
extern u8 u2_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u2_tx_cnt, u2_rx_cnt1, u2_rx_cnt2 ;

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
   By Jack Li, 2011-9-21   
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
	if ( !u1_rec_binary ) {//receive char
		if ( u1_rx_buf_flag ) {	//default save to buf1
			if ( u1_rx_buf1_ava ) { //buf1 still not handle
				u1_rx_lost_data = 1 ;
				un_expect = USART_ReceiveData(USART1);
			}
			else { //save to buf1
				u1_rx_buf1[u1_rx_cnt1] = USART_ReceiveData(USART1);
				u1_rx_cnt1++ ;
				if ((u1_rx_buf1[u1_rx_cnt1-1] == 0x0A) &&\
					(u1_rx_buf1[u1_rx_cnt1-2] == 0x0D)) {
					u1_rx_buf1_ava = 1 ;//receive a whole sentence
					u1_rx_buf_flag = 0 ;//use buf2 to receive
				}
				if ( u1_rx_cnt1 > RX_BUF_SIZE ) {
					u1_rx_lost_data = 1 ;
					u1_rx_cnt1 = 0 ;//prevent overflow, but date lost
				}
			}
		}
		else { //save to buf2
			if ( u1_rx_buf2_ava ) { //buf2 still not handle
				u1_rx_lost_data = 1 ;
				un_expect = USART_ReceiveData(USART1);
			}
			else { //save to buf2
				u1_rx_buf2[u1_rx_cnt2] = USART_ReceiveData(USART1);
				u1_rx_cnt2++ ;
				if ((u1_rx_buf2[u1_rx_cnt2-1] == 0x0A) &&\
					(u1_rx_buf2[u1_rx_cnt2-2] == 0x0D)) {
					u1_rx_buf2_ava = 1 ;//receive a whole sentence
					u1_rx_buf_flag = 1 ;//use buf1 to receive
				}
				if ( u1_rx_cnt2 > RX_BUF_SIZE ) {
					u1_rx_lost_data = 1 ;
					u1_rx_cnt2 = 0 ;//prevent overflow, but date lost
				}
			}
		}
	}
	else {//receive binary
		;//TBD
	}
  }

  if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
  {   
	if ( u1_tx_cnt > 0 ) {
	    /* Write one byte to the transmit data register */
	    USART_SendData(USART1, *u1_tx_buf);
		u1_tx_buf++;
		u1_tx_cnt--;
	}
	else {
      /* Disable the USART1 Transmit interrupt */
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
	if ( !u2_rec_binary ) {//receive char
		if ( u2_rx_buf_flag ) {	//default save to buf1
			if ( u2_rx_buf1_ava ) { //buf1 still not handle
				u2_rx_lost_data = 1 ;
				un_expect = USART_ReceiveData(USART2);
			}
			else { //save to buf1
				u2_rx_buf1[u2_rx_cnt1] = USART_ReceiveData(USART2);
				u2_rx_cnt1++ ;
				if ((u2_rx_buf1[u2_rx_cnt1-1] == 0x0A) &&\
					(u2_rx_buf1[u2_rx_cnt1-2] == 0x0D)) {
					u2_rx_buf1_ava = 1 ;//receive a whole sentence
					u2_rx_buf_flag = 0 ;//use buf2 to receive
				}
				if ( u2_rx_cnt1 > RX_BUF_SIZE ) {
					u2_rx_lost_data = 1 ;
					u2_rx_cnt1 = 0 ;//prevent overflow, but date lost
				}
			}
		}
		else { //save to buf2
			if ( u2_rx_buf2_ava ) { //buf2 still not handle
				u2_rx_lost_data = 1 ;
				un_expect = USART_ReceiveData(USART2);
			}
			else { //save to buf2
				u2_rx_buf2[u2_rx_cnt2] = USART_ReceiveData(USART2);
				u2_rx_cnt2++ ;
				if ((u2_rx_buf2[u2_rx_cnt2-1] == 0x0A) &&\
					(u2_rx_buf2[u2_rx_cnt2-2] == 0x0D)) {
					u2_rx_buf2_ava = 1 ;//receive a whole sentence
					u2_rx_buf_flag = 1 ;//use buf1 to receive
				}
				if ( u2_rx_cnt2 > RX_BUF_SIZE ) {
					u2_rx_lost_data = 1 ;
					u2_rx_cnt2 = 0 ;//prevent overflow, but date lost
				}
			}
		}
	}
	else {//receive binary
		;//TBD
	}
  }

  if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
  {   
	if ( u2_tx_cnt > 0 ) {
	    /* Write one byte to the transmit data register */
	    USART_SendData(USART2, *u2_tx_buf);
		u2_tx_buf++;
		u2_tx_cnt--;
	}
	else {
      /* Disable the USART2 Transmit interrupt */
      USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
    }    
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
