#include "main.h"

//gobal var
struct ICAR_TX u1_tx_buf;
struct ICAR_RX u1_rx_buf;

struct ICAR_TX u2_tx_buf;
struct ICAR_RX u2_rx_buf;

struct ICAR_TX u3_tx_buf;
struct ICAR_RX u3_rx_buf;

/* Private function prototypes -----------------------------------------------*/

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

void uart1_init( void ) 
{
	USART_InitTypeDef USART1_InitStructure;
	//DMA_InitTypeDef DMA_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//Init Global vars
	/* Using DMA 
	u1_tx_buf.buf1_cnt = 0;
	u1_tx_buf.buf2_cnt = 0;
	u1_tx_buf.use_buf1 = false;
	u1_tx_buf.use_buf2 = false;
	*/

	/* Using interrupt */
	u1_tx_buf.out_last = u1_tx_buf.buf;
	u1_tx_buf.in_last  = u1_tx_buf.buf;
	u1_tx_buf.empty = true;

	u1_rx_buf.out_last = u1_rx_buf.buf;
	u1_rx_buf.in_last  = u1_rx_buf.buf;
	u1_rx_buf.full  = false;
	u1_rx_buf.empty = true;
	u1_rx_buf.lost_data= false;

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
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	USART1_InitStructure.USART_BaudRate = 115200;
	USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART1_InitStructure.USART_StopBits = USART_StopBits_1;
	USART1_InitStructure.USART_Parity = USART_Parity_No ;
	USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART1_InitStructure);

	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);

	/* Using DMA
    //DMA Init for USART1.Tx --> DMA.CH4
    DMA_DeInit(DMA1_Channel4);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)USART1+4;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)u1_tx_buf.buf1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

	//DMA通道1传输完成中断
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

    //Set USART1.Tx mode to DMA
   	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	End using DMA */

	USART_Cmd(USART1, ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //need external pull up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//use internal pull up
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void uart2_init( void ) 
{
	USART_InitTypeDef USART2_InitStructure;
	//DMA_InitTypeDef DMA_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//Init Global vars
	/* Using DMA
	u2_tx_buf.buf1_cnt = 0;
	u2_tx_buf.buf2_cnt = 0;
	u2_tx_buf.use_buf1 = false;
	u2_tx_buf.use_buf2 = false;
	*/

	/* Using interrupt */
	u2_tx_buf.out_last = u2_tx_buf.buf;
	u2_tx_buf.in_last  = u2_tx_buf.buf;
	u2_tx_buf.empty = true;

	u2_rx_buf.out_last = u2_rx_buf.buf;
	u2_rx_buf.in_last  = u2_rx_buf.buf;
	u2_rx_buf.full  = false;
	u2_rx_buf.empty = true;
	u2_rx_buf.lost_data= false;

	/* USARTx configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 , ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	USART2_InitStructure.USART_BaudRate = 115200;
	USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART2_InitStructure.USART_StopBits = USART_StopBits_1;
	USART2_InitStructure.USART_Parity = USART_Parity_No ;
	USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART2_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART2_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);

	/* Using DMA
    //DMA Init for USART2.Tx --> DMA.CH7
    DMA_DeInit(DMA1_Channel7);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)USART2+4;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)u2_tx_buf.buf1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);

	//DMA通道7传输完成中断
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

    //Set USART2.Tx mode to DMA
   	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	End Using DMA */

	USART_Cmd(USART2, ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //need external pull up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void uart3_init( void ) 
{
	USART_InitTypeDef USART3_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//Init Global vars
	u3_tx_buf.out_last = u3_tx_buf.buf;
	u3_tx_buf.in_last  = u3_tx_buf.buf;
	u3_tx_buf.empty = true;

	u3_rx_buf.out_last = u3_rx_buf.buf;
	u3_rx_buf.in_last  = u3_rx_buf.buf;
	u3_rx_buf.full  = false;
	u3_rx_buf.empty = true;
	u3_rx_buf.lost_data= false;

	/* USARTx configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | 
            RCC_APB2Periph_AFIO ,
            ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART3_InitStructure.USART_BaudRate = 115200;
	USART3_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART3_InitStructure.USART_StopBits = USART_StopBits_1;
	USART3_InitStructure.USART_Parity = USART_Parity_No ;
	USART3_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART3_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART3_InitStructure);

	USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);

	USART_Cmd(USART3, ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//use internal pull up
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//***************************** 
//放入一个字节到发送缓冲区
//经测试，会有溢出，因为串口发送速度不及放入速度
//已加延时解决
bool putbyte( COM_TypeDef COM, unsigned char ch )
{
/* Using interrupt */
	switch( COM ){
		case COM1:

		    while ( (((u1_tx_buf.out_last-u1_tx_buf.in_last)==2) \
					&& (u1_tx_buf.out_last > u1_tx_buf.in_last ))\
					|| ((u1_tx_buf.out_last < u1_tx_buf.in_last) \
					&& (TX_BUF_SIZE-(u1_tx_buf.in_last-u1_tx_buf.out_last)==2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 1); //wait 1 msecond, send ~10Bytes@115200
				USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			*(u1_tx_buf.in_last) = ch ; //put to send buffer
			u1_tx_buf.in_last++;
			if ( u1_tx_buf.in_last == u1_tx_buf.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				u1_tx_buf.in_last  = u1_tx_buf.buf;
			}

			u1_tx_buf.empty = false;
			//start transmit
			USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

			return true;

		case COM2:
		    while ( (((u2_tx_buf.out_last-u2_tx_buf.in_last)==2) \
					&& (u2_tx_buf.out_last > u2_tx_buf.in_last ))\
					|| ((u2_tx_buf.out_last < u2_tx_buf.in_last) \
					&& (TX_BUF_SIZE-(u2_tx_buf.in_last-u2_tx_buf.out_last)==2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 1); //wait 1 msecond, send ~10Bytes@115200
				USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			*(u2_tx_buf.in_last) = ch ; //put to send buffer
			u2_tx_buf.in_last++;
			if ( u2_tx_buf.in_last == u2_tx_buf.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				u2_tx_buf.in_last  = u2_tx_buf.buf;
			}

			u2_tx_buf.empty = false;
			//start transmit
			USART_ITConfig(USART2, USART_IT_TXE, ENABLE);

			return true;

		case COM3:

		    while ( (((u3_tx_buf.out_last-u3_tx_buf.in_last)==2) \
					&& (u3_tx_buf.out_last > u3_tx_buf.in_last ))\
					|| ((u3_tx_buf.out_last < u3_tx_buf.in_last) \
					&& (TX_BUF_SIZE-(u3_tx_buf.in_last-u3_tx_buf.out_last)==2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 1); //wait 1 msecond, send ~10Bytes@115200
				USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			*(u3_tx_buf.in_last) = ch ; //put to send buffer
			u3_tx_buf.in_last++;
			if ( u3_tx_buf.in_last == u3_tx_buf.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				u3_tx_buf.in_last  = u3_tx_buf.buf;
			}

			u3_tx_buf.empty = false;
			//start transmit
			USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

			return true;

		default://no this COM port
			return false;
	}

/* Using DMA ***
	switch( COM ){
		case COM1:

			if ( !u1_tx_buf.use_buf1 ) {//buf1 free, add to buf1
		
				u1_tx_buf.buf1[u1_tx_buf.buf1_cnt] = ch;
				u1_tx_buf.buf1_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel4->CCR & 0x01)){
					//start DMA with buf1
					u1_tx_buf.use_buf1 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel4->CMAR = (u32)u1_tx_buf.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel4->CNDTR = u1_tx_buf.buf1_cnt;
					//DMA_Cmd(DMA1_Channel4, ENABLE);
				    DMA1_Channel4->CCR |= DMA_CCR1_EN;
				}
				else {		
					if ( u1_tx_buf.buf1_cnt > TX_BUF_SIZE -2 ) {//overflow
						u1_tx_buf.buf1_cnt--;
						return false;
					}
				}
			return true;
			}
		
			if ( !u1_tx_buf.use_buf2 ) {//buf2 free, add to buf2
		
				u1_tx_buf.buf2[u1_tx_buf.buf2_cnt] = ch;
				u1_tx_buf.buf2_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel4->CCR & 0x01)){
					//start DMA with buf2
					u1_tx_buf.use_buf2 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel4->CMAR = (u32)u1_tx_buf.buf2;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel4->CNDTR = u1_tx_buf.buf2_cnt;
					//DMA_Cmd(DMA1_Channel4, ENABLE);
				    DMA1_Channel4->CCR |= DMA_CCR1_EN;
				}
				else {	
					if ( u1_tx_buf.buf2_cnt > TX_BUF_SIZE -2 ) {//overflow
						u1_tx_buf.buf2_cnt--;
						return false;
					}
				}
			return true;
			}
		
			OSTimeDlyHMSM(0, 0,	2, 0); //wait 2 sec.
		
			return false;

		case COM2:

			if ( !u2_tx_buf.use_buf1 ) {//buf1 free, add to buf1
		
				u2_tx_buf.buf1[u2_tx_buf.buf1_cnt] = ch;
				u2_tx_buf.buf1_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel7->CCR & 0x01)){
					//start DMA with buf1
					u2_tx_buf.use_buf1 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel7->CMAR = (u32)u2_tx_buf.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel7->CNDTR = u2_tx_buf.buf1_cnt;
					//DMA_Cmd(DMA1_Channel5, ENABLE);
				    DMA1_Channel7->CCR |= DMA_CCR1_EN;
				}
				else {
					if ( u2_tx_buf.buf1_cnt > TX_BUF_SIZE -2 ) {//overflow
						u2_tx_buf.buf1_cnt--;
						return false;
					}
				}
			return true;
			}
		
			if ( !u2_tx_buf.use_buf2 ) {//buf2 free, add to buf2
		
				u2_tx_buf.buf2[u2_tx_buf.buf2_cnt] = ch;
				u2_tx_buf.buf2_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel7->CCR & 0x01)){
					//start DMA with buf2
					u2_tx_buf.use_buf2 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel7->CMAR = (u32)u2_tx_buf.buf2;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel7->CNDTR = u2_tx_buf.buf2_cnt;
					//DMA_Cmd(DMA1_Channel5, ENABLE);
				    DMA1_Channel7->CCR |= DMA_CCR1_EN;
				}
				else {
					if ( u2_tx_buf.buf2_cnt > TX_BUF_SIZE -2 ) {//overflow
						u2_tx_buf.buf2_cnt--;
						return false;
					}
				}
			return true;
			}
		
			OSTimeDlyHMSM(0, 0,	2, 0); //wait 2 sec.
		
			return false;

		default://no this COM port
			return false;
		//break;
	}
End Using DMA ***/
}

//***************************************
//发送一个定义在程序存储区的字符串到串口
bool putstring(COM_TypeDef COM, unsigned char  *puts)
{	
	u8 retry = 0 ;

	for (;*puts!=0;puts++) {  //遇到停止符0结束
		retry = 0 ;
		while ( (!putbyte(COM, *puts)) && retry < 10 ) {
			//error, maybe buffer full
			retry++;
			OSTimeDlyHMSM(0, 0,	retry, 0); //wait xx sec.
		}

		if ( retry == 10 ) {
			return false;
		}
	}
	return true;
}

//*************************************
//check the  buffer empty flag first if you don't want to wait
unsigned char getbyte ( COM_TypeDef COM )
{
	unsigned char c1, c2, c3 ;//防止重入时出问题

	switch( COM ){
		case COM1:
			while ( u1_rx_buf.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		
			c1= *(u1_rx_buf.out_last); //get the data
			u1_rx_buf.out_last++;
			u1_rx_buf.full = false; //reset the full flag
		
			if (u1_rx_buf.out_last==u1_rx_buf.buf+RX_BUF_SIZE) {
				u1_rx_buf.out_last = u1_rx_buf.buf; //地址到顶部,回到底部
			}
		
			if (u1_rx_buf.out_last==u1_rx_buf.in_last) {
				u1_rx_buf.empty = true ;//set the empty flag
			}
		
			USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		
			return (c1);
			//break;

		case COM2:		
			while ( u2_rx_buf.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
		
			c2= *(u2_rx_buf.out_last); //get the data
			u2_rx_buf.out_last++;
			u2_rx_buf.full = false; //reset the full flag
		
			if (u2_rx_buf.out_last==u2_rx_buf.buf+RX_BUF_SIZE) {
				u2_rx_buf.out_last = u2_rx_buf.buf; //地址到顶部,回到底部
			}
		
			if (u2_rx_buf.out_last==u2_rx_buf.in_last) {
				u2_rx_buf.empty = true ;//set the empty flag
			}
		
			USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
		
			return (c2);
			//break;

		case COM3:
			while ( u3_rx_buf.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		
			c3= *(u3_rx_buf.out_last); //get the data
			u3_rx_buf.out_last++;
			u3_rx_buf.full = false; //reset the full flag
		
			if (u3_rx_buf.out_last==u3_rx_buf.buf+RX_BUF_SIZE) {
				u3_rx_buf.out_last = u3_rx_buf.buf; //地址到顶部,回到底部
			}
		
			if (u3_rx_buf.out_last==u3_rx_buf.in_last) {
				u3_rx_buf.empty = true ;//set the empty flag
			}
		
			USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
		
			return (c3);
			//break;

		default://no this COM port
			return 0;
		//break;
	}
}


PUTCHAR_PROTOTYPE
{
	u8 retry = 0;
/*
	USART1->DR = (u8) ch;

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}
*/	
	while ( (! putbyte( COM1, ch )) && retry < 100 ) {//use DMA, put to buffer first
		//error, maybe buffer full
		retry++;
		OSTimeDlyHMSM(0, 0,	0, 10); //wait 10 msec.
	}

	if ( retry == 100 ) {
		while ( 1 ) USART1->DR ='E' ;
	}
	else {
		retry = 0 ;
		return ch;
	}
}
