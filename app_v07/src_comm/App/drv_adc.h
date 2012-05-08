/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/STM32/fw_v06/source/App/drv_adc.h $ 
  * @version $Rev: 158 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-04-16 15:34:25 +0800 (周一, 2012-04-16) $
  * @brief   This file is for analog-digital converter define in STM32
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_ADC_H
#define __DRV_ADC_H

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define ADC_BUF_SIZE  16

/* Exported types ------------------------------------------------------------*/
struct ADC_STATUS {
	u16 converted[ADC_BUF_SIZE];

	bool completed;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void ADCTEMP_Configuration(void);
u16 digit_filter(u16* buf,u8 no);

#endif /* __DRV_ADC_H */
