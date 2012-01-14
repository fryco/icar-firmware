#include "main.h"

struct RTC_STATUS stm32_rtc;

//Backup register list:
//BKP_DR1, stm32_rtc.prescaler

static unsigned int datetime_to_seconds (struct DATE_TIME *datetime)
{
    unsigned int years, months, days;
	const unsigned int days_before_m[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    years = datetime->year - 2000;
    days = years * 365 + (years + 3) / 4;    // 2000年至去年年底的已逝天数
    if(years > 100) days--;                // 例外的非闰年只有2100年

    months = datetime->month;
    days += days_before_m[months - 1];
    if((months > 2) && ((years & 0x03) == 0) && (years != 100)) days++;    // 闰年3月以后天数加一

    days += datetime->day - 1;        // 日期从 1 开始

    return days*86400 + datetime->hour*3600 + datetime->minute*60 + datetime->second;
}

static void seconds_to_datetime (unsigned int counts, struct DATE_TIME *datetime)
{
    unsigned int  years, months, days, hours, minutes, seconds, tt;
	const unsigned int days_before_m[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    seconds = counts % 60;
    counts /= 60;
    minutes = counts % 60;
    counts /= 60;
    hours = counts % 24;
    counts /= 24;

    years = counts / 365;        // 现在counts 中只剩天数了
    days = counts % 365;
    tt = (years + 3) / 4;
    if(years > 100) tt--;        // 现在tt中存有闰年调整天数
    if(days < tt){            // 如果不够调整
        years--;                // 说明今天属于去年
        if(((years & 0x03) == 0) && (years != 100)) days++;
        days += 365;
    }
    days -= tt;

    months = 0;
    while((months < 12) && (days >= days_before_m[months])) months++;    // 计算月
    days -= days_before_m[months - 1];    // 分离日

    if((months > 2) && ((years & 0x03) == 0) && (years != 100)){    // 闰年 2 月日期调整
        if(days == 0){
            months--;
            days = (months == 2) ? 29 : days_before_m[months] - days_before_m[months-1];
        }
        days--;
    }
    days++;

    datetime->year = years + 1970;

    datetime->month = months;
    datetime->day = days;
    datetime->hour = hours;
    datetime->minute = minutes;
    datetime->second = seconds;
}

void rtc_init(void)	{

	struct DATE_TIME datetime;

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

	seconds_to_datetime(RTC_GetCounter() , &datetime);
	prompt("UTC: %04d-%02d-%02d  ", datetime.year, datetime.month, datetime.day);
	printf("%02d:%02d:%02d\r\n", datetime.hour, datetime.minute, datetime.second);
}

void RTC_update_calibrate( unsigned char *p1, unsigned char *p2) 
{
	struct DATE_TIME datetime;
	unsigned char i ;

	stm32_rtc.update_time = 0 ;
	for ( i = 0 ; i < 4 ; i++ ) {//
		if ( (p1+i+5) < p2+GSM_BUF_LENGTH ) {
			stm32_rtc.update_time |= (*(p1+i+5))<<(24-i*8);
		}
		else {//data in begin of buffer
			stm32_rtc.update_time |= (*(p1+i+5-GSM_BUF_LENGTH))<<(24-i*8);
		}
	}
	//prompt("stm32_rtc.update_time: %08X ",stm32_rtc.update_time);


	if ( stm32_rtc.update_time != RTC_GetCounter( )) {
		prompt("RTC prescaler %d ==> ",((uint32_t)RTC->PRLH << 16 ) | RTC->PRLL);
		if ( stm32_rtc.update_time > RTC_GetCounter( )) {

			i = stm32_rtc.update_time - RTC_GetCounter( );
	
			if ( i < 10 ) {
				stm32_rtc.prescaler = stm32_rtc.prescaler--;
			}
			else {
				stm32_rtc.prescaler = stm32_rtc.prescaler - i*20;
			}
			if ( stm32_rtc.prescaler < 30000 ) {//prevent calc error
				stm32_rtc.prescaler = 30000 ;
			}

		}
		else {
			i = RTC_GetCounter( ) - stm32_rtc.update_time;
			if ( i < 10 ) {
				stm32_rtc.prescaler = stm32_rtc.prescaler++;
			}
			else {
				stm32_rtc.prescaler = stm32_rtc.prescaler + i*20;
			}
			if ( stm32_rtc.prescaler > 45000 ) {//prevent calc error
				stm32_rtc.prescaler = 45000 ;
			}
		}
		
		RTC_SetPrescaler(stm32_rtc.prescaler);
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	
		BKP_WriteBackupRegister(BKP_DR1, stm32_rtc.prescaler);
		printf("%d\r\n",((uint32_t)RTC->PRLH << 16 ) | RTC->PRLL);
	}//if ==, no need update RTC Prescaler

	seconds_to_datetime(RTC_GetCounter() , &datetime);
	prompt("UTC: %d-%02d-%02d  ", datetime.year, datetime.month, datetime.day);
	printf("%02d:%02d:%02d ==> ", datetime.hour, datetime.minute, datetime.second);

	//Update RTC
	RTC_SetCounter( stm32_rtc.update_time );
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	//prompt("RTC_GetCounter:%08X\tupdate_time:%08X\r\n",RTC_GetCounter(),stm32_rtc.update_time);

	seconds_to_datetime(RTC_GetCounter() , &datetime);
	printf("%d-%02d-%02d  ", datetime.year, datetime.month, datetime.day);
	printf("%02d:%02d:%02d\r\n", datetime.hour, datetime.minute, datetime.second);

}

void RTC_show_time( )
{
	struct DATE_TIME datetime;
	seconds_to_datetime(RTC_GetCounter() , &datetime);
	printf("UTC: %02d:%02d:%02d\r\n", datetime.hour, datetime.minute, datetime.second);
}
