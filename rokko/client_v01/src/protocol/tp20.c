#include "stm32f10x_lib.h"
#include "prot.h"
#include "dlc.h"
#include "util.h"
#include "string.h"
#include "box.h"

int TP20_RecvFrame(u8* pRecvFrame)
{
	u8 length,buffer[16];
	u8 bContinueFlag=1;
	u8 nOrder,nLinkCnt=0;
	u16 iLength,iRecvLen;
	u16 busy_count=0;
	u8 idle_cmd[]={0x00,0x09,0x07,0x00,0x00,0xA1,0x0F,0x64,0x9C,0x0A,0x9C};

	idle_cmd[3]=g_box.link_buff[2+1];
	idle_cmd[4]=g_box.link_buff[2+2];
	while(bContinueFlag){
		length=CAN_RecvFrame(buffer);
		if(length==0)break;
		if(length<=4){
			if(buffer[3]==0xA3){////接收链路保持贞
				Delay(g_box.f2f_time);
				CAN_SendFrame(idle_cmd);
				nLinkCnt++;
				if(nLinkCnt==10)return 0;
			}
			continue;	
		}
		if(length>=7&&buffer[6]==0x7F&&buffer[8]==0x78){
			busy_count++;
			nOrder=buffer[3];
			g_box.link_buff[2+3]=0xB0+(((nOrder&0x0F)+1)&0x0F);
			Delay(g_box.f2f_time);
			CAN_SendFrame(g_box.link_buff);	
			g_box.link_buff[2+3]=0xA3;
			if(busy_count==g_box.busy_count){
				AddFrameToBuffer(buffer,length);
				return 1;
			}
			continue;
		}												
		if((length>7)&&(buffer[3]==0xA1)){//接收到等待贞
			AddFrameToBuffer(buffer,length);
			return 1 ;
		}
		if(buffer[4]&0x80)bContinueFlag=1;
		else bContinueFlag=0;

		iRecvLen=length-6;//已接收的长度
		iLength=(buffer[4]&0x0f)*256+buffer[5]; //需要接收的长度
		nOrder=buffer[3];
		AddFrameToBuffer(buffer,length);
		while(iRecvLen<iLength){
			length=CAN_RecvFrame(buffer);
			if(length==0)break;
			else{
				iRecvLen+=(length-4);
				nOrder=buffer[3];
				AddFrameToBuffer(buffer,length);
			}
		}						
		g_box.link_buff[2+3]=0xB0+(((nOrder&0x0F)+1)&0x0F);
		Delay(g_box.f2f_time);
		CAN_SendFrame(g_box.link_buff);	
		g_box.link_buff[2+3]=0xA3;
	}
	return 1;
}
