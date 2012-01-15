#include "main.h"

static	OS_STK		   App_TaskGsmStk[APP_TASK_GSM_STK_SIZE];
static void calc_sn( void );
static unsigned char mcu_id_eor( unsigned int );
static unsigned char gsm_send_time( struct SENT_QUEUE *, unsigned char *);
static unsigned char gsm_rx_decode( struct GSM_RX *, struct SENT_QUEUE *queue_p);

extern struct UART_RX u1_rx_buf;
extern struct UART_TX u1_tx_buf;
extern struct GSM_STATUS mg323_status ;
extern struct GSM_COMMAND mg323_cmd ;
extern struct RTC_STATUS stm32_rtc;
extern struct ICAR_ADC adc_temperature;

static unsigned char pro_sn[]="02P1xxxxxx";
//Last 6 bytes replace by MCU ID xor result
/* SN, char(10): 0  2  P  1 1  A  H  0 0 0
 *               ① ② ③ ④⑤ ⑥ ⑦ ⑧⑨⑩
 *               ① 0:iCar low end, 1: iCar mid end, 2: iCar high end ...
 *               ② revision, 0~9, A~Z, except I,L,O
 *               ③ part, 0:PCB, 1:PCBA, 2:case, 3:GSM antenna ... P:final product
 *               ④⑤ Year
 *               ⑥   Month, 1~9, 10:A, 11:B 12:C
 *               ⑦   Day,   1~9, A~Z, except I,L,O
 *               ⑧⑨⑩ serial number, 0~9, a~z, except i,l,o
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
	unsigned char var_uchar , gsm_sequence=0xC0;

	struct SENT_QUEUE gsm_sent_q[MAX_CMD_QUEUE];
	struct GSM_RX mg323_rx_buf;

	u16 adc;

	(void)p_arg;

	/* Initialize the queue.	*/
	for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
		gsm_sent_q[var_uchar].send_time= 0 ;//free queue if > 1 hours
		gsm_sent_q[var_uchar].send_pcb = 0 ;
	}

	/* Initialize the SysTick.								*/
	OS_CPU_SysTickInit(); 

	//prompt("\r\n\r\n%s, line:	%d\r\n",__FILE__, __LINE__);
	prompt("Micrium	uC/OS-II V%d.%d\r\n", OSVersion()/100,OSVersion()%100);
	prompt("TickRate: %d\t\t", OS_TICKS_PER_SEC);
	printf("OSCPUUsage: %d\r\n", OSCPUUsage);

	calc_sn( );//prepare serial number

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


	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	//enable temperature adc DMA
	//DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA1_Channel1->CCR |= DMA_CCR1_EN;

	//wait power stable
	OSTimeDlyHMSM(0, 0,	1, 0);

	//independent watchdog init
	iwdg_init( );

	mg323_status.ask_power = true ;
	mg323_rx_buf.time = OSTime ;
	mg323_rx_buf.start = mg323_cmd.rx;//prevent access unknow address
	mg323_rx_buf.status = S_HEAD;

	while	(1)
	{
		/* Reload IWDG counter */
		IWDG_ReloadCounter();  

		if ( mg323_cmd.tx_len > 0 ) {//have command, need online
			mg323_status.ask_online = true ;
		}
		else {//consider in the future
			;//mg323_status.ask_online = false ;
		}

		//Send command
		if ( (RTC_GetCounter( ) - stm32_rtc.update_time) > RTC_UPDATE_PERIOD || \
				stm32_rtc.update_time == 0 ) {//need update RTC by server time

			gsm_send_time( gsm_sent_q, &gsm_sequence );
		}

		if ( !mg323_cmd.rx_empty ) {//receive some TCP data from GSM

			gsm_rx_decode( &mg323_rx_buf, gsm_sent_q );
		}

		if ( u1_rx_buf.lost_data ) {//error! lost data
			prompt("Uart1 lost data, check: %s: %d\r\n",__FILE__, __LINE__);
			u1_rx_buf.lost_data = false ;
		}

		while ( !u1_rx_buf.empty ) {//receive some data from console...
			var_uchar = getbyte( COM1 ) ;
			putbyte( COM1, var_uchar );
			if ( var_uchar == 'o' || var_uchar == 'O' ) {//online
				prompt("Ask GSM online...\r\n");
				mg323_status.ask_online = true ;
			}
			if ( var_uchar == 'c' || var_uchar == 'C' ) {//close
				prompt("Ask GSM off line...\r\n");
				mg323_status.ask_online = false ;
			}
		}

		if( adc_temperature.completed ) {//temperature convert complete
			adc_temperature.completed = false;

			//滤波,只要数据的中间一段
			adc=digit_filter(adc_temperature.converted,ADC_BUF_SIZE);

			//DMA_Cmd(DMA1_Channel1, ENABLE);
			DMA1_Channel1->CCR |= DMA_CCR1_EN;

			//adc=(1.42 - adc*3.3/4096)*1000/4.35 + 25;
			//已兼顾速度与精度
			adc = 142000 - ((adc*330*1000) >> 12);
			adc = adc*100/435+2500;

			if ( (OSTime/1000)%10 == 0 ) {
				prompt("T: %d.%02d C\t",adc/100,adc%100);
				RTC_show_time();
			}
		}
				 
		/* Insert delay	*/
		//OSTimeDlyHMSM(0, 0,	1, 0);
		OSTimeDlyHMSM(0, 0,	0, 800);
		//printf("L%010d\r\n",OSTime);
		led_toggle( OBD_UNKNOW ) ;
		//led_toggle( OBD_KWP ) ;

		if ( (OSTime/1000)%10 == 0 ) {//check every 10 sec
			for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
				if ( OSTime - gsm_sent_q[var_uchar].send_time > 60*60*1000 ) {
					gsm_sent_q[var_uchar].send_time= 0 ;//free queue if > 1 hours
					gsm_sent_q[var_uchar].send_pcb = 0 ;
				}
			}
		}//end of check every 10 sec
	}
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

static void calc_sn( )
{
	static unsigned char chkbyte ;

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7E8));
	snprintf((char *)&pro_sn[4],3,"%02X",chkbyte);

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7EC));
	snprintf((char *)&pro_sn[6],3,"%02X",chkbyte);

	chkbyte = mcu_id_eor(*(vu32*)(0x1FFFF7F0));
	snprintf((char *)&pro_sn[8],3,"%02X",chkbyte);

	prompt("The MCU ID is %X %X %X\tSN:%s\r\n",\
		*(vu32*)(0x1FFFF7E8),*(vu32*)(0x1FFFF7EC),*(vu32*)(0x1FFFF7F0),pro_sn);
}

//return 0: ok
//return 1: no send queue
//return 2: no free buffer or buffer busy
static unsigned char gsm_send_time( struct SENT_QUEUE *queue_p, unsigned char *sequence)
{
	static unsigned char i, chkbyte, index;
#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	prompt("Need update RTC\t%d, tx_len= %d\r\n",\
		(RTC_GetCounter( ) - stm32_rtc.update_time),mg323_cmd.tx_len);

	stm32_rtc.update_time = RTC_GetCounter( ) ;

	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < MAX_CMD_QUEUE ; index++) {

		if ( queue_p[index].send_pcb == 0 ) { //no use

			//prompt("Use queue: %02d\r\n",index);

			//HEAD SEQ CMD Length(2 bytes) SN(char 10) check
			if ( !mg323_cmd.lock \
				&& mg323_cmd.tx_len < (GSM_BUF_LENGTH-20) ) {
				//no process occupy && have enough buffer
	
				OS_ENTER_CRITICAL();
				mg323_cmd.lock = true ;
				OS_EXIT_CRITICAL();
	
				//set protocol string value
				queue_p[index].send_time= OSTime ;
				queue_p[index].send_seq = *sequence ;
				queue_p[index].send_pcb = GSM_CMD_TIME ;
	
				//prepare GSM command
				mg323_cmd.tx[mg323_cmd.tx_len]   = GSM_HEAD ;
				mg323_cmd.tx[mg323_cmd.tx_len+1] = *sequence ;//SEQ
				*(sequence++);
				mg323_cmd.tx[mg323_cmd.tx_len+2] = GSM_CMD_TIME ;//PCB
				mg323_cmd.tx[mg323_cmd.tx_len+3] = 0  ;//length high
				mg323_cmd.tx[mg323_cmd.tx_len+4] = 10 ;//length low
				strncpy((char *)&mg323_cmd.tx[mg323_cmd.tx_len+5], (char *)pro_sn, 10);
	
				//prompt("GSM CMD: %02X ",mg323_cmd.tx[mg323_cmd.tx_len]);
				chkbyte = GSM_HEAD ;
				for ( i = 1 ; i < 15 ; i++ ) {//calc chkbyte
					chkbyte ^= mg323_cmd.tx[mg323_cmd.tx_len+i];
					//printf("%02X ",mg323_cmd.tx[mg323_cmd.tx_len+i]);
				}
				mg323_cmd.tx[mg323_cmd.tx_len+15] = chkbyte ;
				//printf("%02X\r\n",mg323_cmd.tx[mg323_cmd.tx_len+15]);
				//update buf length
				mg323_cmd.tx_len = mg323_cmd.tx_len + 16 ;
	
				OS_ENTER_CRITICAL();
				mg323_cmd.lock = false ;
				OS_EXIT_CRITICAL();

				return 0 ;
			}//end of if ( !mg323_cmd.lock && mg323_cmd.tx_len < (GSM_BUF_LENGTH-20))
			else {//no buffer
				prompt("No free buffer or buffer busy! check %s:%d\r\n",__FILE__, __LINE__);	
				return 2;
			}
		}//end of queue_p[index].send_pcb == 0
	}

	prompt("No free queue! check %s:%d\r\n",__FILE__, __LINE__);
	return 1;
}

//return 0: ok
//return 1: failure
static unsigned char gsm_rx_decode( struct GSM_RX *buf ,struct SENT_QUEUE *queue_p)
{
	unsigned char chkbyte, queue_index=0;
	unsigned int  chk_index;
	unsigned char  len_high=0, len_low=0;
#if OS_CRITICAL_METHOD == 3   /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	//status transfer: S_HEAD -> S_PCB -> S_CHK ==> S_HEAD...
	//note:从S_PCB开始计时queue_time,如果>5*AT_TIMEOUT,则重置状态为S_HEAD
	//1, find HEAD
	if ( buf->status == S_HEAD ) {
		while ( !mg323_cmd.rx_empty && buf->status == S_HEAD) {

			if ( *mg323_cmd.rx_out_last == GSM_HEAD ) {//found
				buf->start = mg323_cmd.rx_out_last ;//mark the start
				buf->status = S_PCB ;//search protocol control byte
				buf->time = OSTime ;
				//prompt("Found HEAD: %X\r\n",buf->start);
			}
			else { //no found
				mg323_cmd.rx_out_last++;//search next byte
				if (mg323_cmd.rx_out_last==mg323_cmd.rx+GSM_BUF_LENGTH) {
					mg323_cmd.rx_out_last = mg323_cmd.rx; //地址到顶部,回到底部
				}

				OS_ENTER_CRITICAL();
				mg323_cmd.rx_full = false; //reset the full flag
				if (mg323_cmd.rx_out_last==mg323_cmd.rx_in_last) {
					mg323_cmd.rx_empty = true ;//set the empty flag
				}
				OS_EXIT_CRITICAL();

			}
			//printf(">%02X  ",*mg323_cmd.rx_out_last);
		}
	}

	//2, find PCB&SEQ&LEN
	if ( buf->status == S_PCB ) {

		if ( OSTime - buf->time > 5*AT_TIMEOUT ) {//reset status
			prompt("In S_PCB timeout, reset to S_HEAD status!!!\r\n");
			buf->status = S_HEAD ;
			mg323_cmd.rx_out_last++	 ;
		}

		if ( ((mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) \
			&&  (mg323_cmd.rx_in_last - mg323_cmd.rx_out_last > 5))\
			|| ((mg323_cmd.rx_out_last > mg323_cmd.rx_in_last) \
			&& (GSM_BUF_LENGTH - \
			(mg323_cmd.rx_out_last - mg323_cmd.rx_in_last) > 5))) {
			//HEAD SEQ CMD Length(2 bytes) = 5 bytes

			//get PCB
			if ( (buf->start + 2)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				//PCB no in the end of buffer
				buf->pcb = *(buf->start + 2);
			}
			else { //PCB in the end of buffer
				buf->pcb = *(buf->start + 2 - GSM_BUF_LENGTH);
			}//end of (buf->start + 2)  < (mg323_cmd.rx+GSM_BUF_LENGTH)
			//printf("\r\nrespond PCB is %02X\t",buf->pcb);

			//get sequence
			if ( (buf->start + 1)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				//SEQ no in the end of buffer
				buf->seq = *(buf->start + 1);
			}
			else { //SEQ in the end of buffer
				buf->seq = *(buf->start + 1 - GSM_BUF_LENGTH);
			}//end of (buf->start + 1)  < (mg323_cmd.rx+GSM_BUF_LENGTH)
			//printf("SEQ is %02X\t",buf->seq);

			//get len high
			if ( (buf->start + 3)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				len_high = *(buf->start + 3);
			}
			else { //LEN in the end of buffer
				len_high = *(buf->start + 3 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - mg323_cmd.rx)
			//printf("respond LEN H: %02d\t",len_high);

			//get len low
			if ( (buf->start + 4)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				len_low = *(buf->start + 4);
			}
			else { //LEN in the end of buffer
				len_low = *(buf->start + 4 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - mg323_cmd.rx)
			//printf("L: %02d ",len_low);

			buf->len = len_high << 8 | len_low ;
			//printf("respond LEN: %d\r\n",buf->len);

			//update the status & timer
			buf->status = S_CHK ;//search check byte
			buf->time = OSTime ;
		}//end of (mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) ...
	}//end of if ( buf->status == S_PCB )

	//3, find CHK
	if ( buf->status == S_CHK ) {

		if ( OSTime - buf->time > 10*AT_TIMEOUT ) {//reset status
			prompt("In S_CHK timeout, reset to S_HEAD status!!!\r\n");
			buf->status = S_HEAD ;
			mg323_cmd.rx_out_last++	 ;
		}

		if ( ((mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) \
			&&  (mg323_cmd.rx_in_last - mg323_cmd.rx_out_last > (5+buf->len)))\
			|| ((mg323_cmd.rx_out_last > mg323_cmd.rx_in_last) \
			&& (GSM_BUF_LENGTH - \
			(mg323_cmd.rx_out_last - mg323_cmd.rx_in_last) > (5+buf->len)))) {
			//buffer > HEAD SEQ CMD Length(2 bytes) = 5 bytes + LEN + CHK(1)

			//get CHK
			if ( (buf->start + 5+buf->len)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				//CHK no in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len );
				//printf("CHK add: %X\t",buf->start + 5 + buf->len);
			}
			else { //CHK in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len - GSM_BUF_LENGTH);
				//printf("CHK add: %X\t",buf->start + 5 + buf->len - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - mg323_cmd.rx)
			//2012/1/4 19:54:50 已验证边界情况，正常

			chkbyte = GSM_HEAD ;
			for ( chk_index = 1 ; chk_index < buf->len+5 ; chk_index++ ) {//calc chkbyte
				if ( (buf->start+chk_index) < mg323_cmd.rx+GSM_BUF_LENGTH ) {
					chkbyte ^= *(buf->start+chk_index);
				}
				else {//data in begin of buffer
					chkbyte ^= *(buf->start+chk_index-GSM_BUF_LENGTH);
				}
			}

			if ( chkbyte == buf->chk ) {//data correct
				//find the sent record in gsm_sent_q by SEQ
				for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
					if ( queue_p[queue_index].send_seq == buf->seq \
						&& buf->pcb==(queue_p[queue_index].send_pcb | 0x80)) { 

						//prompt("queue_p %d is correct record.\r\n",queue_index);
						//found, release this record
						queue_p[queue_index].send_pcb = 0 ;
						break ;
					}//end of queue_p[queue_index].send_pcb == 0
				}

				//handle the respond
				switch (buf->pcb&0x7F) {

				case 0x4C://'L':
					break;

				case GSM_CMD_TIME://0x54,'T'
					//C9 08 D4 00 04 4F 0B CD E5 7D
					prompt("Time respond PCB: 0x%X @ %08X, %08X~%08X\r\n",\
						buf->pcb&0x7F,buf->start,\
						mg323_cmd.rx,mg323_cmd.rx+GSM_BUF_LENGTH);

					//Update and calibrate RTC
					RTC_update_calibrate(buf->start,mg323_cmd.rx) ;
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
				if ( (buf->start+chk_index) < mg323_cmd.rx+GSM_BUF_LENGTH ) {
					*(buf->start+chk_index) = 0x0;
				}
				else {//data in begin of buffer
					*(buf->start+chk_index-GSM_BUF_LENGTH) = 0x0;
				}
			}

			//update the buffer point
			if ( (buf->start + 6+buf->len)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
				mg323_cmd.rx_out_last = buf->start + 6 + buf->len ;
			}
			else { //CHK in the end of buffer
				mg323_cmd.rx_out_last = buf->start + 6 + buf->len - GSM_BUF_LENGTH ;
			}

			OS_ENTER_CRITICAL();
			mg323_cmd.rx_full = false; //reset the full flag
			if (mg323_cmd.rx_out_last==mg323_cmd.rx_in_last) {
				mg323_cmd.rx_empty = true ;//set the empty flag
			}
			OS_EXIT_CRITICAL();

			prompt("Next add: %X\t",mg323_cmd.rx_out_last);
			printf("In last: %X\t",mg323_cmd.rx_in_last);
			printf("rx_empty: %X\r\n",mg323_cmd.rx_empty);

			//update the next status
			buf->status = S_HEAD;

		}//end of (mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) ...
		else { //
			;//prompt("Buffer no enough.\r\n");
		}
	}//end of if ( buf->status == S_CHK )

	return 0 ;
}
