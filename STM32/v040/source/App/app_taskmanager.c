#include "main.h"

#define	BUILD_DATE "iCar v04, built at "__DATE__" "__TIME__
#define	BUILD_REV  "$Rev$"

static	OS_STK		   App_TaskGsmStk[APP_TASK_GSM_STK_SIZE];
static void calc_sn( void );
static void conv_rev( char * );
static void flash_led( unsigned int );
static unsigned int calc_free_buffer(unsigned char *,unsigned char *,unsigned int);
static unsigned char mcu_id_eor( unsigned int );
static unsigned char gsm_send_sn( unsigned char *);
static unsigned char gsm_send_pcb( unsigned char *, unsigned char, unsigned int *);//protocol control byte
static unsigned char gsm_rx_decode( struct GSM_RX_RESPOND *);

struct CAR2SERVER_COMMUNICATION c2s_data ;// tx ���崦�����Ľ�

extern struct ICAR_DEVICE my_icar;

static unsigned char pro_sn[]="02P1xxxxxx";
//Last 6 bytes replace by MCU ID xor result
/* SN, char(10): 0  2  P  1 1  A  H  0 0 0
 *               �� �� �� �ܢ� �� �� ����
 *               �� 0:iCar low end, 1: iCar mid end, 2: iCar high end ...
 *               �� revision, 0~9, A~Z, except I,L,O
 *               �� part, 0:PCB, 1:PCBA, 2:case, 3:GSM antenna ... P:final product
 *               �ܢ� Year
 *               ��   Month, 1~9, 10:A, 11:B 12:C
 *               ��   Day,   1~9, A~Z, except I,L,O
 *               ���� serial number, 0~9, a~z, except i,l,o
*/

/*
*********************************************************************************************************
*										   App_TaskManage()
*
* Description :	The	startup	task.  The uC/OS-II	ticker should only be initialize once multitasking starts.
* Note(s)	  :	This is the firmware core, manage all the subtask
*********************************************************************************************************
*/

void  App_TaskManager (void *p_arg)
{

	CPU_INT08U	os_err;
	unsigned char var_uchar , gsm_sequence=0;
	unsigned int record_sequence = 0;
	struct GSM_RX_RESPOND mg323_rx_cmd;

	u16 adc;

	(void)p_arg;


	/* Initialize the queue.	*/
	for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
		c2s_data.queue_sent[var_uchar].send_timer= 0 ;//free queue if > 60 secs.
		c2s_data.queue_sent[var_uchar].send_pcb = 0 ;
	}
	c2s_data.queue_count = 0;

	c2s_data.tx_lock = false ;
	c2s_data.tx_timer= 0 ;

	c2s_data.rx_out_last = c2s_data.rx;
	c2s_data.rx_in_last  = c2s_data.rx;
	c2s_data.rx_empty = true ;
	c2s_data.rx_full = false ;
	c2s_data.rx_timer = 0 ;
	c2s_data.check_timer = 0 ;

	/* Initialize the SysTick.								*/
	OS_CPU_SysTickInit();
	//Note: the minimum time unit is 10ms,
	//Don't use: OSTimeDlyHMSM(0,0,0,1 );
	//At least : OSTimeDlyHMSM(0,0,0,10);
	/*
	while ( 1 ) {//for verify tick accuracy
		led_toggle( ALARM_LED ) ;
		OSTimeDlyHMSM(0, 0,	0, 10);
	}//2012/1/21 verified
	*/

	printf("\r\n"),	prompt("%s\r\n",BUILD_DATE);
	conv_rev(BUILD_REV);
	prompt("Revision: %d  OS Tick: %d\r\n",my_icar.rev,OS_TICKS_PER_SEC);
	show_rst_flag( ), show_err_log( );
 	
#if	(OS_TASK_STAT_EN > 0)
	OSStatInit();												/* Determine CPU capacity.								*/
#endif

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

	//enable temperature adc DMA
	//DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA1_Channel1->CCR |= DMA_CCR1_EN;

	my_icar.debug = 0 ;
	my_icar.login_timer = 0 ;//will be update in RTC_update_calibrate
	my_icar.err_log_send_timer = 0 ;//
	my_icar.need_sn = 3 ;
	my_icar.mg323.ask_power = true ;

	mg323_rx_cmd.timer = OSTime ;
	mg323_rx_cmd.start = c2s_data.rx;//prevent access unknow address
	mg323_rx_cmd.status = S_HEAD;

	calc_sn( );//prepare serial number

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	//flash LED, wait power stable and others task init
	flash_led( 80 );//100ms

	//independent watchdog init
	iwdg_init( );

	while	(1)
	{
		/* Reload IWDG counter */
		IWDG_ReloadCounter();  

		if ( c2s_data.tx_len > 0 || !my_icar.login_timer ) {//have command, need online
			my_icar.mg323.ask_online = true ;
		}
		else {//consider in the future
			;//my_icar.mg323.ask_online = false ;
		}

		//Send command
		if ( my_icar.need_sn && c2s_data.tx_sn_len == 0 ) {//no in sending process
			gsm_send_sn( &gsm_sequence );//will be return time also
		}

		if ( my_icar.login_timer ) {//send others CMD after login

			if ( (RTC_GetCounter( ) - my_icar.stm32_rtc.update_timer) > RTC_UPDATE_PERIOD ) {
				//need update RTC by server time, will be return time from server
				gsm_send_pcb(&gsm_sequence, GSM_CMD_TIME,&record_sequence);
			}

			if ( (OSTime - my_icar.err_log_send_timer) > TCP_RESPOND_TIMEOUT \
					&& BKP_ReadBackupRegister(BKP_DR1) ) {//ERR log index
				//middle task 
				if ( !gsm_send_pcb(&gsm_sequence, GSM_CMD_ERROR, &record_sequence)){
					//prompt("Upload err log and reset send_timer...\r\n");
					my_icar.err_log_send_timer = OSTime ;
					c2s_data.tx_timer = 0 ;//need send ASAP
				}
			}

			if ( my_icar.mg323.tcp_online ) {//record GSM signal and ADC, for testing
				//lowest task, just send when online
				gsm_send_pcb(&gsm_sequence, GSM_CMD_RECORD, &record_sequence);
			}
		}

		if ( !c2s_data.rx_empty ) {//receive some TCP data from GSM

			gsm_rx_decode( &mg323_rx_cmd );
		}

		if ( my_icar.stm32_u1_rx.lost_data ) {//error! lost data
			prompt("Uart1 lost data, check: %s: %d\r\n",__FILE__, __LINE__);
			my_icar.stm32_u1_rx.lost_data = false ;
		}

		while ( !my_icar.stm32_u1_rx.empty ) {//receive some data from console...
			var_uchar = getbyte( COM1 ) ;
			putbyte( COM1, var_uchar );

			if ( var_uchar == 'b' || var_uchar == 'B' ) {//show Backup register
				prompt("Print Backup register info ... \r\n");
				show_err_log( );
			}

			if ( var_uchar == 'o' || var_uchar == 'O' ) {//online
				prompt("Ask GSM online...\r\n");
				my_icar.mg323.ask_online = true ;
			}

			if ( var_uchar == 'c' || var_uchar == 'C' ) {//close
				prompt("Ask GSM off line...\r\n");
				my_icar.mg323.ask_online = false ;
			}

			if ( var_uchar == 'v' || var_uchar == 'V' ) {//show revision
				prompt("$URL$\r\n");
				prompt("$Id$\r\n");
				prompt("Revision: %d  OS Tick: %d\r\n",my_icar.rev,OS_TICKS_PER_SEC);
			}

			if ( var_uchar == 'd' ) {//set debug flag
				my_icar.debug++ ;
				prompt("Increase debug lever, my_icar.debug:%d\r\n",my_icar.debug);
			}

			if ( var_uchar == 'D' ) {//reset debug flag
				my_icar.debug = 0 ;
				prompt("Reset debug flag... my_icar.debug:%d\r\n",my_icar.debug);
			}

			if ( var_uchar == 'g' ) {//Suspend GSM task for debug
				os_err = OSTaskSuspend(APP_TASK_GSM_PRIO);
				if ( os_err == OS_NO_ERR ) {
					prompt("Suspend GSM task ok.\r\n");
				}
				else {
					prompt("Suspend GSM task err: %d\r\n",os_err);
				}
			}
			if ( var_uchar == 'G' ) {//Resume GSM task for debug
				//reset timer here, prevent reset GSM module
				my_icar.mg323.at_timer = OSTime ;

				os_err = OSTaskResume(APP_TASK_GSM_PRIO);
				if ( os_err == OS_NO_ERR ) {
					prompt("Resume GSM task ok.\r\n");
				}
				else {
					prompt("Resume GSM task err: %d\r\n",os_err);
				}
			}

		}

		if( my_icar.stm32_adc.completed ) {//temperature convert complete
			my_icar.stm32_adc.completed = false;

			//�˲�,ֻҪ���ݵ��м�һ��
			adc=digit_filter(my_icar.stm32_adc.converted,ADC_BUF_SIZE);

			//DMA_Cmd(DMA1_Channel1, ENABLE);
			DMA1_Channel1->CCR |= DMA_CCR1_EN;

			//adc=(1.42 - adc*3.3/4096)*1000/4.35 + 25;
			//�Ѽ���ٶ��뾫��
			adc = 142000 - ((adc*330*1000) >> 12);
			adc = adc*100/435+2500;

			if ( (OSTime/100)%10 == 0 ) {
				prompt("IP: %s\tT: %d.%02d C\t",my_icar.mg323.ip_local,adc/100,adc%100);
				RTC_show_time(RTC_GetCounter());
			}
		}
				 
		if ( (OSTime/100)%30 == 0 ) {//check every 30 sec, don't check last 2 queue
			for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE-2 ; var_uchar++) {
				if ( (OSTime - c2s_data.queue_sent[var_uchar].send_timer > CLEAN_QUEUE_PERIOD) \
					&& (c2s_data.queue_sent[var_uchar].send_pcb !=0)) {
					prompt("queue %d, CMD: 0x%02X timeout, reset!\r\n",\
						var_uchar,c2s_data.queue_sent[var_uchar].send_pcb);//60 secs.
					c2s_data.queue_sent[var_uchar].send_timer= 0 ;//free queue if > 1 min
					c2s_data.queue_sent[var_uchar].send_pcb = 0 ;
					if ( c2s_data.queue_count > 0 ) {
						c2s_data.queue_count--;
					}
				}
			}

			if ( c2s_data.queue_count > MAX_CMD_QUEUE/2 ) {
				c2s_data.tx_timer= 0 ;//need send immediately
				prompt("Free queue is: %d, RESET c2s_data.tx_timer @ %d\r\n",\
					MAX_CMD_QUEUE-c2s_data.queue_count,__LINE__);
			}
		}//end of check every 30 sec

		/* Insert delay, IWDG set to 2 second
		App_taskmanger: longest time, but highest priority 
		Othoers: shorter time, but lower priority */
		OSTimeDlyHMSM(0, 0,	1, 0);
		led_toggle( POWER_LED ) ;
	}
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

static void conv_rev( char *p )
{//$Rev$
	unsigned char i , j;

	my_icar.rev = 0 ;
	i = 0 , p = p + 6 ;
	while ( *(p+i) != 0x20 ) {
		i++ ;
		if ( *(p+i) == 0x24 || i > 4 ) break ; //$
	}

	j = 0 ;
	while ( i ) {
		i-- ;
		my_icar.rev = (*(p+i)-0x30)*pow(j) + my_icar.rev;
		j++;
	}
}

static void calc_sn( )
{
	static unsigned char chkbyte ;

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7E8));
	snprintf((char *)&pro_sn[4],3,"%02X",chkbyte);

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7EC));
	snprintf((char *)&pro_sn[6],3,"%02X",chkbyte);

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7F0));
	snprintf((char *)&pro_sn[8],3,"%02X",chkbyte);

	my_icar.sn = pro_sn ;

	prompt("The MCU ID is %X %X %X\tSN:%s\r\n",\
		*(vu32*)(0x1FFFF7E8),*(vu32*)(0x1FFFF7EC),*(vu32*)(0x1FFFF7F0),my_icar.sn);
}

static unsigned char mcu_id_eor( unsigned int id)
{
	static unsigned char chkbyte ;

	//Calc. MCU ID eor result as product SN
	chkbyte =  (id >> 24)&0xFF ;
	chkbyte ^= (id >> 16)&0xFF ;
	chkbyte ^= (id >> 8)&0xFF ;
	chkbyte ^= id&0xFF ;

	return chkbyte ;
}

static void flash_led( unsigned int i )
{
	led_off(POWER_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(RELAY_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(RELAY_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(ONLINE_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(ONLINE_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(RELAY_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(RELAY_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(POWER_LED);
}

//return 0: ok
//return 1: failure
static unsigned char gsm_rx_decode( struct GSM_RX_RESPOND *buf )
{
	unsigned char chkbyte, queue_index=0;
	unsigned int  chk_index, free_len=0;
	unsigned char len_high=0, len_low=0;
#if OS_CRITICAL_METHOD == 3   /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	//status transfer: S_HEAD -> S_PCB -> S_CHK ==> S_HEAD...
	//note:��S_PCB��ʼ��ʱqueue_time,���>5*AT_TIMEOUT,������״̬ΪS_HEAD
	//1, find HEAD
	if ( buf->status == S_HEAD ) {
		while ( !c2s_data.rx_empty && buf->status == S_HEAD) {

			if ( *c2s_data.rx_out_last == GSM_HEAD ) {//found
				buf->start = c2s_data.rx_out_last ;//mark the start
				buf->status = S_PCB ;//search protocol control byte
				buf->timer = OSTime ;
				if ( my_icar.debug > 3) {
					prompt("Found HEAD: %X\r\n",buf->start);
				}
			}
			else { //no found
				c2s_data.rx_out_last++;//search next byte
				if (c2s_data.rx_out_last==c2s_data.rx+GSM_BUF_LENGTH) {
					c2s_data.rx_out_last = c2s_data.rx; //��ַ������,�ص��ײ�
				}

				OS_ENTER_CRITICAL();
				c2s_data.rx_full = false; //reset the full flag
				if (c2s_data.rx_out_last==c2s_data.rx_in_last) {
					c2s_data.rx_empty = true ;//set the empty flag
				}
				OS_EXIT_CRITICAL();

			}
		}
	}

	//2, find PCB&SEQ&LEN
	if ( buf->status == S_PCB ) {

		if ( OSTime - buf->timer > 5*AT_TIMEOUT ) {//reset status
			prompt("In S_PCB timeout, reset to S_HEAD status!!!\r\n");
			buf->status = S_HEAD ;
			c2s_data.rx_out_last++	 ;
		}

		free_len = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		if ( free_len > 5 ) {
			//HEAD SEQ CMD Length(2 bytes) = 5 bytes

			//get PCB
			if ( (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//PCB no in the end of buffer
				buf->pcb = *(buf->start + 2);
			}
			else { //PCB in the end of buffer
				buf->pcb = *(buf->start + 2 - GSM_BUF_LENGTH);
			}//end of (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH)

			if ( my_icar.debug > 3) {
				printf("\r\nrespond PCB is %02X\t",buf->pcb);
			}

			//get sequence
			if ( (buf->start + 1)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//SEQ no in the end of buffer
				buf->seq = *(buf->start + 1);
			}
			else { //SEQ in the end of buffer
				buf->seq = *(buf->start + 1 - GSM_BUF_LENGTH);
			}//end of (buf->start + 1)  < (c2s_data.rx+GSM_BUF_LENGTH)
			//printf("SEQ is %02X\t",buf->seq);

			//get len high
			if ( (buf->start + 3)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				len_high = *(buf->start + 3);
			}
			else { //LEN in the end of buffer
				len_high = *(buf->start + 3 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//printf("respond LEN H: %02d\t",len_high);

			//get len low
			if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				len_low = *(buf->start + 4);
			}
			else { //LEN in the end of buffer
				len_low = *(buf->start + 4 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//printf("L: %02d ",len_low);

			buf->len = len_high << 8 | len_low ;
			//printf("respond LEN: %d\r\n",buf->len);

			//update the status & timer
			buf->status = S_CHK ;//search check byte
			buf->timer = OSTime ;
		}//end of (c2s_data.rx_in_last > c2s_data.rx_out_last) ...
	}//end of if ( buf->status == S_PCB )

	//3, find CHK
	if ( buf->status == S_CHK ) {

		if ( OSTime - buf->timer > 10*AT_TIMEOUT ) {//reset status
			prompt("In S_CHK timeout, reset to S_HEAD status!!!\r\n");
			buf->status = S_HEAD ;
			c2s_data.rx_out_last++	 ;
		}

		free_len = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		if ( free_len > (5+buf->len) ) {
			//buffer > HEAD SEQ CMD Length(2 bytes) = 5 bytes + LEN + CHK(1)

			//get CHK
			if ( (buf->start + 5+buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//CHK no in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len );
				//printf("CHK add: %X\t",buf->start + 5 + buf->len);
			}
			else { //CHK in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len - GSM_BUF_LENGTH);
				//printf("CHK add: %X\t",buf->start + 5 + buf->len - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//2012/1/4 19:54:50 ����֤�߽����������

			chkbyte = GSM_HEAD ;
			for ( chk_index = 1 ; chk_index < buf->len+5 ; chk_index++ ) {//calc chkbyte
				if ( (buf->start+chk_index) < c2s_data.rx+GSM_BUF_LENGTH ) {
					chkbyte ^= *(buf->start+chk_index);
				}
				else {//data in begin of buffer
					chkbyte ^= *(buf->start+chk_index-GSM_BUF_LENGTH);
				}
			}

			if ( chkbyte == buf->chk ) {//data correct

				if ( my_icar.debug > 1) {
					prompt("CMD: %c ,  ",(buf->pcb)&0x7F);
				}
				//find the sent record in c2s_data.queue_sent by SEQ
				for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
					if ( c2s_data.queue_sent[queue_index].send_seq == buf->seq \
						&& buf->pcb==(c2s_data.queue_sent[queue_index].send_pcb | 0x80)) { 
						if ( my_icar.debug > 1 ) {
							printf("queue: %d seq: %d match.\r\n",queue_index,buf->seq);
						}
						//found, free this record
						c2s_data.queue_sent[queue_index].send_pcb = 0 ;
						if ( c2s_data.queue_count > 0 ) {
							c2s_data.queue_count--;
						}
						break ;
					}//end of c2s_data.queue_sent[queue_index].send_pcb == 0
				}

				//handle the respond
				switch (buf->pcb&0x7F) {

				case 0x4C://'L':
					break;

				case GSM_CMD_ERROR://0x45,'E'  C9 3B C5 00 01 00 36
					//prompt("Record respond PCB @ %08X, %08X~%08X\r\n",\
						//buf->start,c2s_data.rx,c2s_data.rx+GSM_BUF_LENGTH);
					my_icar.err_log_send_timer = 0 ;//can send others
					switch (*((buf->start)+5)) {

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("Upload err log failure, need product SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));

						break;

					case 0x2://Insert DB err
						prompt("Upload err log failure, insert into database error! \
							CMD_seq: %02X\r\n",*((buf->start)+1));
						break;

					//BKP_DR1, ERR index: 	15~12:MCU reset 
					//						11~8:reverse
					//						7~4:GPRS disconnect reason
					//						3~0:GSM module poweroff reason
					case 0x10://record err log part 1: GSM module poweroff success
						prompt("Upload GSM module poweroff err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						//Clear the error flag
						//BKP_DR2, GSM Module power off time(UTC Time) high
						//BKP_DR3, GSM Module power off time(UTC Time) low
					    BKP_WriteBackupRegister(BKP_DR2, 0);//high
					    BKP_WriteBackupRegister(BKP_DR3, 0);//low
						BKP_WriteBackupRegister(BKP_DR1, \
							((BKP_ReadBackupRegister(BKP_DR1))&0xFFF0));

						break;

					case 0x20://record err log part 2: GPRS disconnect success
						prompt("Upload GPRS disconnect err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						//Clear the error flag
						//BKP_DR4, GPRS disconnect time(UTC Time) high
						//BKP_DR5, GPRS disconnect time(UTC Time) low
					    BKP_WriteBackupRegister(BKP_DR4, 0);//high
					    BKP_WriteBackupRegister(BKP_DR5, 0);//low
						BKP_WriteBackupRegister(BKP_DR1, \
							((BKP_ReadBackupRegister(BKP_DR1))&0xFF0F));

						break;

					case 0x30://record err log part 3: rev success
						prompt("Upload rev err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						//Clear the error flag
						BKP_WriteBackupRegister(BKP_DR1, \
							((BKP_ReadBackupRegister(BKP_DR1))&0xF0FF));

						break;

					case 0x40://record err log part 4: MCU reset
						prompt("Upload MCU reset err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						//Clear the error flag
						//BKP_DR8, MCU reset time(UTC Time) high
						//BKP_DR9, MCU reset time(UTC Time) low
					    BKP_WriteBackupRegister(BKP_DR8, 0);//high
					    BKP_WriteBackupRegister(BKP_DR9, 0);//low
						BKP_WriteBackupRegister(BKP_DR1, \
							((BKP_ReadBackupRegister(BKP_DR1))&0x0FFF));

						break;

					default:
						prompt("Upload err log failure, unknow error code: 0x%02X CMD_seq: %02X ",\
								*((buf->start)+5),*((buf->start)+1));
						printf("check %s: %d\r\n",__FILE__,__LINE__);
						break;
					}
					break;

				case GSM_CMD_RECORD://0x52,'R'
					//C9 06 D2 00 01 01 1D
					//prompt("Record respond PCB @ %08X, %08X~%08X\r\n",\
						//buf->start,c2s_data.rx,c2s_data.rx+GSM_BUF_LENGTH);

					switch (*((buf->start)+5)) {

					case 0x0://record success
						if ( my_icar.debug > 2) {
							prompt("Record GSM signal success, CMD_seq: %02X\r\n",*((buf->start)+1));
						}
						break;

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("Record CMD failure, need product SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));

						break;

					case 0x2://Insert DB err
						prompt("Record CMD failure, insert into database error! \
							CMD_seq: %02X\r\n",*((buf->start)+1));
						break;

					default:
						prompt("Record CMD failure, unknow error code: 0x%02X CMD_seq: %02X ",\
								*((buf->start)+5),*((buf->start)+1));
						printf("check %s: %d\r\n",__FILE__,__LINE__);
						break;
					}
					break;

				case GSM_CMD_SN://0x53,'S'
					//C9 08 D4 00 04 4F 0B CD E5 7D

					switch (*((buf->start)+4)) {

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("SN CMD failure! CMD_seq: %02X\r\n",\
							*((buf->start)+1));
						break;

					case 0x4://CMD success
						my_icar.need_sn = 0 ;
						//Update and calibrate RTC
						prompt("Upload SN CMD success, CMD_seq: %02X\r\n",*((buf->start)+1));
						RTC_update_calibrate(buf->start,c2s_data.rx) ;
						my_icar.need_sn = 0 ;
						c2s_data.tx_timer = 0 ;//need send others queue ASAP
						break;

					default:
						prompt("SN CMD failure, unknow error code: 0x%02X CMD_seq: %02X ",\
								*((buf->start)+5),*((buf->start)+1));
						printf("check %s: %d\r\n",__FILE__,__LINE__);
						break;
					}

					break;

				case GSM_CMD_TIME://0x54,'T'
					//C9 08 D4 00 04 4F 0B CD E5 7D

					switch (*((buf->start)+4)) {

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("Time CMD failure, need product SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));

						break;

					case 0x4://CMD success
						if ( my_icar.debug > 2) {
							prompt("Time CMD success, CMD_seq: %02X\r\n",*((buf->start)+1));
						}
						//Update and calibrate RTC, update my_icar.login_timer also
						RTC_update_calibrate(buf->start,c2s_data.rx) ;

						break;

					default:
						prompt("Record CMD failure, unknow error code: 0x%02X CMD_seq: %02X ",\
								*((buf->start)+5),*((buf->start)+1));
						printf("check %s: %d\r\n",__FILE__,__LINE__);
						break;
					}
					break;

				default:
					prompt("Unknow respond PCB: 0x%X\r\n",buf->pcb&0x7F);

					break;
				}//end of handle the respond

			}//end of if ( chkbyte == buf->chk )
			else {//data no correct
				prompt("Rec data CHK no correct!\r\n");
				prompt("Calc. CHK: %02X\t",chkbyte);
				printf("respond CHK: %02X\r\n",buf->chk);
			}

			//Clear buffer content
			for ( chk_index = 0 ; chk_index < buf->len+6 ; chk_index++ ) {
				if ( (buf->start+chk_index) < c2s_data.rx+GSM_BUF_LENGTH ) {
					*(buf->start+chk_index) = 0x0;
				}
				else {//data in begin of buffer
					*(buf->start+chk_index-GSM_BUF_LENGTH) = 0x0;
				}
			}

			//update the buffer point
			if ( (buf->start + 6+buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				c2s_data.rx_out_last = buf->start + 6 + buf->len ;
			}
			else { //CHK in the end of buffer
				c2s_data.rx_out_last = buf->start + 6 + buf->len - GSM_BUF_LENGTH ;
			}

			OS_ENTER_CRITICAL();
			c2s_data.rx_full = false; //reset the full flag
			if (c2s_data.rx_out_last==c2s_data.rx_in_last) {
				c2s_data.rx_empty = true ;//set the empty flag
			}
			OS_EXIT_CRITICAL();

			if ( my_icar.debug > 3) {
				prompt("Next add: %X\t",c2s_data.rx_out_last);
				printf("In last: %X\t",c2s_data.rx_in_last);
				printf("rx_empty: %X\r\n",c2s_data.rx_empty);
			}

			//update the next status
			buf->status = S_HEAD;

		}//end of (c2s_data.rx_in_last > c2s_data.rx_out_last) ...
		else { //
			if ( my_icar.debug > 3) {
				prompt("Buffer no enough.\r\n");
			}
		}
	}//end of if ( buf->status == S_CHK )

	return 0 ;
}

static unsigned int calc_free_buffer(unsigned char *in,unsigned char *out,unsigned int len)
{//����ǰ����ȷ�� buffer ��Ϊ��

	if ( in == out ) {
		return 0 ;//full
	}
	else {
		if ( in > out ) {
			return (in-out);
		}
		else { //in < out
			return (len-(out-in));
		}
	}
}

//return 0: ok
//return 1: no send queue
//return 2: no free buffer or buffer busy
static unsigned char gsm_send_pcb( unsigned char *sequence, unsigned char out_pcb, unsigned int *record_seq)
{
	static unsigned char chkbyte, index;
	static u16 i ;
	static unsigned int seq ;

#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	i = 0 , seq = 0 ;
	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < MAX_CMD_QUEUE-4 ; index++) {

		if ( c2s_data.queue_sent[index].send_pcb == 0 ) { //no use

			//prompt("Send PCB: %c, tx_len: %d, tx_lock: %d, ",\
				out_pcb,c2s_data.tx_len,c2s_data.tx_lock);
			//printf("queue: %02d\r\n",index);

			if ( !c2s_data.tx_lock \
				&& c2s_data.tx_len < (GSM_BUF_LENGTH-13) ) {
				//no process occupy && have enough buffer
				//Max len: Head+SEQ+CMD+Len+err_time+err_code+CHK
				OS_ENTER_CRITICAL();
				c2s_data.tx_lock = true ;
				OS_EXIT_CRITICAL();

				//save pcb to queue
				c2s_data.queue_sent[index].send_timer= OSTime ;
				c2s_data.queue_sent[index].send_seq = *sequence ;
				c2s_data.queue_sent[index].send_pcb = out_pcb ;
				c2s_data.queue_count++;
	
				//prepare GSM command
				c2s_data.tx[c2s_data.tx_len]   = GSM_HEAD ;
				c2s_data.tx[c2s_data.tx_len+1] = *sequence ;//SEQ
				*sequence = c2s_data.tx[c2s_data.tx_len+1]+1;//increase seq
				c2s_data.tx[c2s_data.tx_len+2] = out_pcb ;//PCB

				if ( out_pcb == GSM_CMD_TIME ) {//Max. 6 Bytes
					my_icar.stm32_rtc.update_timer = RTC_GetCounter( );

					c2s_data.tx[c2s_data.tx_len+3] = 0;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 0;//length low

					if ( my_icar.debug > 3) {
						prompt("GSM CMD: %02X ",c2s_data.tx[c2s_data.tx_len]);
					}
					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < 5 ; i++ ) {//calc chkbyte
						chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
						if ( my_icar.debug > 3) {
							printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
						}
					}
					c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;
					if ( my_icar.debug > 3) {
						printf("%02X\r\n",c2s_data.tx[c2s_data.tx_len+i]);
					}
					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
				}

				if ( out_pcb == GSM_CMD_ERROR ) {//Max. 13 Bytes
					//HEAD SEQ PCB Length(2 bytes) DATA(6 Bytes) check
					//DATA: err_time(4 bytes)+err_code(2 byte)
	
					//Backup register, 16 bit = 2 bytes * 10 for STM32R8
					//BKP_DR1, ERR index: 	15~12:MCU reset 
					//						11~8:reverse
					//						7~4:GPRS disconnect reason
					//						3~0:GSM module poweroff reason
					//BKP_DR2, GSM Module power off time(UTC Time) high
					//BKP_DR3, GSM Module power off time(UTC Time) low
					//BKP_DR4, GPRS disconnect time(UTC Time) high
					//BKP_DR5, GPRS disconnect time(UTC Time) low
					//BKP_DR6, reverse time(UTC Time) high
					//BKP_DR7, reverse time(UTC Time) low
					//BKP_DR8, MCU reset time(UTC Time) high
					//BKP_DR9, MCU reset time(UTC Time) low

					c2s_data.tx[c2s_data.tx_len+3] = 0 ;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 6 ;//length low

					i = BKP_ReadBackupRegister(BKP_DR1);
					//err_code, 2 byte
					c2s_data.tx[c2s_data.tx_len+9] = (i>>8)&0xFF;
					c2s_data.tx[c2s_data.tx_len+10]= (i)&0xFF;
					if ( i & 0xF000 ) { //highest priority, send first
						//MCU reset time, 4 bytes
						seq = BKP_ReadBackupRegister(BKP_DR8) ;
						c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
						seq = BKP_ReadBackupRegister(BKP_DR9) ;
						c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low
					}
					else {
						if ( i & 0x0F00 ) { //higher priority
							;//reverse
						}
						else {	
							if ( i & 0x00F0 ) { //GPRS disconnect err
								//gprs disconnect time, 4 bytes
								seq = BKP_ReadBackupRegister(BKP_DR4) ;
								c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
								c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
								seq = BKP_ReadBackupRegister(BKP_DR5) ;
								c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
								c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low
							}
							else {		
								if ( i & 0x000F ) { //GSM module poweroff err
									//GSM power off time, 4 bytes
									seq = BKP_ReadBackupRegister(BKP_DR2) ;
									c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
									c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
									seq = BKP_ReadBackupRegister(BKP_DR3) ;
									c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
									c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low
								}
								else {//program logic error
									prompt("firmware logic error, chk: %s: %d\r\n",\
										__FILE__,__LINE__);			
								}//end (i & 0x000F)
							}//end (i & 0x00F0)
						}//end (i & 0x0F00)
					}//end (i & 0xF000)

	
					//prompt("GSM CMD: %02X ",c2s_data.tx[c2s_data.tx_len]);
					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < 11 ; i++ ) {//calc chkbyte
						chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
						//printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					}
					c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;
					//printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
				}

				if ( out_pcb == GSM_CMD_RECORD ) {//Max. 9 Bytes
					//HEAD SEQ PCB Length(2 bytes) DATA(5 bytes) check
					//DATA: Record_seq(2 bytes)+GSM signal(1byte)+voltage(2 bytes)

					c2s_data.tx[c2s_data.tx_len+3] = 0 ;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 5 ;//length low
	
					//record sequence, 2 bytes
					seq = *record_seq ;
					c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//seq high
					c2s_data.tx[c2s_data.tx_len+6] =  seq&0xFF  ;//seq low
					seq++, *record_seq=seq;//increase seq
	
					//GSM signal, 1 byte
					c2s_data.tx[c2s_data.tx_len+7] = my_icar.mg323.signal;
	
					//ADC, 2 byte
					c2s_data.tx[c2s_data.tx_len+8] = 0;
					c2s_data.tx[c2s_data.tx_len+9] = 0;
	
					//prompt("GSM CMD: %02X ",c2s_data.tx[c2s_data.tx_len]);
					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < 10 ; i++ ) {//calc chkbyte
						chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
						//printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					}
					c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;
					//printf("%02X\r\n",c2s_data.tx[c2s_data.tx_len+i]);
					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
				}

				OS_ENTER_CRITICAL();
				c2s_data.tx_lock = false ;
				OS_EXIT_CRITICAL();

				return 0 ;
			}//end of if ( !c2s_data.tx_lock && c2s_data.tx_len < (GSM_BUF_LENGTH-20))
			else {//no buffer
				if ( c2s_data.tx_lock ) {
					prompt("TCP tx buffer lock: %d, can't add CMD: %c ",\
							c2s_data.tx_lock,GSM_CMD_TIME);
				}
				else {
					prompt("TCP tx free buffer: %d ",GSM_BUF_LENGTH-c2s_data.tx_len);
				}
				printf("check %s:%d\r\n",__FILE__, __LINE__);
				return 2;
			}
		}//end of c2s_data.queue_sent[index].send_pcb == 0
	}

	prompt("No free queue: %d check %s:%d\r\n",index,__FILE__, __LINE__);
	return 1;
}

//return 0: ok
//return 1: no send queue
//return 2: no free buffer or buffer busy
static unsigned char gsm_send_sn( unsigned char *sequence)
{
	static unsigned char i, chkbyte, index,ip_length;

	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < MAX_CMD_QUEUE-2 ; index++) {

		if ( c2s_data.queue_sent[index].send_pcb == 0 ) { //no use

	
			my_icar.stm32_rtc.update_timer = RTC_GetCounter( ) ;

			//HEAD SEQ CMD Length(2 bytes) OSTime SN(char 10) IP check

			if ( strlen((char *)my_icar.mg323.ip_local) > 7 ) {//1.2.3.4
	
				prompt("Send SN, tx_sn_len: %d, ",c2s_data.tx_len);
				printf("queue: %02d\r\n",index);

				//set protocol string value
				c2s_data.queue_sent[index].send_timer= OSTime ;
				c2s_data.queue_sent[index].send_seq = *sequence ;
				c2s_data.queue_sent[index].send_pcb = GSM_CMD_SN ;
				c2s_data.queue_count++;
	
				ip_length = strlen((char *)my_icar.mg323.ip_local) ;

				//prepare GSM command
				c2s_data.tx_sn[0]   = GSM_HEAD ;
				c2s_data.tx_sn[1] = *sequence ;//SEQ
				*sequence = c2s_data.tx_sn[1]+1;//increase seq
				c2s_data.tx_sn[2] = GSM_CMD_SN ;//PCB
				c2s_data.tx_sn[3] = 0  ;//length high
				c2s_data.tx_sn[4] = ip_length+14;//len(SN)+len(OSTime)

				//record OSTime, 4 bytes
				c2s_data.tx_sn[5] = (OSTime>>24)&0xFF  ;//OSTime high
				c2s_data.tx_sn[6] = (OSTime>>16)&0xFF  ;//OSTime high
				c2s_data.tx_sn[7] = (OSTime>>8)&0xFF  ;//OSTime low
				c2s_data.tx_sn[8] =  OSTime&0xFF  ;//OSTime low

				strncpy((char *)&c2s_data.tx_sn[9], (char *)my_icar.sn, 10);

				//Local IP
				strncpy((char *)&c2s_data.tx_sn[19], \
						(char *)my_icar.mg323.ip_local, IP_LEN-1);

				//prompt("SN CMD: %02X ",c2s_data.tx_sn[0]);
				chkbyte = GSM_HEAD ;
				for ( i = 1 ; i < 19+ip_length ; i++ ) {//calc chkbyte
					chkbyte ^= c2s_data.tx_sn[i];
					//printf("%02X ",c2s_data.tx_sn[i]);
				}
				c2s_data.tx_sn[i] = chkbyte ;
				//printf("%02X\r\n",c2s_data.tx_sn[i]);
				c2s_data.tx_sn_len = i+1 ;

				return 0 ;
			}//end of if ( strlen(my_icar.mg323.ip_local) > 7 )
			else {//no buffer
				//prompt("check %s:%d\r\n",__FILE__, __LINE__);
				return 2;
			}
		}//end of c2s_data.queue_sent[index].send_pcb == 0
	}

	prompt("No free queue: %d check %s:%d\r\n",index,__FILE__, __LINE__);
	return 1;
}