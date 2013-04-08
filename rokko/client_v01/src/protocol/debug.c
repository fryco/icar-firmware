#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Debug.h"

#define DEBUG_BUFFER_LENGTH 100

void Debug(char *szFormat,...)
{   
	char szMessage[DEBUG_BUFFER_LENGTH]="";	
  
	va_list vaList;
    va_start(vaList, szFormat );
    vsprintf(szMessage,  szFormat, vaList );        
    va_end(vaList );
	
	DEBUG_STRING(szMessage);
}


