#include "main.h"

struct rtc_status stm32_rtc;

void rtc_init(void)	{

	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	
	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	stm32_rtc.prescaler = BKP_ReadBackupRegister(BKP_DR1) ;

	if (stm32_rtc.prescaler == 0x0)
	{
	    prompt("RTC not yet configured....\r\n");
	
		/* Reset Backup Domain */
		BKP_DeInit();
		
		/* Enable LSI */
		RCC_LSICmd(ENABLE);
		
		/* Wait till LSI is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
		{}
		
		/* Select LSI as RTC Clock Source */
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		
		/* Enable RTC Clock */
		RCC_RTCCLKCmd(ENABLE);
		
		/* Wait for RTC registers synchronization */
		RTC_WaitForSynchro();
		
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
				
		/* Set RTC prescaler: set RTC period to 1sec */
		//RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
		RTC_SetPrescaler(40000);
		
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	
	    prompt("RTC configured.\r\n");
	
		stm32_rtc.prescaler = ((uint32_t)RTC->PRLH << 16 ) | RTC->PRLL ;

	    BKP_WriteBackupRegister(BKP_DR1, stm32_rtc.prescaler);
	}
	else {
    	prompt("No need to configure RTC....%X\r\n",stm32_rtc.prescaler);

		RTC_SetPrescaler(stm32_rtc.prescaler);
  
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
	/* Enable the RTC Second */
	RTC_ITConfig(RTC_IT_SEC, ENABLE);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	stm32_rtc.update_time = 0 ;
	stm32_rtc.updating = false ;
}
