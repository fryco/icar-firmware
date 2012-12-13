#include "main.h"

#define	BUILD_DATE "iCar Boot loader, built at "__DATE__" "__TIME__", $Rev: 173 $\r\n"

unsigned int jump_address;
unsigned int page_size; 
unsigned int upgrade_base; 

typedef  void (*pFunction)(void);
pFunction jump_application;

void putbyte( char c)
{
	USART1->DR = (u8)c ;
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}
}

//*************************************
//发送一个字节的hex码，分成两个字节发。
unsigned char hex_[]={"0123456789ABCDEF"}; 

void puthex(unsigned char c)
{
	int ch;
	ch=(c>>4)&0x0f;
	putbyte(hex_[ch]);
	ch=c&0x0f;
	putbyte(hex_[ch]);
}

void putstring(unsigned char  *puts)
{	

	for (;*puts!=0;puts++) {  //遇到停止符0结束
		putbyte(*puts);
	}
}

void delay_ms(u32 ms)
{
	int j,i;
	for(i = 0; i < ms; i++){
		for(j = 0; j < 4000; j++){
			__nop();
		}
	}
}

void uart1_initial( void ) 
{
	USART_InitTypeDef USART1_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* USARTx configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
            RCC_APB2Periph_AFIO |
            RCC_APB2Periph_USART1 , 
            ENABLE);

	USART1_InitStructure.USART_BaudRate = 115200;
	USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART1_InitStructure.USART_StopBits = USART_StopBits_1;
	USART1_InitStructure.USART_Parity = USART_Parity_No ;
	USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART1_InitStructure);

	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);

	USART_Cmd(USART1, ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//use internal pull up
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void blink_led( unsigned int i )
{
	led_on(POWER_LED);
	led_on(ALARM_LED);
	//led_on(RELAY_LED);
	led_on(ONLINE_LED);
	delay_ms( i );
	led_off(POWER_LED);
	led_off(ALARM_LED);
	//led_off(RELAY_LED);
	led_off(ONLINE_LED);
	delay_ms( i );
}

int	main(void)
{
	u16 var_u16 ;

	//delay, wait power stable
	delay_ms( 100 );
	
	gpio_init( ) ;
	uart1_initial( );
	putstring("\r\n");

	led_init_all( );

	led_on(POWER_LED);

	GSM_PM_OFF;
	GPS_PM_OFF;
	SD_PM_OFF;

	putstring(BUILD_DATE);
	putstring("\r\n");

	if ( *(vu16*)(0x1FFFF7E0) >= 256 ) {
		page_size = 0x800; //2KB
		upgrade_base = 0x08000000 + ((*(vu16*)(0x1FFFF7E0))>>2)*page_size;
		putstring("Page size: 2KB\r\n");
	}
	else { 
		page_size = 0x400; //1KB
		upgrade_base = 0x08010C00 ;	//Page67
		putstring("Page size: 1KB\r\n");
	}


	//check flag correct?
	if ( (*(vu32*)(upgrade_base+FW_READY_ADD) == FW_READY_FLAG) &&\
		(*(vu32*)(upgrade_base+FW_READY_ADD+4) == ~FW_READY_FLAG ) ) {

		putstring("Find new firmware, upgrade...\r\n");
		if ( flash_upgrade( )) {
			//failure
			while ( 1 ) {
				blink_led( 400 );
				putstring("Upgrade failure, need send to factory!\r\n\r\n");
			}
		}

		/* Enable PWR and BKP clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

		/* Allow access to BKP Domain */
		PWR_BackupAccessCmd(ENABLE);

		//BKP_DR1, ERR index: 	15~12:MCU reset 
		//						11~8:upgrade fw success flag
		//						7~4:GPRS disconnect reason
		//						3~0:GSM module poweroff reason
		var_u16 = (BKP_ReadBackupRegister(BKP_DR1))&0xF0FF;
		var_u16 = var_u16 | (ERR_UPGRADE_SUCCESSFUL<<8) ; //fw upgrade successful
	    BKP_WriteBackupRegister(BKP_DR1, var_u16);

		//BKP_DR6, upgrade fw time(UTC Time) high
		//BKP_DR7, upgrade fw time(UTC Time) low
	    BKP_WriteBackupRegister(BKP_DR6, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
	    BKP_WriteBackupRegister(BKP_DR7, (RTC_GetCounter( ))&0xFFFF);//low

		putstring("\r\nUpgrade successful, load new firmware...\r\n");
	}
	else {
		putstring("Load main application ...\r\n\r\n");
	}

	//boot with main app
	jump_address = *(vu32*) (APPLICATION_ADDRESS + 4);
	/* Jump to user application */
	jump_application = (pFunction) jump_address;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(vu32*) APPLICATION_ADDRESS);
	jump_application();
}
