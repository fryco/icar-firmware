#include "stm32f10x_lib.h"
#include "prot.h"
#include "dlc.h"
#include "util.h"
#include "string.h"
#include "box.h"

#define FilterBytes(a,b)	(pRecvFrame[index]==(a)&&pRecvFrame[index+2]==(b))	

u8 CAN_FilterFrame(u8 *pRecvFrame,int iLength)
{
	u8 index=4;
	if(pRecvFrame[0]&0x80)index=6;
	if(g_box.protocol==PT_BMW_CAN)index++;
	if(index+2>=iLength)return 0;
	if(g_box.protocol==PT_VOLVO_CAN){
		if(FilterBytes(0x7E,0x23))return 1;
		return 0;
	}
	if(FilterBytes(0x7F,0x78))return 1;
	if(FilterBytes(0x7F,0x21)){
		g_box.busy_flag=1;
		return 1;
	}
	return 0;
}
u8 KWP_FilterFrame(u8 *pRecvFrame,int iLength)
{
	u8 index=3;
	if(pRecvFrame[0]==(u8)0x80)index=4;
	else if(pRecvFrame[0]==0x00)index=2;
	else if(pRecvFrame[0]<(u8)0x80)index=1;
	if(index+2>=iLength)return 0;
	if(g_box.protocol==PT_VOLVO_KWP){
		if(FilterBytes(0x7E,0x23))return 1;
		return 0;
	}
	if(FilterBytes(0x7F,0x78))return 1;
	if(FilterBytes(0x7F,0x21)){
		g_box.busy_flag=1;
		return 1;
	}
	return 0;
}
u8 BMW2_FilterFrame(u8 *pRecvFrame,int iLength)
{
	u8 index=2;
	if(index+0>=iLength)return 0;
	if(pRecvFrame[index]==(u8)0xA1)return 1;
	return 0;
}

u8 IsBusyFrame(u8 *pRecvFrame,int iLength)
{
	u8 ret=0;
	g_box.busy_flag=0;
	if(g_box.busy_count==0)return 0;
	switch(g_box.protocol){
	case PT_CAN:
	case PT_VOLVO_CAN:
	case PT_SINGEL_CAN:
	case PT_BMW_CAN:
	case PT_TP20_CAN:
		ret=CAN_FilterFrame(pRecvFrame,iLength);
		break;
	case PT_KWP:
	case PT_VOLVO_KWP:
	case PT_BMW_MODE3:
		ret=KWP_FilterFrame(pRecvFrame,iLength);
		break;
	case PT_BMW_MODE2:
		ret=BMW2_FilterFrame(pRecvFrame,iLength);
		break;
	}
	return ret;
}
