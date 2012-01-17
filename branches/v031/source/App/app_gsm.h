/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GSM_H
#define __APP_GSM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported macro ------------------------------------------------------------*/
//HEAD SEQ CMD Length(2 bytes) INF(Max.1024) check
//#define GSM_BUF_LENGTH		1024+8 //for GSM command
#define GSM_BUF_LENGTH		61 //for GSM command

/* Exported types ------------------------------------------------------------*/
struct GSM_STATUS {
	bool ask_power ;
	bool power_on ;

	bool gprs_ready ;
	unsigned char gprs_count;
	bool ask_online;
	bool tcp_online;
	unsigned char try_online;

	//运营商信息，+COPS: 0,0,"CHINA MOBILE"
	unsigned char carrier[32];
	unsigned char imsi[16];
	unsigned char local_ip[16];
	unsigned char *server_ip_port;
	unsigned int apn_index;
	unsigned char signal;
	unsigned char err_no;
	bool roam;
	bool cgatt;
	bool rx_empty;//GSM Module rx buffer
	unsigned int at_timer;

	unsigned char ring_count;
	unsigned int dial_timer;
	bool need_dial ;
	bool voice_confirm ;
};

struct GSM_COMMAND {
	bool lock ; //如果发现长期加锁情况，则加lock_timer
	//tx buffer for tcp data
	unsigned char tx[GSM_BUF_LENGTH];
	unsigned char *tx_out_last;
	unsigned char *tx_in_last;
	unsigned int tx_len;

	//rx buffer for tcp data
	unsigned char rx[GSM_BUF_LENGTH];
	unsigned char *rx_out_last;
	unsigned char *rx_in_last;
	bool rx_empty;
	bool rx_full;
	unsigned int rx_timer;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void  App_TaskGsm        (void        *p_arg);

#endif /* __APP_GSM_H */
