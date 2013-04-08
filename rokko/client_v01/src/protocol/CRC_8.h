#ifndef __CRC_8_H__
#define __CRC_8_H__

#include "stm32f10x_lib.h"

//<=========================º¯Êý¶¨Òå=================================>
void AddCRC8(P_U8  OrigData);
U8 GS_CalcCrcSum(P_U8  OrigData, U8 Datalen);

#endif