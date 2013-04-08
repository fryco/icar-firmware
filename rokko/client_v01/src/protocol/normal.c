#include "stm32f10x_lib.h"
#include "prot.h"
#include "dlc.h"
#include "util.h"
#include "string.h"
#include "box.h"

static u8 s_ucFixReceiveLength=0;
int Normal_SendFrame (u8* pSendFrame)
{
	int iSendLength = (pSendFrame[0]<<8)|pSendFrame[1];
	switch(g_box.filter_mode){
	case FILTER_SPCLEN:
		s_ucFixReceiveLength=*(pSendFrame+2+iSendLength);
		iSendLength=SendToEcu(pSendFrame+2,iSendLength);
		break;
	default:
		iSendLength=SendToEcu(pSendFrame+2,iSendLength);
		break;
	}
	return iSendLength;
}
int Normal_RecvFrame(u8* pRecvFrame)
{
	int iReceiveLength = 0;	
	if(WaitDataFromEcu()==0)return 0;
	switch(g_box.filter_mode){
	case FILTER_FIXLEN:
		iReceiveLength=ReceiveFromEcu(pRecvFrame,g_box.filter_buff[0],g_box.out_b2b);
		break;
	case FILTER_SPCLEN:
		iReceiveLength=ReceiveFromEcu(pRecvFrame,s_ucFixReceiveLength,g_box.out_b2b);
		break;
	case FILTER_NORMAL:		
		{struct tagLengthNormal{
			u8 ucLengthPosition;
			u8 ucRecogniseMark;
			char chOffset;
		}*pNormalMode=(struct tagLengthNormal*)(g_box.filter_buff);
		int iReceiveLengthu8 = 0;
		u8 ucMoveBitNumber = 0;
		while(!( pNormalMode->ucRecogniseMark & (0x01<<ucMoveBitNumber)))++ucMoveBitNumber;
		iReceiveLength=ReceiveFromEcu(pRecvFrame, pNormalMode->ucLengthPosition+1,g_box.out_b2b);
		iReceiveLengthu8 = pRecvFrame[iReceiveLength-1];
		iReceiveLengthu8 &= pNormalMode->ucRecogniseMark;
		iReceiveLengthu8 >>= ucMoveBitNumber;
		iReceiveLengthu8 += pNormalMode->chOffset-iReceiveLength;
		iReceiveLength += ReceiveFromEcu(pRecvFrame+iReceiveLength, iReceiveLengthu8,g_box.out_b2b);
		}break;
	default:
		if(g_box.protocol==PT_ISO){//&&g_box.filter_mode==FILTER_KEYWORD){
		//	u8 offset=g_box.filter_buff[0];
		//	u8 length=g_box.filter_buff[1];
		//	u8 *pKey=g_box.filter_buff+2;
		//	if(length>=2&&pKey[offset+0]==0x48&&pKey[offset+1]==0x6B){
				u8 cs=0;
				while(1){
					if(ReceiveFromEcu(pRecvFrame+iReceiveLength,1,g_box.out_b2b)==0)break;
					if(iReceiveLength>3&&pRecvFrame[0]==0x48&&pRecvFrame[1]==0x6B&&cs==pRecvFrame[iReceiveLength]){
						iReceiveLength++;
						break;
					}
					cs+=pRecvFrame[iReceiveLength++];
				}
				return iReceiveLength;
	//		}
		}
		iReceiveLength=ReceiveFromEcu(pRecvFrame,1024,g_box.out_b2b);
		break;
	}
	return iReceiveLength;
}

