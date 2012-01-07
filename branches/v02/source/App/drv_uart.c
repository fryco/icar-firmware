#include "main.h"
#include "trm_v1.h"
u8 un_expect;

u8 *u1_tx_buf=NULL;//set to tx buffer before enable interrupt
u8 u1_rx_buf1[RX_BUF_SIZE];
u8 u1_rx_buf2[RX_BUF_SIZE];
u8 u1_rx_buf1_ava = 0 ;//0: unavailable, 1: available
u8 u1_rx_buf2_ava = 0 ;//0: unavailable, 1: available
u8 u1_rx_buf_flag = 1 ;//1: use buf1, 0: use buf2
u8 u1_rx_lost_data= 0 ;//1: lost data
u8 u1_rec_binary = 0; //0:receive bin, 1:char, default is char
u32 u1_tx_cnt = 0, u1_rx_cnt1 = 0 , u1_rx_cnt2 = 0 ;

u8 *u2_tx_buf=NULL;//set to tx buffer before enable interrupt
u8 u2_rx_buf1[RX_BUF_SIZE];
u8 u2_rx_buf2[RX_BUF_SIZE];
u8 u2_rx_buf1_ava = 0 ;//0: unavailable, 1: available
u8 u2_rx_buf2_ava = 0 ;//0: unavailable, 1: available
u8 u2_rx_buf_flag = 1 ;//1: use buf1, 0: use buf2
u8 u2_rx_lost_data= 0 ;//1: lost data
u8 u2_rec_binary = 0; //0:receive bin, 1:char, default is char
u32 u2_tx_cnt = 0, u2_rx_cnt1 = 0 , u2_rx_cnt2 = 0 ;



void uart0_init( void ) 
{
	USART_InitTypeDef USART1_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* USARTx configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
            RCC_APB2Periph_AFIO |
            RCC_APB2Periph_USART1 , 
            ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; need external pull up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//use internal pull up
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART1_InitStructure.USART_BaudRate = 115200;
	USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART1_InitStructure.USART_StopBits = USART_StopBits_1;
	USART1_InitStructure.USART_Parity = USART_Parity_No ;
	USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART1_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_Cmd(USART1, ENABLE);
}

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  //USART_SendData(USART1, (uint8_t) ch);
  USART_SendData(TRM_V1_COM1, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  {}

  return ch;
}
