#include "main.h"

extern struct ICAR_DEVICE my_icar;

#define PWM_RXD()	GetLevel(my_icar.pwm_rx_io,my_icar.pwm_rx_pin)
#define PWM_HIGH()	SetLevel(my_icar.pwm_tx_io,my_icar.pwm_tx_pin,0)
#define PWM_LOW()	SetLevel(my_icar.pwm_tx_io,my_icar.pwm_tx_pin,1)

#define g_box_out_time 100
#define g_box_t_time 100

int PWM_SendFrame (unsigned char* pSendFrame)
{
	u8 index,bit,len,data;
	int cnt=g_box_out_time;
	//SysTick_ITConfig(DISABLE);
	StartTimer();
//检测EOF(低电平48us)
DEOF:
	index=0;bit=0;len=pSendFrame[0];data=pSendFrame[1];
	ResetUs();
	while(PWM_RXD()){//等低
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
	while(!PWM_RXD()){//等高
		if(TIM2->CNT>=48)goto START;
	}
	goto DEOF;
START:
//发送SOF(高电平32us)
	PWM_HIGH();
	DelayUs(32);
//发送数据(0:高电平有两个宽度, 1:低电平是两个宽度)
	PWM_LOW();
	DelayUs(16);
	while(1){
		if(data&0x80){
			PWM_HIGH();
			DelayUs(8);		
			PWM_LOW();
			DelayUs(16);
		}
		else{
			PWM_HIGH();
			DelayUs(16);		
			PWM_LOW();
			DelayUs(8);		
		}
		if(PWM_RXD())goto DEOF;
		bit++;
		data<<=1;
		if(bit==8){
			index++;
			if(index==len)break;
			bit=0;
			data=pSendFrame[1+index];
		}
	}
	PWM_LOW();
	DelayUs(1000);
	//g_tickcount+=(g_box_out_time-cnt)+((48+32+16+len*8*24)+500)/1000+1;
EXIT:
	StopTimer();
//	SysTick_ITConfig(ENABLE);
	return len;
}
int PWM_RecvFrame(unsigned char* pRecvFrame)
{
	u8 tmp,bit,len,data;
	int cnt=g_box_t_time;
//	SysTick_ITConfig(DISABLE);
	StartTimer();
DSOF:
	len=0;data=0;bit=0;
	ResetUs();
	while(!PWM_RXD()){//等高
		if(TIM2->CNT>=1000){
			if(cnt>0)cnt--;
			if(cnt==0){
			//	g_tickcount+=(g_box.out_time-cnt);
				goto EXIT;
			}
			ResetUs();
		}
	}
	ResetUs();
	while(PWM_RXD()){//等低
		if(TIM2->CNT>=32+8){
//			g_tickcount+=(g_box.out_time-cnt)+TIM2->CNT;
			goto EXIT;
		}
	}
	if(TIM2->CNT<24)goto DSOF;
	while(1){
		ResetUs();
		while(!PWM_RXD()){
			if(TIM2->CNT>=16+8)goto END;
		}
		DelayUs(12);//中间点
		data<<=1;
		tmp=PWM_RXD()?0:1;
		data|=tmp;
		bit++;
		if(bit==8){
			pRecvFrame[len++]=data;
			data=0;
			bit=0;
		}
		ResetUs();
		while(PWM_RXD()){
			if(TIM2->CNT>=16+8)goto END;
		}
	}
END:
	if(cnt>0){
//		g_tickcount+=((32+16+len*8*24)/1000);
		goto DSOF;
	}
	if(len>0){
		DelayUs(12);
		data=0xf1;
		for(bit=0;bit<8;bit++){
			if(data&0x80){
				PWM_HIGH();
				DelayUs(8);		
				PWM_LOW();
				DelayUs(16);				
			}
			else{
				PWM_HIGH();
				DelayUs(16);		
				PWM_LOW();
				DelayUs(8);				
			}
			data<<=1;
		}
//		g_tickcount+=(g_box.out_time-cnt)+(((32+16+12+(len+1)*8*24)+500)/1000);
	}
	else{
	//	g_tickcount+=(g_box.out_time-cnt)+(((32+16+len*8*24)+500)/1000);
	}
EXIT:
	StopTimer();
//	SysTick_ITConfig(ENABLE);
	return len;
}
