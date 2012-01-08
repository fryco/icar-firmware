/**
  ******************************************************************************
  * @file    source/App/drv_uart.h 
  * @author  cn0086@139.com
  * @version V00
  * @date    2011/10/27 9:54:35
  * @brief   This file contains global function
  ******************************************************************************
  * @history v00: 2011/10/27, draft, by cn0086@139.com
  * @        v01:
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_UART_H
#define __DRV_UART_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define TX_BUF_SIZE  256
#define RX_BUF_SIZE  256

/* Exported functions ------------------------------------------------------- */
void uart0_init( void );

#endif /* __DRV_UART_H */
