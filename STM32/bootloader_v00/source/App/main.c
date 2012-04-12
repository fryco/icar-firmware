#include "main.h"

#define	BUILD_DATE "iCar Boot loader, built at "__DATE__" "__TIME__

unsigned char BUILD_REV[] __attribute__ ((section ("FW_REV"))) ="$Rev: 121 $";

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

}

unsigned int pow( unsigned char n)
{
	unsigned int result ;

	result = 1 ;
	while ( n ) {
		result = result*10;
		n--;
	}
	return result;
}

static void conv_rev( unsigned char *p , u16 *fw_rev)
{//$Rev: 9999 $
	unsigned char i , j;

	i = 0 , p = p + 6 ;
	while ( *(p+i) != 0x20 ) {
		i++ ;
		if ( *(p+i) == 0x24 || i > 4 ) break ; //$
	}

	j = 0 ;
	while ( i ) {
		i-- ;
		*fw_rev = (*(p+i)-0x30)*pow(j) + *fw_rev;
		j++;
	}
}

static void flash_led( unsigned int i )
{
	led_off(POWER_LED);
	delay_ms( i );

	led_on(ALARM_LED);
	delay_ms( i );
	led_off(ALARM_LED);
	delay_ms( i );

	led_on(RELAY_LED);
	delay_ms( i );
	led_off(RELAY_LED);
	delay_ms( i );

	led_on(ONLINE_LED);
	delay_ms( i );
	led_off(ONLINE_LED);
	delay_ms( i );

	led_on(RELAY_LED);
	delay_ms( i );
	led_off(RELAY_LED);
	delay_ms( i );

	led_on(ALARM_LED);
	delay_ms( i );
	led_off(ALARM_LED);
	delay_ms( i );

	led_on(POWER_LED);
}

int	main(void)
{

	my_icar.debug = 0 ;
	my_icar.fw_rev = 0 ;

	NVIC_Configuration( );
	
	gpio_init( ) ;
	uart1_init( );

	rtc_init();

	uart3_init( );

	/* Enable CRC clock */
	//RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

	led_init_all( );
	led_on(POWER_LED);

	//GSM_PM_ON;
	//while ( 1 ) ;//for upgrade GSM fw
	GSM_PM_OFF;
	GPS_PM_OFF;
	SD_PM_OFF;

	show_rst_flag( );
	
	show_err_log( );

	printf("\r\n%s\r\n",BUILD_DATE);

	conv_rev((unsigned char *)BUILD_REV,&my_icar.fw_rev);

	printf("Revision: %d\r\n",my_icar.fw_rev);

	while ( 1 ) {
		flash_led( 200 );
		printf("Upgrade complete, will reset MCU!\r\n\r\n");
	}
}
