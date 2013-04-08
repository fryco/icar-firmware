#ifndef __DEBUG_H__
#define __DEBUG_H__
 
#include "Packet.h"

void Debug(char *szFormat,...) ;
#define DEBUG_STRUCT(X) {int i;for(i=0; i<sizeof(X); i++) SendByte(*((char*)&X+i));}
#define DEBUG_PARAMETER(X,LENGTH) {int i;for(i=0; i<LENGTH; i++) SendByte(*((char*)X+i));}
#define DEBUG_STRING(X) DEBUG_PARAMETER(X,strlen(X));DEBUG_PARAMETER("\r\n",2+1);


#endif

