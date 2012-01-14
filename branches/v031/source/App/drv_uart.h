/**
  ******************************************************************************
  * @file    source/App/drv_uart.h
  * @author  cn0086@139.com
  * @version V02
  * @date    2011/11/24 10:34:18
  * @brief   This file contains uart driver
  ******************************************************************************
  * @history v00: 2011/10/27, draft, by cn0086@139.com
  * @        v01: 2011/10/29, u1 tx use DMA, rx use interrupt
  * @        v01: 2011/11/24, uart tx/rx use interrupt
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_UART_H
#define __DRV_UART_H

/* Exported macro ------------------------------------------------------------*/
#define TX_BUF_SIZE  16
#define RX_BUF_SIZE  64

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Using DMA
struct ICAR_TX {
	u8 buf1[TX_BUF_SIZE];
	u8 buf2[TX_BUF_SIZE];

	u32 buf1_cnt;
	u32 buf2_cnt;

	bool use_buf1;
	bool use_buf2;
}; End Using DMA */

//Using interrupt
struct ICAR_TX {
	u8 buf[TX_BUF_SIZE];

	u8 *out_last;
	u8 *in_last;

	bool empty;
};

struct ICAR_RX {
	u8 buf[RX_BUF_SIZE];

	u8 *out_last;
	u8 *in_last;

	bool empty;
	bool full;
	bool lost_data;
};

typedef enum
{
	COM1 = 0,
	COM2 = 1,
	COM3 = 2
} COM_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void uart1_init( void );
void uart2_init( void );
void uart3_init( void );

bool putbyte( COM_TypeDef COM, unsigned char );
bool putstring(COM_TypeDef COM, unsigned char  *);
unsigned char getbyte ( COM_TypeDef COM );

#endif /* __DRV_UART_H */
