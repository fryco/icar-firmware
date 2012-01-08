#include "main.h"

#define	BUILD_DATE "Built by Jack Li at	" __DATE__"	"__TIME__ "\r\n" 

static	OS_STK		   App_TaskManageStk[APP_TASK_MANAGE_STK_SIZE];
static	OS_STK		   App_TaskGsmStk[APP_TASK_GSM_STK_SIZE];

static	void  App_TaskManage		(void		 *p_arg);

extern u8 un_expect;

extern u8 *u1_tx_buf;//set to tx buffer before enable interrupt
extern u8 u1_rx_buf1[RX_BUF_SIZE];
extern u8 u1_rx_buf2[RX_BUF_SIZE];
extern u8 u1_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u1_rx_lost_data;//1: lost data
extern u8 u1_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u1_tx_cnt, u1_rx_cnt1, u1_rx_cnt2 ;

extern u8 *u2_tx_buf;//set to tx buffer before enable interrupt
extern u8 u2_rx_buf1[RX_BUF_SIZE];
extern u8 u2_rx_buf2[RX_BUF_SIZE];
extern u8 u2_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u2_rx_lost_data;//1: lost data
extern u8 u2_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u2_tx_cnt, u2_rx_cnt1, u2_rx_cnt2 ;

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
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);


 /* Enable the RTC Interrupt */
/*
   NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
*/

  /* Enable the USART1 Interrupt */ 
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
  NVIC_Init(&NVIC_InitStructure); 

    /* Enable the USART2 Interrupt */ 
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 

  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
  NVIC_Init(&NVIC_InitStructure); 
}

int	main(void)
{
	CPU_INT08U  os_err;

	NVIC_Configuration( );

	gpio_init( ) ;

	led_init(LED1);
	led_on(LED1);

	uart1_init( );

	led_init(GSM_PM);
	GSM_PM_OFF;

	led_init(GPS_PM);
	GPS_PM_OFF;

	led_init(SD_PM);
	SD_PM_OFF;

	/* Output	a message on Hyperterminal using printf	function */
	prompt("\r\n\r\n%s\r\n",BUILD_DATE);

	OSInit();	 /*	Initialize "uC/OS-II, The Real-Time	Kernel".		 */

	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskManage,  /* Create the start task.							   */
						   (void		  *	) 0,
						   (OS_STK		  *	)&App_TaskManageStk[APP_TASK_MANAGE_STK_SIZE - 1],
						   (INT8U			) APP_TASK_MANAGE_PRIO,
						   (INT16U			) APP_TASK_MANAGE_PRIO,
						   (OS_STK		  *	)&App_TaskManageStk[0],
						   (INT32U			) APP_TASK_MANAGE_STK_SIZE,
						   (void		  *	)0,
						   (INT16U			)(OS_TASK_OPT_STK_CLR |	OS_TASK_OPT_STK_CHK));

#if	(OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_MANAGE_PRIO,	(CPU_INT08U	*)"Start Task",	&os_err);
#endif

	OSStart();	/* Start multitasking (i.e.	give control to	uC/OS-II).	*/

	return (0);  
}

/*
*********************************************************************************************************
*										   App_TaskManage()
*
* Description :	The	startup	task.  The uC/OS-II	ticker should only be initialize once multitasking starts.
*
* Argument(s) :	p_arg		Argument passed	to 'App_TaskManage()' by 'OSTaskCreate()'.
*
* Return(s)	  :	none.
*
* Caller(s)	  :	This is	a task.
*
* Note(s)	  :	none.
*********************************************************************************************************
*/

static	void  App_TaskManage (void *p_arg)
{

	CPU_INT08U	os_err;
	u32 dev_Serial0=0, dev_Serial1=0, dev_Serial2=0;

	dev_Serial0 = *(vu32*)(0x1FFFF7E8);
	dev_Serial1 = *(vu32*)(0x1FFFF7EC);
	dev_Serial2 = *(vu32*)(0x1FFFF7F0);

	(void)p_arg;

	/* Initialize the SysTick.								*/
	OS_CPU_SysTickInit();

	//prompt("\r\n\r\n%s, line:	%d\r\n",__FILE__, __LINE__);
	prompt("Micrium	uC/OS-II V%d\r\n", OSVersion());
	prompt("TickRate: %d\t\t", OS_TICKS_PER_SEC);
	printf("OSCPUUsage: %d\r\n", OSCPUUsage);
	prompt("The MCU ID is %X %X %X\r\n",dev_Serial0,dev_Serial1,dev_Serial2);

#if	(OS_TASK_STAT_EN > 0)
	OSStatInit();												/* Determine CPU capacity.								*/
#endif

#ifdef GSM

	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskGsm,	/* Create the start	task.								*/
						   (void		  *	) 0,
						   (OS_STK		  *	)&App_TaskGsmStk[APP_TASK_GSM_STK_SIZE - 1],
						   (INT8U			) APP_TASK_GSM_PRIO,
						   (INT16U			) APP_TASK_GSM_PRIO,
						   (OS_STK		  *	)&App_TaskGsmStk[0],
						   (INT32U			) APP_TASK_GSM_STK_SIZE,
						   (void		  *	)0,
						   (INT16U			)(OS_TASK_OPT_STK_CLR |	OS_TASK_OPT_STK_CHK));

#if	(OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_GSM_PRIO, (CPU_INT08U *)"Gsm	Task", &os_err);
#endif

#endif //#ifdef GSM

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	while	(1)
	{

		if ( u1_rx_lost_data ) {//error! lost data
			prompt("Lost data, check: %s: %d\r\n",__FILE__, __LINE__);
			while ( 1 ) ;
		}

		if ( u1_rx_buf1_ava ) {//send buf1
			//send the receive char
			u1_tx_buf = u1_rx_buf1 ;
			u1_tx_cnt = u1_rx_cnt1;
			USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
			while ( u1_tx_cnt ) ;//wait send complete
			u1_rx_cnt1 = 0 ;
			u1_rx_buf1_ava = 0 ;
		}

		if ( u1_rx_buf2_ava ) {//send buf2
			//send the receive char
			u1_tx_buf = u1_rx_buf2 ;
			u1_tx_cnt = u1_rx_cnt2;
			USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
			while ( u1_tx_cnt ) ;//wait send complete
			u1_rx_cnt2 = 0 ;
			u1_rx_buf2_ava = 0 ;
		}

		//printf("L%010d\r\n",OSTime);
			
	
		/* Insert delay	*/
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
}
