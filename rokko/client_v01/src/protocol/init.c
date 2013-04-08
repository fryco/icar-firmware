#include "stm32f10x_lib.h"
#include "dlc.h"
#include "queue.h"
#include "absacc.h"
#include "string.h"
#include "init.h"

#ifdef _VCI2
void RCC_Configuration(void)
{
	RCC_DeInit ();                        /* RCC system reset(for debug purpose)*/
	RCC_HSEConfig (RCC_HSE_ON);           /* Enable HSE */
	
	/* Wait till HSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
	
	RCC_HCLKConfig   (RCC_SYSCLK_Div1);   /* HCLK   = SYSCLK  */
	RCC_PCLK2Config  (RCC_HCLK_Div1);     /* PCLK2  = HCLK    */
	RCC_PCLK1Config  (RCC_HCLK_Div1);     /* PCLK1  = HCLK/2  */
	
	FLASH_SetLatency(FLASH_Latency_2);    /* Flash 2 wait state */
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
	
	/* PLLCLK = 8MHz * 9 = 72 MHz */
	RCC_PLLConfig (RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
	
	RCC_PLLCmd (ENABLE);                  /* Enable PLL */
	
	/* Wait till PLL is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
	
	/* Select PLL as system clock source */
	RCC_SYSCLKConfig (RCC_SYSCLKSource_PLLCLK);
	
	/* Wait till PLL is used as system clock source */
	while (RCC_GetSYSCLKSource() != 0x08);
}
void NVIC_Configuration(u8 IRQChannel,u8 Priority)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);   
	NVIC_InitStructure.NVIC_IRQChannel = IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);		
}
void SysTick_Configuration(void)
{
	/* Configure HCLK clock as SysTick clock source */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	SysTick_SetReload(72000);
	/* Enable the SysTick Interrupt */
	SysTick_ITConfig(ENABLE);
	/* Enable the SysTick Counter */
	SysTick_CounterCmd(SysTick_Counter_Enable);
}
void TIM2_Configuration(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_DeInit(TIM2);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseStructure.TIM_Period = (50000-1);     
	TIM_TimeBaseStructure.TIM_Prescaler = (72-1);   
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;    
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
}
void GPIO_Configuration(void)
{
	/* Disable the Serial Wire Jtag Debug Port SWJ-DP */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
	/* Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);
	/* COnfigure All*/
	SetIoMode(GPIOA,GPIO_Pin_All,GPIO_Mode_Out_PP);
	SetIoMode(GPIOB,GPIO_Pin_All,GPIO_Mode_Out_PP);
	SetIoMode(GPIOC,GPIO_Pin_All,GPIO_Mode_Out_PP);
	SetIoMode(GPIOD,GPIO_Pin_All,GPIO_Mode_Out_PP);
	SetIoMode(GPIOE,GPIO_Pin_All,GPIO_Mode_Out_PP);
	/* Configure USART1*/
	SetIoMode(GPIOA,GPIO_Pin_9,GPIO_Mode_AF_PP);
	SetIoMode(GPIOA,GPIO_Pin_10,GPIO_Mode_IN_FLOATING);
	/* Configure USART2*/
	SetIoMode(GPIOA,GPIO_Pin_2,GPIO_Mode_AF_PP);
	SetIoMode(GPIOA,GPIO_Pin_3,GPIO_Mode_IN_FLOATING);
	/* Configure USART3*/
	SetIoMode(GPIOB,GPIO_Pin_10,GPIO_Mode_AF_PP);
	SetIoMode(GPIOB,GPIO_Pin_11,GPIO_Mode_IN_FLOATING);
	/* Configure CAN1 pin*/ 
	SetIoMode(GPIOA,GPIO_Pin_11,GPIO_Mode_IPU); 
	SetIoMode(GPIOA,GPIO_Pin_12,GPIO_Mode_AF_PP);
	/* Configure CAN2 pin*/ 
	SetIoMode(GPIOB,GPIO_Pin_8,GPIO_Mode_IPU); 
	SetIoMode(GPIOB,GPIO_Pin_9,GPIO_Mode_AF_PP);
	/* Configure ADC NISSAN*/
	SetIoMode(GPIOA,GPIO_Pin_0,GPIO_Mode_AIN);
	/* Configure BOOT1*/
	SetIoMode(GPIOB,GPIO_Pin_2,GPIO_Mode_IN_FLOATING);
	I2C_DMACmd(I2C2,DISABLE);
}
void RTC_Configuration(void)
{
 	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
 	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);
 	/* Reset Backup Domain */
	BKP_DeInit();
 	// Select HSE/128 as RTC Clock Source
	RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);  
	/* Enable RTC Clock */
	RCC_RTCCLKCmd(ENABLE);
	/* Wait for RTC registers synchronization */
	RTC_WaitForSynchro();
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	/* Enable the RTC Second */  
//	RTC_ITConfig(RTC_IT_SEC, ENABLE);
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	/* Set RTC prescaler: set RTC period to 1sec  1ms */
	RTC_SetPrescaler(62); /*62499 RTC period = RTCCLK/RTC_PR = 8M / 128 = 62.5kHz ) -> (62499+1) */
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
}
#endif
void ADC1_Configuration(void)
{
	ADC_InitTypeDef   ADC_InitStructure;
	ADC_DeInit(ADC1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;  //独立模式
 	ADC_InitStructure.ADC_ScanConvMode       = DISABLE;      //连续多通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;      //连续转换
	ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; //转换不受外界决定
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
	ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;   //右对齐
	ADC_InitStructure.ADC_NbrOfChannel       = 1;       //扫描通道数
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_1Cycles5); //通道X,采样时间为1.5周期,1代表规则通道第1个这个1是啥意思我不太清楚只有是1的时候我的ADC才正常。
	ADC_Cmd(ADC1, ENABLE);              //使能或者失能指定的ADC
}
void USART_Configuration(USART_TypeDef* USART,u32 bps,u16 databit,u16 paritybit)
{
	USART_InitTypeDef USART_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	
	USART_DeInit(USART);
 	if(USART==USART1)RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	if(USART==USART2)RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	if(USART==USART3)RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_InitStructure.USART_BaudRate            = bps;
	USART_InitStructure.USART_WordLength          = databit;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = paritybit ;
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_ClockInitStructure.USART_Clock          = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL           = USART_CPOL_High;
	USART_ClockInitStructure.USART_CPHA           = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit        = USART_LastBit_Disable;
	
	USART_Init(USART, &USART_InitStructure);
	USART_ClockInit(USART, &USART_ClockInitStructure);
	if(USART==USART1){
		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		NVIC_Configuration(USART1_IRQChannel,0);
	}
	USART_Cmd(USART, ENABLE);
}
void SetIoMode(GPIO_TypeDef* io,u16 pin,GPIOMode_TypeDef mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin =  pin;
	GPIO_InitStructure.GPIO_Mode = mode;
	GPIO_Init(io, &GPIO_InitStructure);
}
