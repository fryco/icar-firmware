#include "stm32f10x_lib.h"
#include "util.h"
#include "dlc.h"

#ifdef _VCI2
vu32 g_tickcount=0;
u32 Clock(void)
{
	return g_tickcount;
}
void Delay(u32 ms)
{
	int i=0;
	u8 hclk=(RCC->CFGR&0xF0);
	u16 cnt=1000;
	if(hclk==RCC_SYSCLK_Div2)cnt=500;
	ResetUs();
	StartTimer();
	while(i<ms){
		DelayUs(cnt);
		i++;
	}
	StopTimer();
}
#endif
void DelayXs(u32 xs)
{
	int i=0;
	u8 hclk=(RCC->CFGR&0xF0);
	u16 cnt=100;
	if(hclk==RCC_SYSCLK_Div2)cnt=50;
	ResetUs();
	StartTimer();
	while(i<xs){
		DelayUs(cnt);
		i++;
	}
	StopTimer();
}
