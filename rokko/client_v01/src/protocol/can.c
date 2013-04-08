#include "main.h"

void CAN_Configuration(u32 bps)
{
	CAN_InitTypeDef CAN_InitStructure;
	CAN_DeInit();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, ENABLE);
	CAN_StructInit(&CAN_InitStructure);
	CAN_InitStructure.CAN_TTCM=DISABLE;
	CAN_InitStructure.CAN_ABOM=DISABLE;
	CAN_InitStructure.CAN_AWUM=DISABLE;
	CAN_InitStructure.CAN_NART=DISABLE;
	CAN_InitStructure.CAN_RFLM=DISABLE;
	CAN_InitStructure.CAN_TXFP=DISABLE;
	CAN_InitStructure.CAN_Mode=CAN_Mode_Normal;
	CAN_InitStructure.CAN_SJW=(u8)(bps>>24);
	CAN_InitStructure.CAN_BS1=(u8)(bps>>16)-1;
	CAN_InitStructure.CAN_BS2=(u8)(bps>>8)-1;
	CAN_InitStructure.CAN_Prescaler=(u8)(bps>>0);
	CAN_Init(&CAN_InitStructure);
}
u8 CAN_SetFilter(u8 *param,u16 len)
{
	u8 i,*pid=param+1;
	u32 CanId,MskId;
	u8 CanType=(len-1)/param[0];
	
	CAN_FilterInitTypeDef   CAN_FilterInitStructure;
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FIFO0;
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;

	CAN_Configuration(g_box.baudrate);
	for(i=0;i<param[0];i++){
		CAN_FilterInitStructure.CAN_FilterNumber=i;
		if(CanType==4){
			CanId=((pid[0]<<8)|pid[1])<<21;pid+=2;
			MskId=((pid[0]<<8)|pid[1])<<21;pid+=2;
			CAN_FilterInitStructure.CAN_FilterIdHigh=(u16)(CanId>>16);
			CAN_FilterInitStructure.CAN_FilterIdLow =(u16)(CanId>>0);
			CAN_FilterInitStructure.CAN_FilterMaskIdHigh=(u16)(MskId>>16);
			CAN_FilterInitStructure.CAN_FilterMaskIdLow =(u16)(MskId>>0);				
		}
		else{
			CanId=(((pid[0]<<24)|(pid[1]<<16)|(pid[2]<<8)|pid[3])<<3);pid+=4;
			MskId=(((pid[0]<<24)|(pid[1]<<16)|(pid[2]<<8)|pid[3])<<3);pid+=4;
			CAN_FilterInitStructure.CAN_FilterIdHigh=(u16)(CanId>>16);
			CAN_FilterInitStructure.CAN_FilterIdLow =(u16)(CanId>>0);
			CAN_FilterInitStructure.CAN_FilterMaskIdHigh=(u16)(MskId>>16);
			CAN_FilterInitStructure.CAN_FilterMaskIdLow =(u16)(MskId>>0);
		}
		CAN_FilterInit(&CAN_FilterInitStructure);
	}
	return 1;
}
int CAN_SendFrame (u8* pSendFrame)
{
  u8 index;
  CanTxMsg msg;
  u8 TransmitMailbox;

CAN_FIFORelease(CAN_FIFO0);
  if(pSendFrame[2]&0x80){
  	  index=7;
	  msg.StdId=0;
	  msg.ExtId=(pSendFrame[3]<<24)|(pSendFrame[4]<<16)|(pSendFrame[5]<<8)|pSendFrame[6];
	  msg.IDE=CAN_ID_EXT;
  }
  else{
  	  index=5;
	  msg.StdId=(pSendFrame[3]<<8)|pSendFrame[4];
	  msg.ExtId=0;
	  msg.IDE=CAN_ID_STD;
  }
  msg.RTR=CAN_RTR_DATA;
  msg.DLC=pSendFrame[2]&0x0F;
  memcpy(msg.Data,pSendFrame+index,msg.DLC);
  for(index=0;index<10;index++){
	  TransmitMailbox=CAN_Transmit(&msg);
	  ResetCounter();
	  while(1){
	  	if(CAN_TransmitStatus(TransmitMailbox)==CANTXOK)
			return (pSendFrame[0]<<8)|pSendFrame[1];		
		if(GetCounter()>10)break;
	  }
	  CAN_ClearFlag(CAN_FLAG_EWG|CAN_FLAG_EPV|CAN_FLAG_BOF);
  }
  return 0;
}
int CAN_RecvFrame(u8* pRecvFrame)
{
	u8 index;
	CanRxMsg msg;
	ResetCounter();
	while(1){
		if(GetCounter()>g_box.out_time)return 0;
		if(CAN_MessagePending(CAN_FIFO0))break;
	}
	msg.IDE=CAN_ID_STD;
	CAN_Receive(CAN_FIFO0,&msg);
	if(msg.IDE==CAN_ID_STD){
		index=3;
		pRecvFrame[0]=0x00|msg.DLC;
		pRecvFrame[1]=(u8)(msg.StdId>>8);
		pRecvFrame[2]=(u8)(msg.StdId>>0);
	}
	else{
		index=5;
		pRecvFrame[0]=0x80|msg.DLC;
		pRecvFrame[1]=(u8)(msg.ExtId>>24);
		pRecvFrame[2]=(u8)(msg.ExtId>>16);
		pRecvFrame[3]=(u8)(msg.ExtId>>8);
		pRecvFrame[4]=(u8)(msg.ExtId>>0);
	}
	memcpy(pRecvFrame+index,msg.Data,msg.DLC);
	return index+msg.DLC;
}

