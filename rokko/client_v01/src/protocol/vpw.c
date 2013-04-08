#include "config.h"
#include "main.h"


extern struct ICAR_DEVICE my_icar;
#define VPW_RXD()	GetLevel(my_icar.vpw_rx_io,my_icar.vpw_rx_pin)
#define VPW_HIGH()	SetLevel(my_icar.vpw_tx_io,my_icar.vpw_tx_pin,0)
#define VPW_LOW()	SetLevel(my_icar.vpw_tx_io,my_icar.vpw_tx_pin,1)

#define g_box_out_time 100

int VPW_SendFrame (unsigned char* pSendFrame)
{
	u8 index=0,bit=0;
	u8 data;
	int tmh=0,tml=0;
	int cnt=g_box_out_time;
	//SysTick_ITConfig(DISABLE); ??
	StartTimer();
//检测EOF(低电平280us)
DEOF:
	while(VPW_RXD()){//等低
		if(TIM2->CNT>=1000){
			if(cnt>0)cnt--;
			if(cnt==0){
				//g_tickcount+=g_box_out_time;
				goto EXIT;
			}
			ResetUs();
		}
	}
	ResetUs();
	while(!VPW_RXD()){//等高
		if(TIM2->CNT>=280)goto START;
	}
	goto DEOF;
START:
//发送SOF(高电平200us)
	VPW_HIGH();
	DelayUs(200);
	while(1){
		bit=0;
		data=pSendFrame[1+index];
		while(bit<8){
			VPW_LOW();
			if(data&0x80){
				DelayUs(128);
				tmh++;
			}
			else{
				DelayUs(64);
				tml++;
			}
			data<<=1;
			bit++;		
			VPW_HIGH();
			if(data&0x80){
				DelayUs(64);
				tml++;
			}
			else{
				DelayUs(128);
				tmh++;
			}
			data<<=1;
			bit++;				
		}
		index++;
		if(index==pSendFrame[0])break;
	}
	VPW_LOW();
	//g_tickcount+=(g_box.out_time-cnt)+(((280+200+tmh*128+tml*64)+500)/1000);
EXIT:
	StopTimer();
	//SysTick_ITConfig(ENABLE);
	return data=pSendFrame[0];
}
int VPW_RecvFrame(unsigned char* pRecvFrame)
{
	u8 bit,len,data,level;
	int tmh,tml;
	int cnt=g_box_out_time;

	//SysTick_ITConfig(DISABLE);
	StartTimer();
DSOF:
	level=0;
	bit=0;
	len=0;
	data=0;
	tmh=0;
	tml=0;
	ResetUs();
	while(!VPW_RXD()){//等高
		if(TIM2->CNT==(u16)1000){
			if(cnt>0)cnt--;
			if(cnt==0)goto EXIT;
			ResetUs();
		}
	}
	ResetUs();
	while(VPW_RXD()){//等低
		if(TIM2->CNT==200+32)goto EXIT;
	}
	if(TIM2->CNT<160)goto DSOF;

	while(1){
		ResetUs();
PULSE:
		while(VPW_RXD()==level){
			if(TIM2->CNT==128+32)goto EXIT;
		}
		if(TIM2->CNT<32){//去掉干扰
			while(VPW_RXD()!=level){
				if(TIM2->CNT==128+32)goto EXIT;	
			}
			goto PULSE;
		}
		data<<=1;
		if(TIM2->CNT>=96){tmh++;data|=(level?0:1);}//长电平
		else{tml++;data|=(level?1:0);}//短电平
		bit++;
		if(bit==8){
			pRecvFrame[len++]=data;
			data=0;
			bit=0;
		}
		level=VPW_RXD();
	}
EXIT:
END:
	StopTimer();
	//g_tickcount+=(g_box.out_time-cnt+((200+tml*64+tmh*128)+500)/1000);
	//SysTick_ITConfig(ENABLE);
	return len;
}
