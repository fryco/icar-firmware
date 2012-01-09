/**
  ******************************************************************************
  * @file    source/App/drv_adc.h 
  * @author  cn0086@139.com
  * @version V00
  * @date    2011/10/31 17:57:35
  * @brief   This file contains global function
  ******************************************************************************
  * @history v00: 2011/10/31, draft, by cn0086@139.com
  * @        v01:
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_ADC_H
#define __DRV_ADC_H

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define ADC_BUF_SIZE  16

/* Exported types ------------------------------------------------------------*/
struct icar_adc_buf {
	u16 converted[ADC_BUF_SIZE];

	bool completed;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void ADCTEMP_Configuration(void);
u16 digit_filter(u16* buf,u8 no);

#endif /* __DRV_ADC_H */
