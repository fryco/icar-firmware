#ifndef __UTIL_H__
#define __UTIL_H__

#define StartTimer()			TIM2->CR1|=0x0001
#define StopTimer()				TIM2->CR1&=0x03FE

#define DelayUs(us)				TIM2->CNT=0;TIM2->SR&=~TIM_FLAG_Update;while(TIM2->CNT<(us))
#define ResetUs()				TIM2->CNT=0;TIM2->SR&=~TIM_FLAG_Update

#ifdef _VCI2
u32 Clock(void);
void Delay(u32 ms);
#else
extern u32 Clock(void);
extern void Delay(u32 ms);
#endif
extern vu32 g_tickcount;
void DelayXs(u32 xs);

#define ResetCounter()	RTC_SetCounter(0);RTC_WaitForLastTask()
#define GetCounter()	RTC_GetCounter()


#endif
