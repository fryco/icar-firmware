#include "main.h"

#define LsiFreq  40000 //在30~60KHz 之间变化

void iwdg_init(void)
{
	/* IWDG timeout equal to ~2s */
	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	
	/* IWDG counter clock: LSI/256, ~6.4ms */
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	
	/* Set counter reload value to obtain 4s IWDG TimeOut.
       Counter Reload Value = 4s/IWDG counter clock period
                            = 4s / (256/LSI)
                            = LsiFreq*4/256
	*/
	IWDG_SetReload(LsiFreq*2/256);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();

}

void show_rst_flag( void )
{
	static unsigned char reason;

	reason = 0;
    /* Check if the Pin Reset flag is set */
	if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
		printf("\r\n         External Reset occurred....\r\n");
		reason = 1;
    }

    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)  {
		printf("\r\n         Power On Reset occurred....\r\n");
		reason = 2;
    }

    /* Check if the soft reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)  {
		printf("\r\n         Software reset occurred....\r\n");
		reason = 3;
    }

	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
		/* IWDGRST flag set */
		printf("\r\n       Reset by IWDG...\r\n");
		reason = 4;
	}

	/* Check if the system has resumed from WWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) {
		/* WWDGRST flag set */
		printf("\r\n       Reset by WWDG...\r\n");
		reason = 5;
	}

	/* Check if the system has resumed from LPWRRST reset */
	if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET) {
		/* WWDGRST flag set */
		printf("\r\n       Reset by Low-power management...\r\n");
		reason = 6;
	}

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:reverse
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
    BKP_WriteBackupRegister(BKP_DR1, \
		(((BKP_ReadBackupRegister(BKP_DR1))&0x0FFF) | ((reason<<12)&0xF000)));

	//BKP_DR8, MCU reset time(UTC Time) high
	//BKP_DR9, MCU reset time(UTC Time) low
    BKP_WriteBackupRegister(BKP_DR8, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
    BKP_WriteBackupRegister(BKP_DR9, (RTC_GetCounter( ))&0xFFFF);//low

	/* Clear reset flags */
	RCC_ClearFlag();
}

void show_err_log( void )
{
	static unsigned int var_reg ;

	printf("\r\n");
	var_reg = (BKP_ReadBackupRegister(BKP_DR1))&0x000F ;
	if ( var_reg ) {
		printf("         BKP_DR1, GSM Module power off reason: %d\r\n",var_reg);
	
		//BKP_DR2, GSM Module power off time(UTC Time) high
		//BKP_DR3, GSM Module power off time(UTC Time) low
		var_reg = BKP_ReadBackupRegister(BKP_DR2)<<16 | BKP_ReadBackupRegister(BKP_DR3) ;
		printf("         BKP_DR2&3, GSM Module power off time: ");
		RTC_show_time(var_reg);
	}
	
	var_reg = (BKP_ReadBackupRegister(BKP_DR1))&0x00F0 ;
	var_reg = (var_reg>>4)&0x0F ;
	if ( var_reg ) {
		printf("         BKP_DR1, GPRS disconnect reason: %d\r\n",var_reg);
	
		//BKP_DR4, GPRS disconnect time(UTC Time) high
		//BKP_DR5, GPRS disconnect time(UTC Time) low
		var_reg = BKP_ReadBackupRegister(BKP_DR4)<<16 | BKP_ReadBackupRegister(BKP_DR5) ;
		printf("         BKP_DR4&5, GPRS disconnect time time: ");
		RTC_show_time(var_reg);
	}
	
	printf("         BKP_DR10, stm32_rtc.prescaler: 0x%X\r\n\r\n",\
			BKP_ReadBackupRegister(BKP_DR10));
}
