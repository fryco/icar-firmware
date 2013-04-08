#include "driver_kwp2k.h"
#include "driver_tick.h"
#include "driver_led.h"
#include "app_update.h"
#include "stdio.h"

#define USART2_RX_BUF_SIZE 64
#define USART2_TX_BUF_SIZE 64

u8 USART2RxBuf[USART2_RX_BUF_SIZE];
u16 USART2RxBufGetPos;

u8 USART2TxBuf[USART2_TX_BUF_SIZE];
u16 USART2TxBufPutPos;
u16 USART2TxBufGetPos;

u32 KWPBaud=10400;
u16 KWPBits=USART_WordLength_8b;
u16 KWPStopBits=USART_StopBits_1;
u16 KWPParity=USART_Parity_No;

u32 KWPTxFrameSpaceUs=1000;
u32 KWPTxByteSpaceUs=500;
u32 KWPRxCutFrameMeothd=KWP_RX_FRAME_METHOD_BYTIME;
u32 KWPRXCutFrameTimeUs=10;
u32 KWPOneByteTakeUs;
u16 USART2RxBufGetPos_temp;

void KWPInit(void)
{
	USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	USART_ClockInitTypeDef  USART_ClockInitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;
	int bits;
//	int i;

	USART2RxBufGetPos=0;

	USART2TxBufGetPos=0;
	USART2TxBufPutPos=0;

	//Open USART2 clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    //Open DMA clock
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	//Open GPIO clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | 
            RCC_APB2Periph_AFIO ,
            ENABLE);
	//Power
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	//V2 CONFIG
	//GPIO_Init(GPIOB, &GPIO_InitStructure);				
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	KWP_PWR_ON;

	//RX_REF: PB.9
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	KWP_RX_REF_LOW;

	//TX1_PU
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//	GPIO_Init(GPIOC, &GPIO_InitStructure);
//	KWP_TX1_UP_ON;

	//TX2_PU
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	KWP_TX2_UP_ON;

	//TX2_EN
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	KWP_TX2_EN_ON;

	//UART2: Configure USART2 Tx (PA.2) as alternate function push-pull
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//Configure USART2 Rx (PA.3) as input floating
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//DMA Init for USART2_RX
//    DMA_DeInit(DMA1_Channel6);
//    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)USART2+4;
//    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)USART2RxBuf;
//    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//    DMA_InitStructure.DMA_BufferSize = USART2_RX_BUF_SIZE;
//    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
//    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
//    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//    DMA_Init(DMA1_Channel6, &DMA_InitStructure);
//for(i=0;i<10;i++)
//{
//	USART2RxBuf[3+i] = 0xae;
//}
	//DMA Init for USART3_RX
    DMA_DeInit(DMA1_Channel3);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)USART3+4;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)USART2RxBuf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = USART2_RX_BUF_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);					//通道3是usart3的接收

	//Set USART2_RX to DMA mode
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

 	//Start DMA Transfer
   	DMA_Cmd(DMA1_Channel3, ENABLE);

	//Init USART1
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;

	//Configure the USART2 synchronous paramters
	USART_ClockInit(USART3, &USART_ClockInitStructure);

	//Configure USART1 basic and asynchronous paramters
	USART_InitStructure.USART_BaudRate = KWPBaud;
	USART_InitStructure.USART_WordLength = KWPBits;
	USART_InitStructure.USART_StopBits = KWPStopBits;
	USART_InitStructure.USART_Parity = KWPParity;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure);

	//Calc count the bit count for a word	
	bits=1;	//Startup bit
	if(USART_InitStructure.USART_WordLength==USART_WordLength_8b){
		bits+=8;//data_bits + parity_bit
	}else{
		bits+=9;//data_bits + parity_bit
	}
	switch(USART_InitStructure.USART_StopBits){
	case USART_StopBits_0_5:
		bits+=0.5;
		break;
	case USART_StopBits_1:
		bits+=1;
		break;
	case USART_StopBits_1_5:
		bits+=1.5;
		break;
	case USART_StopBits_2:
		bits+=2;
		break;
	}
	KWPOneByteTakeUs=(u32)(
		bits * 1000000.0 / (double)USART_InitStructure.USART_BaudRate
		+0.5);	//四舍五入

	//Enable USART3
	USART_Cmd(USART3, ENABLE);
}

void KWPUninit(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, DISABLE);

    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);

    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, DISABLE);

	USART_Cmd(USART3, DISABLE);

	KWP_PWR_OFF;

	//KWP_TX1_UP_OFF;
	//KWP_TX2_UP_OFF;

	KWP_TX2_EN_OFF;
}

bool KWPSendFrame(u8* pdata, u16 length)
{
	int num;
	char buf[4];
	
	if(!KWPWaitBusFree()){
		return false;
	}

	USART3->CR1 &= (~0x00000004);

	KWPSendBytes(pdata,length);

	USART3->CR1 |= 0x04;

	LEDExecTask(LED_ID_VCM2CAR, LED_TASK_SHORT_ON);

	return true;
}


u16 KWPReceiveFrameByAutoLength(u8 * pbuffer, u16 bufsize, u32 timeout)
{
	u32 len;
	u8 b;

	if(1 != KWPReceiveBytes(&b,1,timeout)){
		return 0;
	}	

	if( (b&0xC0)==0x00 ){	//03 SID DAT
		len=b+1;
	}else if((b&0x3F)==0x00){	//0x80 LEN TRG SRC
		if(1 != KWPReceiveBytes(&b,1,timeout)){
			return 0;
		}
		len=b+1;

		//Receive trg src, and discard.
		if(2 != KWPReceiveBytes(pbuffer,2,timeout)){
			return 0;
		}

	}else{	//0x83 TRG SRC
		len=(b&0x3F)+1;

		//Receive trg src, and discard.
		if(2 != KWPReceiveBytes(pbuffer,2,timeout)){
			return 0;
		}
	}


	if(bufsize<=len){
		return 0;
	}


	if(len != KWPReceiveBytes(pbuffer,len,timeout)){
		return 0;
	}

	LEDExecTask(LED_ID_CAR2VCM, LED_TASK_SHORT_ON);
	return len;
}

u16 ISOReceiveFrameByAutoLength(u8 * pbuffer, u16 bufsize, u32 timeout)
{
	u32 len = 0;
	u8 b = 0;
	u8 cs = 0;
	int i , num;
	char buf[4];

	if(1 != KWPReceiveBytes(&b,1,timeout)){
		return 0;
	}
	cs = b;

	
/*	u16 len;
	u8 b=0;
	u8 cs;
	int i , num;
	u8 buf[11];


	cs = 0;
	len = 0;
	num = 0;

	if(1 != KWPReceiveBytes(&b,1,timeout)){
		return 0;
	}*/
//	cs = b;
	
//	dbg_msg("first:");
//	dbg_msg((char*)&b);
//判断接受数据，如果是初始化部分收到的是0x55开头，否则是0x48开头
	if(b == 0x48)
	{
/*		buf[0] = b;
		//KWPReceiveBytes(&b,10,timeout);
		KWPReceiveBytes(buf + 1,10,timeout);

		for(i = 0;i < 10; i++)
		{
			cs += buf[i];
			if(cs == buf[i + 1])
			{
				len = i + 1;
			}
		}
		USART2RxBufGetPos = USART2RxBufGetPos_temp + len; 
		if(USART2RxBufGetPos >= USART2_RX_BUF_SIZE)
		{
			USART2RxBufGetPos -= USART2_RX_BUF_SIZE;
		}

		if(len <= 3)
			return 0;

		memcpy(pbuffer, buf + 3, len - 3);
		
		LEDExecTask(LED_ID_CAR2VCM, LED_TASK_SHORT_ON);
		return len;*/
		for(i=0;i<10 ; i++)
		{
			KWPReceiveBytes(&b,1,timeout);
			if(cs == b)
			{
				cs+=b;
				len = i-1;
			}
			else
				cs += b;
			
			if(i >= 2)
			{
				pbuffer[i-2] = b;
				if(i>=4 && pbuffer[i-2]==0x6b && pbuffer[i-3]==0x48)
				{
					USART2RxBufGetPos = USART2RxBufGetPos - 2;
					pbuffer[i-3] = 0;
					pbuffer[i-2] = 0;
					i = i-2;
					goto True_1;
				}
			}
		}
		if(i>len)
		{
			for(;i==len+2; i--)
				pbuffer[i-2] = 0;
		}
True_1:
		LEDExecTask(LED_ID_CAR2VCM, LED_TASK_SHORT_ON);
		return len;
	}
	if(b == 0xcc)
	{
		pbuffer[0] = b;
		len = 1;
		LEDExecTask(LED_ID_CAR2VCM, LED_TASK_SHORT_ON);
		return len;
	}
	if(b == 0x55)
	{
		USART2RxBufGetPos = USART2RxBufGetPos+2;
		
		len = 3;

//		LEDExecTask(LED_ID_CAR2VCM, LED_TASK_SHORT_ON);
		return len;
	}

	return 0;
		
}

u16 KWPReceiveFrameByAITime(u8 * pbuffer, u16 bufsize)
{

	return 0;
}

void KWPSendBytes(u8* pdata, u16 length)
{

	while(length){
		//while(USART_GetITStatus(USART1,USART_FLAG_TXE))
		while(!(USART3->SR & USART_FLAG_TXE));

		USART3->DR=*pdata;
		pdata++;

		length--;
	
		if(length)
			DelayUs(KWPOneByteTakeUs+KWPTxByteSpaceUs);
		else
			DelayUs(KWPOneByteTakeUs);	//Last word

	}
}

bool KWPSendQuickInit(u8 * pdata, u16 length)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if(!KWPWaitBusFree()){
		return false;
	}

	USART3->CR1 &= (~0x00000004);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOB->BRR=GPIO_Pin_10;
	DelayMs(25);
	GPIOB->BSRR=GPIO_Pin_10;
	DelayMs(25);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	KWPSendBytes(pdata,length);

	USART3->CR1 |= 0x04;

	return true;
}

bool ISOSendSlowInit()
{
	const int bufsize=32;
	u8 rcvbuf[bufsize];
	u8 len;

	GPIO_InitTypeDef GPIO_InitStructure;
											
	len = 0;

	if(!KWPWaitBusFree()){
		return false;
	}

	USART3->CR1 &= (~0x00000004);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOB->BRR=GPIO_Pin_10;
	DelayMs(200);
	GPIOB->BSRR=GPIO_Pin_10;
	DelayMs(400);
	GPIOB->BRR=GPIO_Pin_10;
	DelayMs(400);
	GPIOB->BSRR=GPIO_Pin_10;
	DelayMs(400);
	GPIOB->BRR=GPIO_Pin_10;
	DelayMs(400);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	dbg_msg("send 5 baut ok!");	   
	USART3->CR1 |= 0x04;
	
 	len=ISOReceiveFrameByAutoLength(rcvbuf,bufsize,800);
	
	if(len!=3){
		dbg_msg("~~~no 0x55!");
		return false;
	}

	DelayMs(40);
	
	if(!KWPSendFrame("\xf7",1))
	{
		dbg_msg("sed error!");
		return false;
	}
	if(!ISOReceiveFrameByAutoLength(rcvbuf,bufsize,1000))
	{
		dbg_msg("rec error!");
		DelayMs(1000);
		return false;
	}
	dbg_msg("f7:");
	dbg_msg(rcvbuf);
	DelayMs(100);
//	KWPSendBytes(pdata,length);

	return true;
}

bool KWPSendSlowInit(u8 * pdata, u16 length)
{
	if(!KWPWaitBusFree()){
		return false;
	}

	USART3->CR1 &= (~0x00000004);

	KWPSendBytes(pdata,length);

	USART3->CR1 |= 0x04;

	return true;
}

bool KWPHardwareSelfTest(void)
{
	u8 buf[3];
	KWPSendBytes("\xa5\x55\x5a", 3);
	if(!KWPReceiveBytes(buf, 3, 500)){
		return false;
	}

	if(buf[0]!=0xa5) return false;
	if(buf[1]!=0x55) return false;
	if(buf[2]!=0x5a) return false;

	return true;
}


u16 KWPReceiveBytes(u8* pbuffer, u16 length, u32 timeout)
{
	u16 received=0;
	
	u32 tick=TickGetMs()+timeout;	
	
	while(received<length){
		if(USART2RxBufGetPos!= USART2_RX_BUF_SIZE-DMA1_Channel3->CNDTR){
			*(pbuffer + received)=USART2RxBuf[USART2RxBufGetPos];
			received++;
			USART2RxBufGetPos++;
			if(USART2RxBufGetPos>=USART2_RX_BUF_SIZE){
				USART2RxBufGetPos=0;				
			}							
		}		
		if(TickGetMs()>tick)
			return 0;
	}

	return received;
}

bool KWPWaitBusFree(void)
{
	u32 tick;
	u32 max_wait_tick=TickGetMs()+3000;

	tick=TickResetUs()+KWPTxFrameSpaceUs;
	while(TickGetUs()<tick){
		if(!(GPIOB->IDR & GPIO_Pin_11)){
			tick=TickResetUs()+KWPTxFrameSpaceUs;
		}
		if(TickGetMs()>max_wait_tick){
			return false;
		}
	}
	
	return true;
}

void KWPClearRx(void)
{
	
}


