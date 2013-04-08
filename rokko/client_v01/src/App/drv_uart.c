#include "main.h"

extern struct ICAR_DEVICE my_icar;
extern OS_EVENT 	*sem_obd_prot	;



#define ClearUsartError()	if(SR&0x1F)USART3->DR


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
	my_icar.stm32_u1_tx.buf1_cnt = 0;
	my_icar.stm32_u1_tx.buf2_cnt = 0;
	my_icar.stm32_u1_tx.use_buf1 = false;
	my_icar.stm32_u1_tx.use_buf2 = false;
	*/

	/* Using interrupt */
	my_icar.stm32_u1_tx.out_last = my_icar.stm32_u1_tx.buf;
	my_icar.stm32_u1_tx.in_last  = my_icar.stm32_u1_tx.buf;
	my_icar.stm32_u1_tx.empty = true;

	my_icar.stm32_u1_rx.out_last = my_icar.stm32_u1_rx.buf;
	my_icar.stm32_u1_rx.in_last  = my_icar.stm32_u1_rx.buf;
	my_icar.stm32_u1_rx.full  = false;
	my_icar.stm32_u1_rx.empty = true;
	my_icar.stm32_u1_rx.lost_data= false;

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
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)my_icar.stm32_u1_tx.buf1;
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

void data_init( void ) 
{
			 
	my_icar.stm32_data_tx.out_last = my_icar.stm32_data_tx.buf;
	my_icar.stm32_data_tx.in_last  = my_icar.stm32_data_tx.buf;
	my_icar.stm32_data_tx.empty = true;

	my_icar.stm32_data_rx.out_last = my_icar.stm32_data_rx.buf;
	my_icar.stm32_data_rx.in_last  = my_icar.stm32_data_rx.buf;
	my_icar.stm32_data_rx.full  = false;
	my_icar.stm32_data_rx.empty = true;
	my_icar.stm32_data_rx.lost_data= false;

}



void uart2_init( void ) 
{
	USART_InitTypeDef USART2_InitStructure;
	//DMA_InitTypeDef DMA_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//Init Global vars
	/* Using DMA
	my_icar.stm32_u2_tx.buf1_cnt = 0;
	my_icar.stm32_u2_tx.buf2_cnt = 0;
	my_icar.stm32_u2_tx.use_buf1 = false;
	my_icar.stm32_u2_tx.use_buf2 = false;
	*/

	/* Using interrupt */
	my_icar.stm32_u2_tx.out_last = my_icar.stm32_u2_tx.buf;
	my_icar.stm32_u2_tx.in_last  = my_icar.stm32_u2_tx.buf;
	my_icar.stm32_u2_tx.empty = true;

	my_icar.stm32_u2_rx.out_last = my_icar.stm32_u2_rx.buf;
	my_icar.stm32_u2_rx.in_last  = my_icar.stm32_u2_rx.buf;
	my_icar.stm32_u2_rx.full  = false;
	my_icar.stm32_u2_rx.empty = true;
	my_icar.stm32_u2_rx.lost_data= false;

	/* USARTx configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 , ENABLE);

    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

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
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)my_icar.stm32_u2_tx.buf1;
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
	my_icar.stm32_u3_tx.out_last = my_icar.stm32_u3_tx.buf;
	my_icar.stm32_u3_tx.in_last  = my_icar.stm32_u3_tx.buf;
	my_icar.stm32_u3_tx.empty = true;

	my_icar.stm32_u3_rx.out_last = my_icar.stm32_u3_rx.buf;
	my_icar.stm32_u3_rx.in_last  = my_icar.stm32_u3_rx.buf;
	my_icar.stm32_u3_rx.full  = false;
	my_icar.stm32_u3_rx.empty = true;
	my_icar.stm32_u3_rx.lost_data= false;

	/* USARTx configured as follow:
	      - BaudRate = 10416 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,  ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOB| RCC_APB2Periph_AFIO , ENABLE);
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

		/* LINE TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//L LINE TX ENABLE
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC,GPIO_Pin_6);

}

void kline_init( void ) 
{
	//KWP_PM_ON;
	uart3_init();
}



int SendToEcu(u8* pBuffer, int iLength)
{
	int i=0;
	u16 SR=USART3->SR;
	unsigned int tx_timer;


//	USART3->CR1&=~0x04;
	while(i<iLength){
		
		//USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
		USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
		USART3->DR = pBuffer[i];
		i+=1;
		while(1){
			SR=USART3->SR;
			if(SR&USART_FLAG_TXE)break;
			ClearUsartError();
		};
	   if(i<iLength)OSTimeDlyHMSM(0, 0,	0, 5);;//--5ms
	}
	tx_timer = OSTime;
	do{
	   if((OSTime-tx_timer)>1*OS_TICKS_PER_SEC)return 0; // 1 sec
	}
	while(iLength>(my_icar.stm32_u3_rx.in_last-my_icar.stm32_u3_rx.buf));

	return iLength;
}

int ReceiveFromEcu(u8 *pBuffer, int iLength,int iTimeout)
{
	int iCount=0;
	CPU_INT08U	os_err;
	for(iCount=0;iCount<iLength;iCount++){
	my_icar.event = 1;
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	OSSemPend(sem_obd_prot, iTimeout*OS_TICKS_PER_SEC/1000, &os_err);//timeout: 1 seconds
	USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
	if(os_err)break ;
	my_icar.stm32_u1_rx.in_last--;
	*(pBuffer+iCount)=*(my_icar.stm32_u1_rx.in_last);
	}

	return iCount;
  
/*	INT32U rx_timer = OSTime;
	int iCount=0;

	while(iCount<iLength){
		u16 SR=USART3->SR;
		if(SR&USART_FLAG_RXNE){
			pBuffer[iCount++]=(u8)(USART3->DR);

			if(iCount==iLength)break;
			
		}
		else{
			ClearUsartError();
		}
		if((OSTime-rx_timer)>1*OS_TICKS_PER_SEC/10)break;
   }
      return iCount;
  */ 


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

		    while ( (((my_icar.stm32_u1_tx.out_last-my_icar.stm32_u1_tx.in_last) < 2) \
					&& (my_icar.stm32_u1_tx.out_last > my_icar.stm32_u1_tx.in_last ))\
					|| ((my_icar.stm32_u1_tx.out_last < my_icar.stm32_u1_tx.in_last) \
					&& (TX_BUF_SIZE-(my_icar.stm32_u1_tx.in_last-my_icar.stm32_u1_tx.out_last) < 2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 10); //wait 10 msecond, send ~100Bytes@115200
				//USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			*(my_icar.stm32_u1_tx.in_last) = ch ; //put to send buffer
			my_icar.stm32_u1_tx.in_last++;
			if ( my_icar.stm32_u1_tx.in_last == my_icar.stm32_u1_tx.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				my_icar.stm32_u1_tx.in_last  = my_icar.stm32_u1_tx.buf;
			}

			my_icar.stm32_u1_tx.empty = false;
			//start transmit
			USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

			return true;

		case COM2:
		    while ( (((my_icar.stm32_u2_tx.out_last-my_icar.stm32_u2_tx.in_last) < 2) \
					&& (my_icar.stm32_u2_tx.out_last > my_icar.stm32_u2_tx.in_last ))\
					|| ((my_icar.stm32_u2_tx.out_last < my_icar.stm32_u2_tx.in_last) \
					&& (TX_BUF_SIZE-(my_icar.stm32_u2_tx.in_last-my_icar.stm32_u2_tx.out_last) < 2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 10); //wait 10 msecond, send ~100Bytes@115200
				//USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			*(my_icar.stm32_u2_tx.in_last) = ch ; //put to send buffer
			my_icar.stm32_u2_tx.in_last++;
			if ( my_icar.stm32_u2_tx.in_last == my_icar.stm32_u2_tx.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				my_icar.stm32_u2_tx.in_last  = my_icar.stm32_u2_tx.buf;
			}

			my_icar.stm32_u2_tx.empty = false;
			//start transmit
			USART_ITConfig(USART2, USART_IT_TXE, ENABLE);

			return true;

		case COM3:

		    while ( (((my_icar.stm32_u3_tx.out_last-my_icar.stm32_u3_tx.in_last) < 2) \
					&& (my_icar.stm32_u3_tx.out_last > my_icar.stm32_u3_tx.in_last ))\
					|| ((my_icar.stm32_u3_tx.out_last < my_icar.stm32_u3_tx.in_last) \
					&& (TX_BUF_SIZE-(my_icar.stm32_u3_tx.in_last-my_icar.stm32_u3_tx.out_last) < 2))) {
				//中断发送不及，缓存即将满时...
				USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
				//延时，把缓存发送出去
				OSTimeDlyHMSM(0, 0,	0, 10); //wait 10 msecond, send ~100Bytes@115200
				//USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			}                     

			USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			*(my_icar.stm32_u3_tx.in_last) = ch ; //put to send buffer
			my_icar.stm32_u3_tx.in_last++;
			if ( my_icar.stm32_u3_tx.in_last == my_icar.stm32_u3_tx.buf + TX_BUF_SIZE) {
				//指针到了顶部换到底部
				my_icar.stm32_u3_tx.in_last  = my_icar.stm32_u3_tx.buf;
			}

			my_icar.stm32_u3_tx.empty = false;
			//start transmit
			USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

			return true;

		default://no this COM port
			return false;
	}

/* Using DMA ***
	switch( COM ){
		case COM1:

			if ( !my_icar.stm32_u1_tx.use_buf1 ) {//buf1 free, add to buf1
		
				my_icar.stm32_u1_tx.buf1[my_icar.stm32_u1_tx.buf1_cnt] = ch;
				my_icar.stm32_u1_tx.buf1_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel4->CCR & 0x01)){
					//start DMA with buf1
					my_icar.stm32_u1_tx.use_buf1 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel4->CMAR = (u32)my_icar.stm32_u1_tx.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel4->CNDTR = my_icar.stm32_u1_tx.buf1_cnt;
					//DMA_Cmd(DMA1_Channel4, ENABLE);
				    DMA1_Channel4->CCR |= DMA_CCR1_EN;
				}
				else {		
					if ( my_icar.stm32_u1_tx.buf1_cnt > TX_BUF_SIZE -2 ) {//overflow
						my_icar.stm32_u1_tx.buf1_cnt--;
						return false;
					}
				}
			return true;
			}
		
			if ( !my_icar.stm32_u1_tx.use_buf2 ) {//buf2 free, add to buf2
		
				my_icar.stm32_u1_tx.buf2[my_icar.stm32_u1_tx.buf2_cnt] = ch;
				my_icar.stm32_u1_tx.buf2_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel4->CCR & 0x01)){
					//start DMA with buf2
					my_icar.stm32_u1_tx.use_buf2 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel4->CMAR = (u32)my_icar.stm32_u1_tx.buf2;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel4->CNDTR = my_icar.stm32_u1_tx.buf2_cnt;
					//DMA_Cmd(DMA1_Channel4, ENABLE);
				    DMA1_Channel4->CCR |= DMA_CCR1_EN;
				}
				else {	
					if ( my_icar.stm32_u1_tx.buf2_cnt > TX_BUF_SIZE -2 ) {//overflow
						my_icar.stm32_u1_tx.buf2_cnt--;
						return false;
					}
				}
			return true;
			}
		
			OSTimeDlyHMSM(0, 0,	2, 0); //wait 2 sec.
		
			return false;

		case COM2:

			if ( !my_icar.stm32_u2_tx.use_buf1 ) {//buf1 free, add to buf1
		
				my_icar.stm32_u2_tx.buf1[my_icar.stm32_u2_tx.buf1_cnt] = ch;
				my_icar.stm32_u2_tx.buf1_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel7->CCR & 0x01)){
					//start DMA with buf1
					my_icar.stm32_u2_tx.use_buf1 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel7->CMAR = (u32)my_icar.stm32_u2_tx.buf1;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel7->CNDTR = my_icar.stm32_u2_tx.buf1_cnt;
					//DMA_Cmd(DMA1_Channel5, ENABLE);
				    DMA1_Channel7->CCR |= DMA_CCR1_EN;
				}
				else {
					if ( my_icar.stm32_u2_tx.buf1_cnt > TX_BUF_SIZE -2 ) {//overflow
						my_icar.stm32_u2_tx.buf1_cnt--;
						return false;
					}
				}
			return true;
			}
		
			if ( !my_icar.stm32_u2_tx.use_buf2 ) {//buf2 free, add to buf2
		
				my_icar.stm32_u2_tx.buf2[my_icar.stm32_u2_tx.buf2_cnt] = ch;
				my_icar.stm32_u2_tx.buf2_cnt++;
				//If DMA is Disabled
				if(!(DMA1_Channel7->CCR & 0x01)){
					//start DMA with buf2
					my_icar.stm32_u2_tx.use_buf2 = true;
		
					//write DMA Channelx CMAR to configure BaseAddr
					DMA1_Channel7->CMAR = (u32)my_icar.stm32_u2_tx.buf2;
					//Write to DMA Channelx CNDTR to configure buffer size
					DMA1_Channel7->CNDTR = my_icar.stm32_u2_tx.buf2_cnt;
					//DMA_Cmd(DMA1_Channel5, ENABLE);
				    DMA1_Channel7->CCR |= DMA_CCR1_EN;
				}
				else {
					if ( my_icar.stm32_u2_tx.buf2_cnt > TX_BUF_SIZE -2 ) {//overflow
						my_icar.stm32_u2_tx.buf2_cnt--;
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

		if ( retry >= 10 ) {
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
			while ( my_icar.stm32_u1_rx.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		
			c1= *(my_icar.stm32_u1_rx.out_last); //get the data
			my_icar.stm32_u1_rx.out_last++;
			my_icar.stm32_u1_rx.full = false; //reset the full flag
		
			if (my_icar.stm32_u1_rx.out_last==my_icar.stm32_u1_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u1_rx.out_last = my_icar.stm32_u1_rx.buf; //地址到顶部,回到底部
			}
		
			if (my_icar.stm32_u1_rx.out_last==my_icar.stm32_u1_rx.in_last) {
				my_icar.stm32_u1_rx.empty = true ;//set the empty flag
			}
		
			USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		
			return (c1);
			//break;

		case COM2:		
			while ( my_icar.stm32_u2_rx.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
		
			c2= *(my_icar.stm32_u2_rx.out_last); //get the data
			my_icar.stm32_u2_rx.out_last++;
			my_icar.stm32_u2_rx.full = false; //reset the full flag
		
			if (my_icar.stm32_u2_rx.out_last==my_icar.stm32_u2_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u2_rx.out_last = my_icar.stm32_u2_rx.buf; //地址到顶部,回到底部
			}
		
			if (my_icar.stm32_u2_rx.out_last==my_icar.stm32_u2_rx.in_last) {
				my_icar.stm32_u2_rx.empty = true ;//set the empty flag
			}
		
			USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
		
			return (c2);
			//break;

		case COM3:
			while ( my_icar.stm32_u3_rx.empty ) {//buffer empty, wait...
				OSTimeDlyHMSM(0, 0,	0, 100); //wait 100 msec.
			}
		     
			USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		
			c3= *(my_icar.stm32_u3_rx.out_last); //get the data
			my_icar.stm32_u3_rx.out_last++;
			my_icar.stm32_u3_rx.full = false; //reset the full flag
		
			if (my_icar.stm32_u3_rx.out_last==my_icar.stm32_u3_rx.buf+RX_BUF_SIZE) {
				my_icar.stm32_u3_rx.out_last = my_icar.stm32_u3_rx.buf; //地址到顶部,回到底部
			}
		
			if (my_icar.stm32_u3_rx.out_last==my_icar.stm32_u3_rx.in_last) {
				my_icar.stm32_u3_rx.empty = true ;//set the empty flag
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

	if ( retry >= 100 ) {
		while ( 1 ) USART1->DR ='E' ;
	}
	else {
		retry = 0 ;
		return ch;
	}
}
