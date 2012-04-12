/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: http://icar-firmware.googlecode.com/svn/STM32/v040/source/App/drv_uart.h $ 
  * @version $Rev: 65 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-02-08 15:08:27 +0800 (周三, 2012-02-08) $
  * @brief   This is uart driver in STM32
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_UART_H
#define __DRV_UART_H

/* Exported macro ------------------------------------------------------------*/
#define TX_BUF_SIZE  1024
#define RX_BUF_SIZE  1024

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Using DMA
struct UART_TX {
	u8 buf1[TX_BUF_SIZE];
	u8 buf2[TX_BUF_SIZE];

	u32 buf1_cnt;
	u32 buf2_cnt;

	bool use_buf1;
	bool use_buf2;
}; End Using DMA */

//Using interrupt
struct UART_TX {
	u8 buf[TX_BUF_SIZE];

	u8 *out_last;
	u8 *in_last;

	bool empty;
};

struct UART_RX {
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

void delay_ms(u32 ms);

bool putbyte( COM_TypeDef COM, unsigned char );
bool putstring(COM_TypeDef COM, unsigned char  *);
unsigned char getbyte ( COM_TypeDef COM );

#endif /* __DRV_UART_H */
