#include "main.h"

static	OS_STK		   App_TaskManagerStk[APP_TASK_MANAGER_STK_SIZE];

struct ICAR_DEVICE my_icar;

/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	 
#ifdef VECT_TAB_RAM
	// Set the Vector Tab base at location at 0x20000000
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else
	// Set the Vector Tab base at location at 0x80000000
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif   

	/* Configure the NVIC Preemption Priority Bits[配置优先级组] */  
	//第1组：最高1位用于指定抢占式优先级（2个），最低3位用于指定响应优先级（8个）
	//Priority_0 高于Priority_1
	//DMA : Priority_0, others: Priority_1
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//ucos任务切换时可能出错
	
	//第0组：所有4位用于指定响应优先级（16个）
	//确保ucos任务切换正常
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	//SubPriority 排序
	//0: 
	//1: 
	//2:  DMA1.7, 
	//4:  DMA1.1, for adc   <==temperature convert
	//6:  DMA1.4, 
	//8:  Uart3 rx/tx       <==K-Line, OBD-II
	//10: Uart2 rx/tx       <==GSM module
	//12: Uart1 rx/tx       <==console
	//...
	//13: 
	//14: RTC
	//15: System tick timer, for os

	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 14;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART1 Interrupt */ 
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 12; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 
	
	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 10; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	/* Enable the USART3 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 8; 
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	/* Init the NVIC for DMA1.1 global Interrupt for ADC */
	//ADC only work with channel1, do not use other channel
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 

	//SPI1 <==> G-Force
	//USART3 Tx (or SPI1_RX) --> DMA Channel 2 

	//SPI1_TX --> DMA Channel 3

	//SPI2 <==> SPI Flash
	//USART1 Tx (or SPI2_RX) --> DMA Channel 4
	/* U1 TX had changed to interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	*/

	//SPI2_TX --> DMA Channel 5

	/* Init the NVIC for DMA1.7 global Interrupt for USART2.TX */
	/* U2 TX had changed to interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	*/
}

int	main(void)
{
	CPU_INT08U  os_err;

	NVIC_Configuration( );
	
	gpio_init( ) ;
	uart1_init( );

	rtc_init();

	uart3_init( );

	ADCTEMP_Configuration( );

	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

	led_init_all( );
	led_on(POWER_LED);

	//GSM_PM_ON;
	//while ( 1 ) ;//for upgrade GSM fw
	GSM_PM_OFF;
	GPS_PM_OFF;
	SD_PM_OFF;

	/*	Initialize "uC/OS-II, The Real-Time	Kernel".		 */
	OSInit(); //include: OSTime = 0L;
	//OSTimeSet(RTC_GetCounter( ));

	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskManager,  /* Create the start task.*/
						   (void		  *	) 0,
						   (OS_STK		  *	)&App_TaskManagerStk[APP_TASK_MANAGER_STK_SIZE - 1],
						   (INT8U			) APP_TASK_MANAGER_PRIO,
						   (INT16U			) APP_TASK_MANAGER_PRIO,
						   (OS_STK		  *	)&App_TaskManagerStk[0],
						   (INT32U			) APP_TASK_MANAGER_STK_SIZE,
						   (void		  *	)0,
						   (INT16U			)(OS_TASK_OPT_STK_CLR |	OS_TASK_OPT_STK_CHK));

#if	(OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_MANAGER_PRIO,	(CPU_INT08U	*)"Start Task",	&os_err);
#endif

	OSStart();	/* Start multitasking (i.e.	give control to	uC/OS-II).	*/

	return (0);  
}
