/**
  ******************************************************************************
  * @file    source/App/drv_mg323.h
  * @author  cn0086@139.com
  * @version v01
  * @date    2011/11/24 14:25:15
  * @brief   This file contains GSM module: MG323 driver
  ******************************************************************************
  * @history v00: 2011/11/24 14:25:28, draft, by cn0086@139.com
  * @        v01: 
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_MG323_H
#define __DRV_MG323_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

unsigned char gsm_power_on( void ) ;
unsigned char gsm_check_gprs( void );
bool gsm_pwr_off(void);
unsigned char check_gsm_CSQ( void );
unsigned char check_tcp_status( void );
bool get_respond( unsigned char* rec_str);
bool gsm_send_tcp( unsigned char *send_buf, unsigned int dat_len );
bool gsm_ask_tcp( unsigned int dat_len ) ;
#endif /* __DRV_MG323_H */