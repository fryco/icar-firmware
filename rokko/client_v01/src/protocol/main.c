#include "stm32f10x_lib.h"
#include "init.h"
#include "util.h"
#include "queue.h"
#include "packet.h"
#include "box.h"
#include "prot.h"
#include "absacc.h"
#include "string.h"
#include "dlc.h"
#include "encdec.h"
#include "debug.h"
#include "link.h"
#include "other.h"

#ifdef _VCI2
#define SW	0x8888u
#define HW	0x2000u
const u32 MyVER   __at(0x8003000) = (SW<<16)|HW;
const u8  MyDATE[]  __at(0x8003004) = __DATE__;
const u8  MyTIME[]  __at(0x8003010) = __TIME__;
#endif

#define MyVer	0x08003000
#define MyDate	0x08003004
#define MyTime	0x08003010	   
#define MySn	0x08001020	   
//--------------------------------------------------------------------------------

#define SN_LEN	16
u8 GetSerialNo(void)
{
	char DataBuf[SN_LEN];
	memcpy(DataBuf,(u8*)MySn,SN_LEN);
	SeqEncrypt(0xD6AAB5C0,DataBuf,SN_LEN);
	AddFrameToBuffer((u8*)DataBuf,SN_LEN);
	return 2;
}
u8 GetCpuId(void)
{
	u8 i=0,DataBuf[12];
	while(i<12){
		DataBuf[i]=*(vu8*)(0x1FFFF7E8+i);
		i++;
	}
	AddFrameToBuffer(DataBuf,12);
	return 2;
}
u8 GetIoStatus(void)
{
	u8 DataBuf=0x00;
	u16 sta=MCU_K_RECEIVE_SIGNAL;
	if(sta)DataBuf=0xff;
	AddFrameToBuffer(&DataBuf,1);
	return 2;
}
u8 GetBaudrate(void)
{
	u8 DataBuf[4];
	DataBuf[0]=(u8)(g_box.baudrate>>24);
	DataBuf[1]=(u8)(g_box.baudrate>>16);
	DataBuf[2]=(u8)(g_box.baudrate>>8);
	DataBuf[3]=(u8)(g_box.baudrate>>0);
	AddFrameToBuffer(DataBuf,4);
	return 2;
}
u8 GetIoAdc(void)
{
	u8 DataBuf[2];
	u16 VOL;
	ADC1_Configuration();
	ADC_SoftwareStartConvCmd(ADC1,ENABLE); //使能或者失能指定的ADC的软件转换启动功能
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET);//检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
	VOL=ADC_GetConversionValue(ADC1);
	DataBuf[0]=	(u8)(VOL>>8);
	DataBuf[1]=	(u8)VOL;
	AddFrameToBuffer(DataBuf,2);
	return 2;
}
u8 TestUsart3(void)
{
	u8 DataBuf[2]={0x00,0x00};
	USART_Cmd(USART3, DISABLE);
	SetIoMode(GPIOB,GPIO_Pin_10,GPIO_Mode_Out_PP);
	GPIO_SetBits(GPIOB,GPIO_Pin_10);
	Delay(100);
	if(GetLevel(GPIOB,GPIO_Pin_11))DataBuf[0]=0xff; 
   	GPIO_ResetBits(GPIOB,GPIO_Pin_10);
	Delay(100);
	if(GetLevel(GPIOB,GPIO_Pin_11))DataBuf[1]=0xff; 
	SetIoMode(GPIOB,GPIO_Pin_10,GPIO_Mode_AF_PP);
	USART_Cmd(USART3, ENABLE);
	AddFrameToBuffer(DataBuf,2);
	return 2;
}
u8 TestCan(void)
{
	u8 DataBuf=0x00; 
	if(CAN_Polling()==1) DataBuf=0xff; 
 	AddFrameToBuffer(&DataBuf,1);

	DataBuf=MCU_K_RECEIVE_SIGNAL;
	AddFrameToBuffer(&DataBuf,1);
	return 2;
}
typedef  void (*pFunction)(void);
void JumpBios(void)
{
	int i=0;
	u32 JumpAddress = *(vu32*) (0x08000000 + 4);
	pFunction Jump_To_Application = (pFunction) JumpAddress;
	SysTick_ITConfig(DISABLE);
	USART_ITConfig(USART1,USART_IT_RXNE,DISABLE);
	USART_ITConfig(USART2,USART_IT_RXNE,DISABLE);
	USART_ITConfig(USART3,USART_IT_RXNE,DISABLE);
	USART_Cmd(USART1, DISABLE);
	USART_Cmd(USART2, DISABLE);
	USART_Cmd(USART3, DISABLE);
	for(i=0;i<100;i++);
	NVIC_DeInit();
	__MSR_MSP(*(vu32*) 0x08000000);
	Jump_To_Application();
}
u8 ExecuteCmd(u16 cmd,u8 *param,u16 len)
{
	u8 ret=0;//0:负响应数据		1：正响应数据	  2:返回数据	  3:不返回数据
	switch(cmd){
	case CA_RESET_COMMBOX:
		ret=ResetCommbox(param,len);
		break;
	case CA_IO_Parameter:
		ret=SetCommPort(param,len);
		break;
	case CA_ECU_COMMUNICATION_BAUD_RATE:
		ret=SetBaudrate(param,len);
		break;   
	case CA_ECU_COMM_TIME_INTERVAL:
		ret=SetCommTime(param,len);
		break;	
	case CA_ECU_LINK_KEEP:
		ret=SetKeepLink(param,len); 
		break;
	case CA_LINK_KEEP_ENABLE_AND_DISABLE: 
		g_box.link_flag=param[0];
		KeepLinkState(param[0]);	
		ret=1; 
		break;
	case CA_IO_HI_LOW_Voltage:
		ret=SetCommLevel(param,len);
		break;
	case CA_ECU_commnication_model:
		ret=SetProtocol(param,len);
		break;
	case CA_RESPOSE_VERSION:
		AddFrameToBuffer((u8*)MyVer,4);
		ret=2;
		break;
	case CA_RESPOSE_SN:
		ret=GetSerialNo();
		break;
	case CA_RESPOSE_TIME_DATA: 
		AddFrameToBuffer((u8*)MyDate,11);
		ret=2;
		break;
	case CA_RESPOSE_CPU_ID: 
		ret=GetCpuId();
		break;
	case CA_DELAY_TIM_MS:
		Delay((param[0]<<8)|param[1]);
		ret=1;
		break;
	case CA_DELAY_RTC_MS:{
		u16	tm=(param[0]<<8)|param[1];
		ResetCounter();
		while(GetCounter()<tm);}
		ret=1;
		break;
	case CA_DELAY_TICK_MS:{
		u32 tm=Clock()+((param[0]<<8)|param[1]);
		while(Clock()<tm);}
		ret=1;
		break;
	case CA_DELAY_TIM_US:
		DelayXs((param[0]<<8)|param[1]);
		ret=1;
		break;
	case CA_RECEIVE_DATA_FROM_ECU:
		break;	
	case CA_SEND_M_FRAME_DATA:
		BeforeSendReceive();
		ret=SendReceive(param,len); 	
		break;
	case CA_SEND_5BPS_DATA:
		ret=AddrCodeEnter(param,len); 	
		break;
	case CA_SET_COMM_FILTER:
		ret=SetCommFilter(param,len);
		break;
	case CA_SET_CAN_Filter:
		ret=SetCanFilter(param,len);
	 	break;
	case CA_SET_CAN_FLOW_CTRL:
		ret=SetFlowControl(param,len);
		break;
	case CA_AFC_ENABLE_AND_DISABLE:
		g_box.afc_flag=param[0];
		ret=1;
		break;
	case CA_RESPOSE_STA:
		ret=1;
		break;
	case CA_RESPOSE_IO_STA:
		ret=GetIoStatus();
		break;
	case CA_RESPOSE_IO_ADC:
		ret=GetIoAdc();
		break;
	case CA_JMP_BOOT:
		JumpBios();
		ret=1;
		break;
	case CA_JMP_MCU:
		ret=1;
		break;
	case CA_SET_BUSY_MODE:
		ret=SetBusyCount(param,len);
		break;
	case CA_RESPOSE_BPS:
		ret=GetBaudrate();
	 	break;
	case CA_RESPOSE_5CONVERT8V:
		GPIO_SetBits(GPIOA,GPIO_Pin_15);
		ret=1;
		break;
	case CA_RESPOSE_SELTEST_485:
		ret=1;
		break;
	case CA_RESPOSE_SELFTEST_CAN:
		ret=TestCan();
		break;
	case CA_RESPOSE_TEST_RXD:
		ret=GetIoStatus();
		break;
	case CA_RESPOSE_TEST_TXDRXD3:
		ret=TestUsart3();
		break;
	case CA_SET_WORRMODE:
		ret=SetWorkMode(param,len);
		break;
	case CA_SEEK_BPS:
		ret=AutoSeekBps(param,len);
		break;
	case CA_READ_FLASH_CODE:
		ret=FlashCode(param,len);
		break;
	case CA_QUICK_FLASHCODE:
		ret=QuickFlashCode(param,len);
		break;
	}
	return ret;
}
void processCmd(u8 *buffer)
{
	u8 tmp,ret=3;
	int index = 0;
	u8 *pbData=buffer+1;
	InitFrameBuffer();	 
	while(index < buffer[0]){
		tmp=ExecuteCmd((pbData[2]<<8)|pbData[3],pbData+4,((pbData[0]<<8)|pbData[1])-2);
		if(ret!=2)ret=tmp;//若有发送数据命令，返回值为2
		pbData+=2+((pbData[0]<<8)|pbData[1]);
		index++;
	}
	switch(ret){
#ifdef _VCI2 
	case 0:
		SendPacket(0,(u8*)"\x00\x02\x00\x01",4);
		break;
	case 1:
		SendPacket(0,(u8*)"\x00\x02\x00\x00",4);
		break;
	case 2:
		if(g_box.frame_count==0)
			SendPacket(1,(u8*)"\xff\xff",2);
		else
			SendPacket(g_box.frame_count,g_box.frame_buff,g_box.frame_length);
		break;
#endif
	case 3:
		break;
	}
}
#ifdef _VCI2
void InitGPIO(u8 hw);
#define CMD_BUFFER_LENGTH	1+2+2+4+1024
int main(void)
{
	queue_init(SQ_HOST,COM1);
	queue_init(SQ_ECU,COM2);
	RCC_Configuration();
 	RTC_Configuration();
	GPIO_Configuration();
	SysTick_Configuration();
	TIM2_Configuration();
	USART_Configuration(USART1,115200,USART_WordLength_9b,USART_Parity_Even);
	USART_Configuration(USART2,10400,USART_WordLength_8b,USART_Parity_No);
	USART_Configuration(USART3,10400,USART_WordLength_8b,USART_Parity_No);

	InitBox();
	InitGPIO(HW_VCI2);
	g_box.fnInitCommPort();
	Delay(100);
	while(1){
		u8 buffer[CMD_BUFFER_LENGTH];
		if(RecvPacket(buffer,CMD_BUFFER_LENGTH))
			processCmd(buffer);
		else ExecKeepLink();

		switch(g_box.run_mode){
		case MODE_COLLECT:
			CollectData();
			break;
		case MODE_EMULATOR:
			EmulatorEcu();
			break;
		}
	}
}
#endif
