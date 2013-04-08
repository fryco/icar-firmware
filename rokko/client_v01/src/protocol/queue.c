
#include "main.h"


#define ENABLE_USART	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
#define DISABLE_USART	USART_ITConfig(USART3,USART_IT_RXNE,DISABLE);

static QUEUE queue[2];
void queue_init(u8 sq,u8 com)
{
	//if(com==COM1)queue[sq].usart=USART1;
	//else if(com==COM2)queue[sq].usart=USART2;
	//else if(com==COM3)queue[sq].usart=USART3;
	DISABLE_USART
	my_icar.stm32_u3_rx.head=0;
	my_icar.stm32_u3_rx.tail=0;
	ENABLE_USART
}
void queue_add(u8 sq,u8 data)
{
	if((queue[sq].tail+1)%MAX_QUEUE_SIZE!=queue[sq].head){
		queue[sq].buff[queue[sq].tail]=data;
		queue[sq].tail++;
		queue[sq].tail%=MAX_QUEUE_SIZE;
	}
}
u8 queue_get(u8 sq)
{
	u8 data;
	DISABLE_USART
	data=queue[sq].buff[queue[sq].head];
	queue[sq].head++;
	queue[sq].head%=MAX_QUEUE_SIZE;
	ENABLE_USART
	return data;
}
u16 queue_size(u8 sq)
{
	u16 size;
	DISABLE_USART
	size=(queue[sq].tail-queue[sq].head+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
	ENABLE_USART
	return size;
}
void queue_clear(u8 sq)
{
	DISABLE_USART
	queue[sq].head=0;
	queue[sq].tail=0;
	ENABLE_USART
}
