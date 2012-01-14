/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_TASKMANAGER_H
#define __APP_TASKMANAGER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	S_HEAD = 0, //search head
	S_PCB  = 1, //search protocol control byte
	S_CHK  = 2  //search check byte, in the end of buffer
} GSM_RX_STATUS;

struct GSM_RX {
	GSM_RX_STATUS status;
	unsigned char *start;
	unsigned int   time;
	unsigned char  pcb;//protocol control byte
	unsigned char  seq;//sequence
	unsigned char  chk;
	unsigned int   len;//data length
};

struct SENT_QUEUE {
	unsigned int send_time;//cancel CMD if time > 1 hours
	unsigned char send_seq;
	unsigned char send_pcb;
};

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define MAX_CMD_QUEUE		5

/* Exported functions ------------------------------------------------------- */

void  App_TaskManager		(void		 *p_arg);

#endif /* __APP_TASKMANAGE_H */
