#include "stm32f10x_lib.h"
#include "dlc.h"
#include "box.h"

void InitGPIO(u8 hw)
{
	switch(hw){
#ifndef _VCI2
	case HW_PS100:
		PS100_ConfigGpio();
		SetVpwIoPin(GPIOC,GPIO_Pin_0,GPIOC,GPIO_Pin_4);
		SetPwmIoPin(GPIOC,GPIO_Pin_1,GPIOC,GPIO_Pin_3);
		g_box.fnInitCommPort=PS100_InitCommPort;
		g_box.fnEnableLLine=PS100_EnableLLine;
		g_box.fnDisableLLine=PS100_DisableLLine;
		g_box.fnSetIoPort=PS100_SetIoPort;
		break;
	case HW_PS150:
		SetVpwIoPin(GPIOA,GPIO_Pin_1,GPIOA,GPIO_Pin_14);
		SetPwmIoPin(GPIOA,GPIO_Pin_4,GPIOA,GPIO_Pin_13);
		break;
	case HW_MPS2:
		SetVpwIoPin(GPIOA,GPIO_Pin_2,GPIOA,GPIO_Pin_3);
		SetPwmIoPin(GPIOA,GPIO_Pin_2,GPIOA,GPIO_Pin_3);
		break;
	case HW_PS201:
		SetVpwIoPin(GPIOC,GPIO_Pin_0,GPIOC,GPIO_Pin_4);
		SetPwmIoPin(GPIOC,GPIO_Pin_1,GPIOC,GPIO_Pin_3);
		break;
	case HW_T605:
		break;
#endif
	case HW_VCI2:
		VCI2_ConfigGpio();
		SetVpwIoPin(GPIOA,GPIO_Pin_2,GPIOA,GPIO_Pin_3);
		SetPwmIoPin(GPIOA,GPIO_Pin_2,GPIOA,GPIO_Pin_3);
		g_box.fnInitCommPort=VCI2_InitCommPort;
		g_box.fnEnableLLine=VCI2_EnableLLine;
		g_box.fnDisableLLine=VCI2_DisableLLine;
		g_box.fnSetIoPort=VCI2_SetIoPort;
		break;
	}
}


