#include "main.h"

#define LsiFreq  40000 //在30~60KHz 之间变化

void iwdg_init(void)
{
	/* IWDG timeout equal to ~2s */
	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	
	/* IWDG counter clock: LSI/256, ~6.4ms */
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	
	/* Set counter reload value to obtain 3s IWDG TimeOut.
       Counter Reload Value = 3s/IWDG counter clock period
                            = 3s / (256/LSI)
                            = LsiFreq*3/256
	*/
	IWDG_SetReload(LsiFreq*1/256);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();

}

void show_rst_flag( void )
{
	unsigned int gsm_module = 0 ;

    /* Check if the Pin Reset flag is set */
	if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
      printf("\r\n         External Reset occurred....\r\n");
    }

    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)  {
      printf("\r\n         Power On Reset occurred....\r\n");
    }

    /* Check if the soft reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)  {
      printf("\r\n         Software reset occurred....\r\n");
    }

	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
		/* IWDGRST flag set */
		printf("\r\n       Reset by IWDG...\r\n");
	}

	/* Check if the system has resumed from WWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) {
		/* WWDGRST flag set */
		printf("\r\n       Reset by WWDG...\r\n");
	}

	/* Check if the system has resumed from LPWRRST reset */
	if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET) {
		/* WWDGRST flag set */
		printf("\r\n       Reset by Low-power management...\r\n");
	}

	/* Clear reset flags */
	RCC_ClearFlag();

	//Backup register list:
	//BKP_DR1, stm32_rtc.prescaler
	//BKP_DR2, GSM Module power off timer
	//BKP_DR3, GSM Module power off reason
	gsm_module = BKP_ReadBackupRegister(BKP_DR2) ;
	if ( !gsm_module ) {
		printf("\r\n         GSM Module last power off time: ");
		RTC_show_time( gsm_module );
		gsm_module = BKP_ReadBackupRegister(BKP_DR3) ;
		printf("\r\n         GSM Module last power off reason: %d\r\n",gsm_module);
	}
	printf("\r\n");
}
