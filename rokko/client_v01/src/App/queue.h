#ifndef __QUEUE_H__
#define __QUEUE_H__

#define MAX_QUEUE_SIZE	2048
typedef struct{
	USART_TypeDef* usart;
	u8 buff[MAX_QUEUE_SIZE];
	vu16 head,tail;
}QUEUE;

enum{SQ_HOST,SQ_ECU};
//enum{COM1,COM2,COM3};
void queue_init(u8 sq,u8 com);
void queue_add(u8 sq,u8 data);
u8 queue_get(u8 sq);
u16 queue_size(u8 sq);
void queue_clear(u8 sq);

#endif
