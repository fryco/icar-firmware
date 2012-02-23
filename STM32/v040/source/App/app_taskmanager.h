/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_TASKMANAGER_H
#define __APP_TASKMANAGER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported macro ------------------------------------------------------------*/
#define MAX_CMD_QUEUE		60

//HEAD SEQ CMD Length(2 bytes) INF(Max.1024) check
//#define GSM_BUF_LENGTH		1024+8 //for GSM command
#define GSM_BUF_LENGTH		1024 //for GSM command

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	S_HEAD = 0, //search head
	S_PCB  = 1, //search protocol control byte
	S_CHK  = 2  //search check byte, in the end of buffer
} GSM_RX_RESPOND_STATUS;

struct GSM_RX_RESPOND {//for decode respond string from server
	GSM_RX_RESPOND_STATUS status;
	unsigned char *start;
	unsigned int   timer;//S_PCB ==> S_CHK, < 5*AT_TIMEOUT
	unsigned char  pcb;//protocol control byte
	unsigned char  seq;//sequence
	unsigned char  chk;
	unsigned int   len;//data length
};

struct SENT_QUEUE {
	unsigned int send_timer;//cancel CMD if time > 1 hours
	unsigned char send_seq;
	unsigned char send_pcb;
};

struct CAR2SERVER_COMMUNICATION {
	//queue for sent CMD
	unsigned char queue_count ;
	struct SENT_QUEUE queue_sent[MAX_CMD_QUEUE];//last 2 queue for emergency event

	//tx buffer for tcp data
	bool tx_lock ;
	//tx buffer for tcp data
	unsigned char tx[GSM_BUF_LENGTH];
	unsigned int tx_len;//tx_len=0¼´Îª empty;tx_len=GSM_BUF_LENGTH¼´Îªfull
	unsigned int tx_timer; // send if > TCP_SEND_PERIOD

	unsigned char tx_sn[36];//tx buffer for SN
	unsigned int  tx_sn_len;//tx length for SN

	//rx buffer for tcp data
	unsigned char rx[GSM_BUF_LENGTH];
	unsigned char *rx_out_last;
	unsigned char *rx_in_last;
	bool rx_empty;
	bool rx_full;
	unsigned int rx_timer;//reset GSM module if rx timeout
	unsigned int check_timer;//check rx period

	//New instruction from server
	unsigned char next_ist ;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

void  App_TaskManager		(void		 *p_arg);

#endif /* __APP_TASKMANAGE_H */
