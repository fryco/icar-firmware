#ifndef __PACKET_H__
#define __PACKET_H__

#include "stm32f10x_lib.h"

void SendByte(u8 data);
u8 RecvByte(u8 *data);
void SendPacket(u8 num,u8* buff,u16 len);
u16 RecvPacket(u8* buff,u16 maxlen);

#endif
