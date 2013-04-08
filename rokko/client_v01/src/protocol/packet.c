#include "stm32f10x_lib.h"
#include "packet.h"
#include "util.h"
#include "queue.h"
#include "packet.h"

static u16 packet_id=0;
void SendByte(u8 data)
{
	USART1->DR = data;
	while (!(USART1->SR & USART_FLAG_TXE));
}
u8 RecvByte(u8 *data)
{
	ResetCounter();
	while(1){
		if(GetCounter()>200)break;
		if(queue_size(SQ_HOST)){
			*data=queue_get(SQ_HOST);
			return 1;
		}
	}
	return 0;
}
void SendPacket(u8 num,u8* buff,u16 len)
{
	u8 tmp,cs;
	u16 i,length;

	tmp=0xaa;					SendByte(tmp);		cs =tmp;
	tmp=0x55;					SendByte(tmp);		cs^=tmp;
	tmp=(u8)(packet_id>>8);		SendByte(tmp);		cs^=tmp;
	tmp=(u8)(packet_id>>0);		SendByte(tmp);		cs^=tmp;
	length=len+1;
	tmp=(u8)(length>>8);		SendByte(tmp);		cs^=tmp;
	tmp=(u8)(length>>0);		SendByte(tmp);		cs^=tmp;
	length=~length;
	tmp=(u8)(length>>8);		SendByte(tmp);		cs^=tmp;
	tmp=(u8)(length>>0);		SendByte(tmp);		cs^=tmp;
	tmp=num;					SendByte(tmp);		cs^=tmp;
	for(i=0;i<len;i++){			SendByte(buff[i]);	cs^=buff[i];}
	SendByte(cs);
}
u16 RecvPacket(u8* buff,u16 maxlen)
{
	u8 tmp,cs;
	u16 i,len;
	while(1){
		if(queue_size(SQ_HOST)<9)return 0;
		if(!RecvByte(&tmp))return 0;	cs= tmp;	if(tmp!=0x55)continue;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	if(tmp!=0xaa)continue;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	i=tmp<<8;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	i|=tmp;
		packet_id=i;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	len=tmp<<8;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	len|=tmp;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	i=tmp<<8;
		if(!RecvByte(&tmp))goto EXIT;	cs^=tmp;	i|=tmp;
		if((u16)~i!=len)goto EXIT;
		if(len>maxlen)goto EXIT;
		for(i=0;i<len;i++){
	 		if(!RecvByte(buff+i))goto EXIT;
			cs^=buff[i];
		}
		if(!RecvByte(&tmp))goto EXIT;
		if(tmp!=cs)goto EXIT;
		break;
	}
	SendPacket(0x00,(u8*)"\x00\x02\x00\x00",4);
	return len;
EXIT:
	SendPacket(0x00,(u8*)"\x00\x02\x00\x01",4);
	return 0;
}
