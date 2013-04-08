#include "stm32f10x_lib.h"
#include "box.h"
#include "prot.h"
#include "queue.h"
#include "init.h"
#include "util.h"
#include "packet.h"
#include "string.h"

static u8 s_init=1;
enum{EM_DEFAULT,EM_PULSE,EM_ADDRCODE};
void InitNormal(u8 *param,u16 len)
{
	switch(g_box.protocol){
	case PT_J1708:
	case PT_J1587:
	case PT_NORMAL:
	case PT_NISSAN_OLD:
	case PT_ISO:
	case PT_KWP:
	case PT_BOSCH:
	case PT_WABCOABS:
		USART_ITConfig(g_box.usart,USART_IT_RXNE,DISABLE);
		break;
	case PT_VPW:
	case PT_PWM:
		break;
	case PT_CAN:
	case PT_BMW_CAN:
	case PT_VOLVO_CAN:
	case PT_TP20_CAN:
	case PT_SINGEL_CAN:
		CAN_ITConfig(CAN_IT_FMP0,DISABLE);
		break;
	}
	queue_init(SQ_HOST,COM1);
	queue_init(SQ_ECU,COM2);
}
void InitCollect(u8 *param,u16 len)
{
	u8 i,data=1;
	switch(param[0]){
	case EM_PULSE:
		USART_Cmd(g_box.usart,DISABLE);
		for(i=0;i<len-1;i++){//高低电平个数
			while(MCU_K_RECEIVE_SIGNAL!=data){if(IsNewCommand())return;}
			data=!data;
		}
		USART_Cmd(g_box.usart,ENABLE);
		break;
	case EM_ADDRCODE:
		data=0;
		USART_Cmd(g_box.usart,DISABLE);
		while(MCU_K_RECEIVE_SIGNAL){if(IsNewCommand())return;}
		Delay(200);
		for(i=0;i<8;i++){
			{if(IsNewCommand())return;}
			Delay(100);
			if(MCU_K_RECEIVE_SIGNAL)data|=(1<<i);
			Delay(100);
		}
		Delay(180);
		while(!MCU_K_RECEIVE_SIGNAL){if(IsNewCommand())return;}
		i=AutoBpsBy55(g_box.out_time);
		USART_Cmd(g_box.usart,ENABLE);
		SendByte(data);
		if(i)SendByte(0x55);
		break;
	default:
		break;
	}
	switch(g_box.protocol){
	case PT_J1708:
	case PT_J1587:
	case PT_NORMAL:
	case PT_NISSAN_OLD:
	case PT_ISO:
	case PT_KWP:
	case PT_BOSCH:
	case PT_WABCOABS:
		data=g_box.usart->DR;
		if(g_box.usart==USART2)NVIC_Configuration(USART2_IRQChannel,0);
		else NVIC_Configuration(USART3_IRQChannel,0);
		USART_ITConfig(g_box.usart,USART_IT_RXNE,ENABLE);
		break;
	case PT_VPW:
	case PT_PWM:
		break;
	case PT_CAN:
	case PT_BMW_CAN:
	case PT_VOLVO_CAN:
	case PT_TP20_CAN:
	case PT_SINGEL_CAN:
		NVIC_Configuration(USB_LP_CAN_RX0_IRQChannel,0);
		CAN_ITConfig(CAN_IT_FMP0,ENABLE);
		break;
	}
}
void InitEmulator(u8 *param,u16 len)
{
	u8 i,data=1;
	switch(param[0]){
	case EM_PULSE:
		USART_Cmd(g_box.usart,DISABLE);
		for(i=0;i<len;i++){//高低电平个数
			while(MCU_K_RECEIVE_SIGNAL!=data){if(IsNewCommand())return;}
			data=!data;
		}
		USART_Cmd(g_box.usart,ENABLE);
		break;
	case EM_ADDRCODE:
		data=0;
		USART_Cmd(g_box.usart,DISABLE);
		while(MCU_K_RECEIVE_SIGNAL){if(IsNewCommand())return;}
		Delay(200);
		for(i=0;i<8;i++){
			{if(IsNewCommand())return;}
			Delay(100);
			if(MCU_K_RECEIVE_SIGNAL)data|=(1<<i);
			Delay(100);
		}
		Delay(180);
		while(!MCU_K_RECEIVE_SIGNAL){if(IsNewCommand())return;}
		USART_Cmd(g_box.usart,ENABLE);
		SendByte(data);
		break;
	default:
		break;
	}
	switch(g_box.protocol){
	case PT_J1708:
	case PT_J1587:
	case PT_NORMAL:
	case PT_NISSAN_OLD:
	case PT_ISO:
	case PT_KWP:
	case PT_BOSCH:
	case PT_WABCOABS:
		data=g_box.usart->DR;
		if(g_box.usart==USART2)NVIC_Configuration(USART2_IRQChannel,0);
		else NVIC_Configuration(USART3_IRQChannel,0);
		USART_ITConfig(g_box.usart,USART_IT_RXNE,ENABLE);
		break;
	case PT_VPW:
	case PT_PWM:
		break;
	case PT_CAN:
	case PT_BMW_CAN:
	case PT_VOLVO_CAN:
	case PT_TP20_CAN:
	case PT_SINGEL_CAN:
		NVIC_Configuration(USB_LP_CAN_RX0_IRQChannel,0);
		CAN_ITConfig(CAN_IT_FMP0,ENABLE);
		break;
	}
}
static u8 s_param[32],s_len;
u8 SetWorkMode(u8 *param,u16 len)
{
	g_box.run_mode=param[0];
	InitNormal(param+1,len-1);
	s_init=0;
	memcpy(s_param,param+1,len-1);
	s_len=len;
	return 1;
}
void VPWM_Produre(void)
{
	int i,iLength=0;
	u8 pRecvFrame[128];
	if(g_box.protocol==PT_VPW)iLength=VPW_RecvFrame(pRecvFrame);
	else iLength=PWM_RecvFrame(pRecvFrame);
	for(i=0;i<iLength;i++)SendByte(pRecvFrame[i]);	
}
void CollectData(void)
{
	u8 data;
	u16 i,size;
	if(s_init==0){
		InitCollect(s_param,s_len);
		s_init=1;
	}
	if(g_box.protocol==PT_VPW||g_box.protocol==PT_PWM)
		VPWM_Produre();
	else{
		size=queue_size(SQ_ECU);
		for(i=0;i<size;i++){
			data=queue_get(SQ_ECU);
			SendByte(data);	
		}
	}
}
void EmulatorEcu(void)
{
	u8 data;
	u16 i,size;
	if(s_init==0){
		InitEmulator(s_param,s_len);
		s_init=1;
	}
	if(g_box.protocol==PT_VPW||g_box.protocol==PT_PWM)
		VPWM_Produre();
	else{
		size=queue_size(SQ_ECU);
		for(i=0;i<size;i++){
			data=queue_get(SQ_ECU);
			SendByte(data);	
		}
	}
}


