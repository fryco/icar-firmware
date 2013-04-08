#include "main.h"

#define g_box_out_time 100
#define g_box_out_b2b  20

extern obd_set_type( u8 type );

int KWP_SendFrame(u8 *pSendFrame)
{
	u8 iSendLength=pSendFrame[0];
	iSendLength=SendToEcu(pSendFrame+1,iSendLength);
	return iSendLength;
}
int KWP_RecvFrame(u8 *pRecvFrame)
{	
	int iRecivelength=0;
	iRecivelength=ReceiveFromEcu(&pRecvFrame[0],1,g_box_out_time);
	if(iRecivelength==0)return iRecivelength;
	if(pRecvFrame[0]<(u8)0x80){
		if(pRecvFrame[0]==0x00){
			if(ReceiveFromEcu(&pRecvFrame[1],1,g_box_out_b2b)==1){
				iRecivelength+=1;	
				if(ReceiveFromEcu(&pRecvFrame[2],pRecvFrame[1]+1,g_box_out_b2b)==(pRecvFrame[1]+1))
					iRecivelength+=pRecvFrame[1]+1;
				else iRecivelength=0;						 
			}
			else iRecivelength=0;						  	 
		}
		else {		
			if(ReceiveFromEcu(&pRecvFrame[1],pRecvFrame[0]+1,g_box_out_b2b)==(pRecvFrame[0]+1))
				iRecivelength+=pRecvFrame[0]+1;		
			else iRecivelength=0;						 
		}
	}
	else{		
		if(pRecvFrame[0]==(u8)0x80){
			if(ReceiveFromEcu(&pRecvFrame[1],3,g_box_out_b2b)==3){
				iRecivelength+=3;	
				if(ReceiveFromEcu(&pRecvFrame[4],pRecvFrame[3]+1,g_box_out_b2b)==(pRecvFrame[3]+1))
					iRecivelength+=pRecvFrame[3]+1;	
				else iRecivelength=0;						 
			}
			else iRecivelength=0;						  	
		}
		else {		
			if(ReceiveFromEcu(&pRecvFrame[1],pRecvFrame[0]-0x80+3,g_box_out_b2b)==(pRecvFrame[0]-0x80+3))
				iRecivelength+=pRecvFrame[0]-0x80+3;			
			else iRecivelength=0;						 
		}
	}
	
    return iRecivelength;
}


int Normal_RecvFrame(u8* pRecvFrame,u16 timeout)
{
	int iReceiveLength = 0;	
	u8 cs=0;
	while(1){
		if(ReceiveFromEcu(pRecvFrame+iReceiveLength,1,timeout)==0)break;
		if(iReceiveLength>3&&pRecvFrame[0]==0x48&&pRecvFrame[1]==0x6B&&cs==pRecvFrame[iReceiveLength]){
			iReceiveLength++;
			break;
		}
		cs+=pRecvFrame[iReceiveLength++];
	}
	return iReceiveLength;
}



bool KWPSendQuickInit(u8 * pdata, u16 length)
{
	GPIO_InitTypeDef GPIO_InitStructure;


	USART3->CR1 &= (~0x00000004);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOB->BRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 25);
	GPIOB->BSRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 25);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	uart3_init();



	while(1)
	SendToEcu(pdata,length);

	USART3->CR1 |= 0x04;

	return true;
}


bool KWPTryQuickInit(void)
{
	const int bufsize=32;
	u8 rcvbuf[bufsize];
	u16 len;

	if(!KWPSendQuickInit("\xc1\x33\xf1\x81\x66",5)){
		return false;
	}

 	len=KWP_RecvFrame(rcvbuf);
	if(len<2){
		return false;
	}


	if(rcvbuf[3]!=0xc1){  //
		return false;
	}

	return true;
}

bool ISOSendSlowInit()
{
	const int bufsize=32;
	u8 rcvbuf[bufsize];
	u8 len;

	GPIO_InitTypeDef GPIO_InitStructure;
											
	len = 0;
	uart3_init();

	USART3->CR1 &= (~0x00000004);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOB->BRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 200);
	GPIOB->BSRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 400);
	GPIOB->BRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 400);
	GPIOB->BSRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 400);
	GPIOB->BRR=GPIO_Pin_10;
	OSTimeDlyHMSM(0, 0,	0, 400);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	dbg_msg("send 5 baut ok!");	   
	USART3->CR1 |= 0x04;
	
 	len=Normal_RecvFrame(rcvbuf,800);
	debug_obd("Rec %d:",len);
	
	if(len!=3){
		//dbg_msg("~~~no 0x55!");
		return false;
	}

	OSTimeDlyHMSM(0, 0,	0, 40);
	
	if(!SendToEcu("\xf7",1))
	{
		//dbg_msg("sed error!");
		return false;
	}

	if(rcvbuf[2] == 0x8f)obd_set_type(OBD_TYPE_KWP);
	else obd_set_type(OBD_TYPE_ISO);
	if(!Normal_RecvFrame(rcvbuf+3,1000))
	{
		OSTimeDlyHMSM(0, 0,	1, 0);
		return false;
	}
	OSTimeDlyHMSM(0, 0,	0, 100);
	return true;
}


