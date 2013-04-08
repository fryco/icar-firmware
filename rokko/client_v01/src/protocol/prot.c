#include "stm32f10x_lib.h"
#include "prot.h"
#include "dlc.h"
#include "util.h"
#include "string.h"
#include "box.h"
#include "stdio.h"
#include "debug.h"
#include "packet.h"
#include "link.h"
#include "queue.h"

#define ClearUsartError()	if(SR&0x1F)g_box.usart->DR

void SendDataWaitIdle(void)
{
	u8 count=0;
	if(g_box.IsWaitBeforeSendFrame==0)return;
	ResetCounter();
	while(1){
		u16 SR=g_box.usart->SR;
		if(SR&USART_FLAG_RXNE){
			g_box.usart->DR;
			ResetCounter();
			count++;
		}
		else{
			ClearUsartError();
		}
		if(GetCounter()>=g_box.f2f_time)break;
		if(count==100)break;
//		if(IsNewCommand())break;
   	}
}
u8 WaitDataFromEcu(void)
{
	ResetCounter();
	while(1){
		u16 SR=g_box.usart->SR;
		if(SR&USART_FLAG_RXNE){
			break;
		}
		else{
			ClearUsartError();
		}
		if(GetCounter()>g_box.out_time)return 0;
		if(IsNewCommand())break;
   	}
	return 1;
}
int SendToEcu(u8* pBuffer, int iLength)
{
	int i=0;
	u16 SR=g_box.usart->SR;
	g_box.usart->CR1&=~0x04;
	while(i<iLength){
//		if(IsNewCommand())break;
		if(g_box.databits==9){
			g_box.usart->DR = pBuffer[i]|(pBuffer[i+1]<<8);
			i+=2;
		}
		else{
			g_box.usart->DR = pBuffer[i];
			i+=1;
		}
#if 1
		while(1){
			SR=g_box.usart->SR;
			if(SR&USART_FLAG_TC)break;
			ClearUsartError();
		};
#else
//		while(!(g_box.usart->SR&USART_FLAG_TXE));
		if(g_box.protocol!=PT_J1587){
			ResetCounter();
			while(1){
				SR=g_box.usart->SR;
				if(SR&USART_FLAG_RXNE)break;
				ClearUsartError();
				if(GetCounter()>200)break;
			}
			g_box.usart->DR;
		}
#endif
		if(i<iLength)Delay(g_box.b2b_time);
		g_box.usart->DR;
	}
	g_box.usart->CR1|=0x04;
	return iLength;
}
int ReceiveFromEcu(u8 *pBuffer, int iLength,int iTimeout)
{
	int iCount=0;
	ResetCounter();
	while(iCount<iLength){
		u16 SR=g_box.usart->SR;
		if(SR&USART_FLAG_RXNE){
			pBuffer[iCount++]=(u8)(g_box.usart->DR);
			if(iCount==iLength)break;
			ResetCounter();
		}
		else{
			ClearUsartError();
		}
		if(GetCounter()>iTimeout)break;
//		if(IsNewCommand())return 0;
   }
   return iCount;
}
//---------------------------------------------------------------------------------------
#define J1708MODE g_box.filter_buff[0]
#define J1708MID  g_box.filter_buff[1]
#define J1708PID  g_box.filter_buff[2]
void PreJ1708Send(u8 *pSendFrame)
{
	J1708MODE = 0;
	J1708MID = 0; 
	J1708PID = 0; 
	if(g_box.filter_mode == FILTER_J1708){
	   if(pSendFrame[2] == 0xAC){
			J1708MODE = pSendFrame[3];
			switch (pSendFrame[3]){
			case 0x80:
				J1708MID = pSendFrame[5];
				break;
			case 0xff:
				J1708PID = pSendFrame[5];
				J1708MID = pSendFrame[6];
				break;
			case 0xfe:
				J1708MID = pSendFrame[4];
				break;
			case 0xc3:
				J1708MID = pSendFrame[5];
				break;
			}
	   }
	   else{
       	  J1708MODE = 1;
		  if(pSendFrame[2] == 0x5){ 
				J1708MODE = 1;
				J1708MID = pSendFrame[3];
				J1708PID = pSendFrame[4];
		  }
	  }
   }
}
static u8 *s_pSendFrame=NULL;
int SendFrame(u8 *pSendFrame)
{
	int iLength=0;
	s_pSendFrame=pSendFrame;
	switch(g_box.protocol){
	case PT_J1708:
		PreJ1708Send(pSendFrame);
	case PT_NORMAL:
	case PT_NISSAN_OLD:
	case PT_J1587:
	case PT_ISO:
	case PT_BMW_MODE2:
	case PT_BMW_MODE3:
		SendDataWaitIdle();
		iLength=Normal_SendFrame(pSendFrame);
		break;
	case PT_KWP:
	case PT_VOLVO_KWP:
		SendDataWaitIdle();
		iLength=KWP_SendFrame(pSendFrame);
		break;
	case PT_VPW:
		iLength=VPW_SendFrame(pSendFrame);
		break;
	case PT_PWM:
		iLength=PWM_SendFrame(pSendFrame);
		break;
	case PT_BOSCH:
		SendDataWaitIdle();
		iLength=BOSCH_SendFrame(pSendFrame);
		break;
	case PT_WABCOABS:
		SendDataWaitIdle();
		iLength=WABCO_SendFrame(pSendFrame);
		break;
	case PT_CAN:
	case PT_BMW_CAN:
	case PT_VOLVO_CAN:
	case PT_TP20_CAN:
	case PT_SINGEL_CAN:
		if(g_box.f2f_time)Delay(g_box.f2f_time);
		iLength=CAN_SendFrame(pSendFrame);
		break;
	}
	ResetIdleTime();
	ResetLinkTime();
	return iLength;
}
u8 IsFilterJ1708(u8 *pRecvFrame,int iLength)
{
   u8 iRet= 1;
   switch(J1708MODE){
	case 1://comnis
		if(((pRecvFrame[0]==0x07)&&(pRecvFrame[1]==(J1708MID-0x40))&&(pRecvFrame[2]==J1708PID))//单帧正确回复
		||((pRecvFrame[0]==0x80)&&(pRecvFrame[1]==0xC0))//多帧回复
		||((pRecvFrame[0]==0x06)&&(pRecvFrame[1]==0x75)&&(pRecvFrame[2]==0x85))//命令不合法,安全校验
		||((pRecvFrame[0]==0x06)&&(pRecvFrame[1]==0x64)&&(pRecvFrame[2]==0x96)))//正确回复	 
		iRet= 0;
		break;
	case 0x80:
		if(((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xc0)) || ((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xc2)))iRet= 0;
		break;
    case 0xff:
		if((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xff)&&(pRecvFrame[2]== J1708PID)) iRet= 0;
		break;
    case 0xfe:
		if(((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xc0)) || ((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xfe)&&(pRecvFrame[2]== 0xAC)))iRet= 0;
		break;
    case 0xc3:
		if((pRecvFrame[0]==J1708MID)&&(pRecvFrame[1]== 0xc4))iRet= 0;
		break;
   }
   return iRet;
}
u8 IsFilterFrame(u8 *pRecvFrame,int iLength)
{
	u8 ret=0;
	switch(g_box.filter_mode){
	case FILTER_KEYWORD:
		{u8 i,offset=g_box.filter_buff[0];
		u8 length=g_box.filter_buff[1];
		u8 *pKey=g_box.filter_buff+2;
		if(iLength<offset+length)return 1;
		for(i=0;i<length;i++){if(pRecvFrame[offset+i]!=pKey[i])return 1;}
		}
		break;
	case FILTER_J1708:
		ret=IsFilterJ1708(pRecvFrame,iLength);
		break;
	}
	return ret;
}
int RecvFrame(u8 *pRecvFrame)
{
	u16 busy_count=0;
	int iLength=0;
	u32 iClick=Clock();
	while(1){
		iLength=0;			
		if(Clock()-iClick>g_box.out_time)break;
		switch(g_box.protocol){
		case PT_NORMAL:
		case PT_NISSAN_OLD:
		case PT_ISO:
		case PT_J1587:
		case PT_BMW_MODE2:
		case PT_BMW_MODE3:
			iLength=Normal_RecvFrame(pRecvFrame);
			break;
		case PT_KWP:
			iLength=KWP_RecvFrame(pRecvFrame);
			break;
		case PT_VOLVO_KWP:
			iLength=VOLVO_KWP_RecvFrame(pRecvFrame);
			break;
		case PT_J1708:
			iLength=Normal_RecvFrame(pRecvFrame);
			break;
		case PT_VPW:
			iLength=VPW_RecvFrame(pRecvFrame);
			break;
		case PT_PWM:
			iLength=PWM_RecvFrame(pRecvFrame);
			break;
		case PT_BOSCH:
			iLength=BOSCH_RecvFrame(pRecvFrame);
			break;
		case PT_WABCOABS:
			iLength=WABCO_RecvFrame(pRecvFrame);
			break;
		case PT_TP20_CAN:
			iLength=TP20_RecvFrame(pRecvFrame);
			break;
		case PT_CAN:
		case PT_BMW_CAN:
		case PT_VOLVO_CAN:
		case PT_SINGEL_CAN:
			iLength=CAN_RecvFrame(pRecvFrame);
			break;
		}
		if(iLength==0)continue;
		if(g_box.protocol==PT_PWM||g_box.protocol==PT_VPW||g_box.protocol==PT_TP20_CAN||g_box.protocol==PT_WABCOABS||g_box.protocol==PT_BOSCH)break;
		if(IsFilterFrame(pRecvFrame,iLength))continue; 
		if(g_box.recv_only||g_box.busy_count==0)break;
		if(!IsBusyFrame(pRecvFrame,iLength))break;
		busy_count++;
		if(busy_count==g_box.busy_count)break;
		if(g_box.busy_flag&&s_pSendFrame&&((s_pSendFrame[0]<<8)|s_pSendFrame[1]))
			SendFrame(s_pSendFrame);
		iClick=Clock();
	}
	ResetIdleTime();
	ResetLinkTime();
	return iLength;
}
