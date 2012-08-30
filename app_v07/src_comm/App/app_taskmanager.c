#include "main.h"

#define	BUILD_DATE "iCar v04, built at "__DATE__" "__TIME__

#define WATCHDOG	//enable independent watchdog

unsigned char BUILD_REV[] __attribute__ ((section ("FW_REV"))) ="$Rev$";//Don't modify this line

static	OS_STK		app_task_gsm_stk[APP_TASK_GSM_STK_SIZE];
static	OS_STK		app_task_obd_stk[APP_TASK_OBD_STK_SIZE];

OS_EVENT 	*sem_obd	;//for develop CAN

static void calc_sn( void );
static void show_sys_info( void );
static void conv_rev( unsigned char * );
static void flash_led( unsigned int );
static unsigned int calc_free_buffer(unsigned char *,unsigned char *,unsigned int);
static unsigned char mcu_id_eor( unsigned int );
static unsigned char gsm_send_sn( unsigned char *, unsigned char );
static unsigned char gsm_send_pcb( unsigned char *, unsigned char, unsigned int *, unsigned char);//protocol control byte
static unsigned char gsm_rx_decode( struct GSM_RX_RESPOND *);
static void console_cmd( unsigned char, unsigned char * );

struct CAR2SERVER_COMMUNICATION c2s_data ;// tx 缓冲处理待改进

extern struct ICAR_DEVICE my_icar;

//Must same as in main.h define
const unsigned char commands[] = {\
GSM_ASK_IST,\
GSM_CMD_ERROR,\
GSM_CMD_RECORD,\
GSM_CMD_SN,\
GSM_CMD_TIME,\
GSM_CMD_UPGRADE,\
GSM_CMD_UPDATE,\
GSM_CMD_WARN\
};

static unsigned char pro_sn[]="CAN_xxxxxx";
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

//warn code define, internal use only
#define	W_TEST				01	//

/*
*********************************************************************************************************
*										   App_TaskManage()
*
* Description :	The	startup	task.  The uC/OS-II	ticker should only be initialize once multitasking starts.
* Note(s)	  :	This is the firmware core, manage all the subtask
*********************************************************************************************************
*/

void  app_task_manager (void *p_arg)
{

	CPU_INT08U	os_err;
	unsigned char var_uchar , ist_cnt, gsm_sequence=0, rst_flag=0;
	unsigned int record_sequence = 0;
	struct GSM_RX_RESPOND mg323_rx_cmd;

	u16 adc;

	(void)p_arg;

	mg323_rx_cmd.timer = OSTime ;
	mg323_rx_cmd.start = c2s_data.rx;//prevent access unknow address
	mg323_rx_cmd.status = S_HEAD;

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
	c2s_data.next_ist = 0 ;

	/* Initialize my_icar */
	my_icar.debug = 0 ;
	my_icar.login_timer = 0 ;//will be update in RTC_update_calibrate

	my_icar.need_sn = 3 ;
	my_icar.mg323.ask_power = true ;
	//my_icar.mg323.ask_power = false ;//for develop CAN only
	my_icar.upgrade.err_no = 0 ;
	my_icar.upgrade.prog_fail_addr = 0 ;

	my_icar.upgrade.new_fw_ready = false ;

	my_icar.obd.can_tx_cnt = 0 ;

	/* Initialize the err msg	*/
	for ( var_uchar = 0 ; var_uchar < MAX_ERR_MSG ; var_uchar++) {
		my_icar.err_q_idx[var_uchar] = MAX_CMD_QUEUE + 1 ;
		//prompt("ERR @ %08X\r\n",&my_icar.err_q_idx[var_uchar]);
	}

	/* Initialize the warn msg	*/
	for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {

		//my_icar.warn[var_uchar].msg = (F_APP_TASKMANAGER) << 24 ;
		//my_icar.warn[var_uchar].msg |= var_uchar << 16 ;
		//my_icar.warn[var_uchar].msg |= __LINE__ ;
		my_icar.warn[var_uchar].msg = 0 ;
		my_icar.warn[var_uchar].queue_idx = MAX_CMD_QUEUE + 1 ;
		//prompt("MSG %08X @ %08X\r\n",my_icar.warn[var_uchar].msg,&my_icar.warn[var_uchar].msg);
	}

	//闪存容量寄存器基地址：0x1FFF F7E0
	//以K字节为单位指示产品中闪存存储器容量。
	//例：0x0080 = 128 K字节
	if ( *(vu16*)(0x1FFFF7E0) >= 256 ) {
		my_icar.upgrade.page_size = 0x800; //2KB
		my_icar.upgrade.base = 0x08000000 + \
			((*(vu16*)(0x1FFFF7E0))>>2)*my_icar.upgrade.page_size;
	}
	else { 
		my_icar.upgrade.page_size = 0x400; //1KB
		my_icar.upgrade.base = 0x08010C00 ;	//Page67
	}

	conv_rev((unsigned char *)BUILD_REV);
	my_icar.hw_rev = 0;//will be set by resistor
	calc_sn( );//prepare serial number

	//enable temperature adc DMA
	//DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA1_Channel1->CCR |= DMA_CCR1_EN;

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

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

	get_parameters( );//use (my_icar.upgrade.base - 2K) for para base address
	show_sys_info( ), show_rst_flag( ), show_err_log( );

	//flash LED, wait power stable and others task init
	flash_led( 80 );//100ms

	/* Create the GSM task.	*/
	os_err = OSTaskCreateExt((void (*)(void *)) app_task_gsm,
						   (void		  *	) 0,
						   (OS_STK		  *	)&app_task_gsm_stk[APP_TASK_GSM_STK_SIZE - 1],
						   (INT8U			) APP_TASK_GSM_PRIO,
						   (INT16U			) APP_TASK_GSM_PRIO,
						   (OS_STK		  *	)&app_task_gsm_stk[0],
						   (INT32U			) APP_TASK_GSM_STK_SIZE,
						   (void		  *	) 0,
						   (INT16U			)(OS_TASK_OPT_STK_CLR |	OS_TASK_OPT_STK_CHK));

#if	(OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_GSM_PRIO, (CPU_INT08U *)"gsm	task", &os_err);
#endif

	/* Create the OBD task. */
	os_err = OSTaskCreateExt((void (*)(void *)) app_task_obd,
						   (void		  *	) 0,
						   (OS_STK		  *	)&app_task_obd_stk[APP_TASK_OBD_STK_SIZE - 1],
						   (INT8U			) APP_TASK_OBD_PRIO,
						   (INT16U			) APP_TASK_OBD_PRIO,
						   (OS_STK		  *	)&app_task_obd_stk[0],
						   (INT32U			) APP_TASK_OBD_STK_SIZE,
						   (void		  *	) 0,
						   (INT16U			)(OS_TASK_OPT_STK_CLR |	OS_TASK_OPT_STK_CHK));

#if	(OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_GSM_PRIO, (CPU_INT08U *)"obd	task", &os_err);
#endif

	//Init SEMAPHORES
	sem_obd = OSSemCreate(0);

	//Suspend GSM task for develop CAN
	//os_err = OSTaskSuspend(APP_TASK_GSM_PRIO);

#if	(OS_TASK_STAT_EN > 0)
	OSStatInit();
#endif

	//independent watchdog init
	#ifdef WATCHDOG
		iwdg_init( );
	#endif

	while	(1)
	{
		/* Reload IWDG counter */
		#ifdef WATCHDOG
			IWDG_ReloadCounter();  
		#endif

		if ( my_icar.upgrade.new_fw_ready ) {
			// new fw ready
			// check others conditions, like: ECU off? ...
			// TBD

			//prompt("Reboot and upgrade new FW!\r\n");
			__set_FAULTMASK(1);	// 关闭所有中端
			NVIC_SystemReset();	// 复位
		}

		if ( c2s_data.tx_len > 0 || !my_icar.login_timer ) {//have command, need online
			my_icar.mg323.ask_online = true ;
		}
		else {//consider in the future
			;//my_icar.mg323.ask_online = false ;
		}

		//Send command

		//high task
		if ( my_icar.need_sn && c2s_data.tx_sn_len == 0 ) {//not in sending process
			gsm_send_sn( &gsm_sequence, MAX_CMD_QUEUE-1 );//will be return time also
		}

		if ( my_icar.login_timer ) {//send others CMD after login

			//middle task
			//由于其它原因，此时连接有可能中断，但也必须把错误日志存放到发送队列里
			for ( var_uchar = MAX_ERR_MSG ; var_uchar > 0  ; var_uchar--) {
				//高位：MCU复位原因，优先级最高
				//if ( BKP_ReadBackupRegister(BKP_DR1) ) {//have err msg


				if ( ((BKP_ReadBackupRegister(BKP_DR1))>>(var_uchar-1)*4)&0x0F ) {//have err msg

					if ( (my_icar.err_q_idx[var_uchar-1] == MAX_CMD_QUEUE + 1) ||\
						(c2s_data.queue_sent[my_icar.err_q_idx[var_uchar-1]].send_pcb != GSM_CMD_ERROR)){
						//not init value or not in send queue

						//send it
						if ( !gsm_send_pcb(&gsm_sequence, GSM_CMD_ERROR, (unsigned int *)&my_icar.err_q_idx[var_uchar-1], MAX_CMD_QUEUE-2)){
							//prompt("Upload err log and reset send_timer...\r\n");
							//my_icar.err_log_send_timer = OSTime ;
							c2s_data.tx_timer = 0 ;//need send ASAP
						}
					}
				}
			}

			if ( my_icar.mg323.tcp_online ) {//Send command when online

				//upload warn msg, 每次从my_icar.warn_msg[0]开始发
				for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {

					if ( my_icar.warn[var_uchar].msg ) { //have msg
						if ( (my_icar.warn[var_uchar].queue_idx == MAX_CMD_QUEUE + 1) ||\
							(c2s_data.queue_sent[my_icar.warn[var_uchar].queue_idx].send_pcb != GSM_CMD_WARN)){
							//not init value or not in send queue

							//send it
							//prompt("Warn MSG is: %08X\r\n",my_icar.warn[var_uchar].msg);
							if ( !gsm_send_pcb(&gsm_sequence, GSM_CMD_WARN,&my_icar.warn[var_uchar].msg,MAX_CMD_QUEUE-3)){
								//var_uchar = MAX_WARN_MSG ;//每次只发一个
							}
						}
					}
				}
	
				if ( (RTC_GetCounter( ) - my_icar.stm32_rtc.update_timer) > RTC_UPDATE_PERIOD ) {
					//need update RTC by server time, will be return time from server
					gsm_send_pcb(&gsm_sequence, GSM_CMD_TIME,&record_sequence,MAX_CMD_QUEUE-4);
				}

				//low task 

				if ( (OSTime/100)%3 == 0 ) { //run every 3 sec
					if ( c2s_data.next_ist ) {
						//Got new instruction
						gsm_send_pcb(&gsm_sequence, c2s_data.next_ist, &record_sequence,MAX_CMD_QUEUE-4);
						c2s_data.next_ist = 0 ;//reset if run
					}
					else {//No new instruction from server
						ist_cnt = 0 ;
						//Check the IST runing or no
						for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
							if ( GSM_ASK_IST==(c2s_data.queue_sent[var_uchar].send_pcb)) { 
	
								ist_cnt++ ;
								if ( ist_cnt > 2 ) { //no need send more
									var_uchar = MAX_CMD_QUEUE+1 ;//end loop
								}
							}
						}
	
						//Ask instruction from server, return 0 if no instruction
						if ( ist_cnt <= 2 ) {
							gsm_send_pcb(&gsm_sequence, GSM_ASK_IST, &record_sequence,MAX_CMD_QUEUE-4);
						}
		
						//lowest task, just send every 15 sec
						if ( (OSTime/100)%15 == 0 ) {
							gsm_send_pcb(&gsm_sequence, GSM_CMD_RECORD, &record_sequence,MAX_CMD_QUEUE-5);
						}
					}
				}
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
			console_cmd( var_uchar, &rst_flag );
		}

		if( my_icar.stm32_adc.completed ) {//temperature convert complete
			my_icar.stm32_adc.completed = false;

			//滤波,只要数据的中间一段
			adc=digit_filter(my_icar.stm32_adc.converted,ADC_BUF_SIZE);

			//DMA_Cmd(DMA1_Channel1, ENABLE);
			DMA1_Channel1->CCR |= DMA_CCR1_EN;

			//adc=(1.42 - adc*3.3/4096)*1000/4.35 + 25;
			//已兼顾速度与精度
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
					prompt("queue %d, CMD: %c timeout, reset!\r\n",\
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

static void conv_rev( unsigned char *p )
{//$Rev$
	unsigned char i , j;

	my_icar.fw_rev = 0 ;
	i = 0 , p = p + 6 ;
	while ( *(p+i) != 0x20 ) {
		i++ ;
		if ( *(p+i) == 0x24 || i > 4 ) break ; //$
	}

	j = 0 ;
	while ( i ) {
		i-- ;
		my_icar.fw_rev = (*(p+i)-0x30)*pow(j) + my_icar.fw_rev;
		j++;
	}
}

static void calc_sn( )
{
	unsigned char sn_eor ;

	sn_eor = mcu_id_eor(*(vu32*)(0x1FFFF7E8));
	snprintf((char *)&pro_sn[4],3,"%02X",sn_eor);

	sn_eor = mcu_id_eor(*(vu32*)(0x1FFFF7EC));
	snprintf((char *)&pro_sn[6],3,"%02X",sn_eor);

	sn_eor = mcu_id_eor(*(vu32*)(0x1FFFF7F0));
	snprintf((char *)&pro_sn[8],3,"%02X",sn_eor);

	my_icar.sn = pro_sn ;
}

static void show_sys_info( void )
{
	printf("\r\n"),	prompt("%s\r\n",BUILD_DATE);
	prompt("Revision: %d  OS Tick: %d\r\n",my_icar.fw_rev,OS_TICKS_PER_SEC);

    //prompt("CPU Speed: %ld MHz  \r\n",BSP_CPU_ClkFreq() / 1000000L);
    prompt("Ticks: %ld  \r\n",OSTime);
    prompt("CtxSw: %ld\r\n\r\n",OSCtxSwCtr);

	prompt("The MCU ID is %X %X %X  SN:%s\r\n",\
		*(vu32*)(0x1FFFF7E8),*(vu32*)(0x1FFFF7EC),*(vu32*)(0x1FFFF7F0),my_icar.sn);

	prompt("Flash: %dKB, Page: %dKB, Parameter: %08X, FW star: %08X\r\n",\
			*(vu16*)(0x1FFFF7E0),(my_icar.upgrade.page_size)>>10,\
			my_icar.upgrade.base - 0x800,\
			my_icar.upgrade.base);

	prompt("Para rev: %d, items: %d\r\n",\
				my_icar.para.rev,(sizeof(my_icar.para)/4)-1);

}

static unsigned char mcu_id_eor( unsigned int id)
{
	unsigned char id_eor ;

	//Calc. MCU ID eor result as product SN
	id_eor =  (id >> 24)&0xFF ;
	id_eor ^= (id >> 16)&0xFF ;
	id_eor ^= (id >> 8)&0xFF ;
	id_eor ^= id&0xFF ;

	return id_eor ;
}

static void flash_led( unsigned int i )
{
	led_off(POWER_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(ALARM_LED);
	OSTimeDlyHMSM(0, 0,	0, i);

	led_on(ONLINE_LED);
	OSTimeDlyHMSM(0, 0,	0, i);
	led_off(ONLINE_LED);
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
	unsigned char chkbyte, queue_index=0, var_u8=0;
	unsigned int  chk_index, var_u32=0;

#if OS_CRITICAL_METHOD == 3   /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	//status transfer: S_HEAD -> S_PCB -> S_CHK ==> S_HEAD...
	//note:从S_PCB开始计时queue_time,如果>5*AT_TIMEOUT,则重置状态为S_HEAD
	//1, find HEAD
	if ( buf->status == S_HEAD ) {
		while ( !c2s_data.rx_empty && buf->status == S_HEAD) {

			if ( *c2s_data.rx_out_last == GSM_HEAD ) {//found
				buf->start = c2s_data.rx_out_last ;//mark the start
				buf->status = S_PCB ;//search protocol control byte
				buf->timer = OSTime ;
				if ( my_icar.debug > 2) {
					prompt("Buf: %X ~ %X\r\n",c2s_data.rx,c2s_data.rx+GSM_BUF_LENGTH);
					prompt("Found HEAD @ %X\t",buf->start);
				}
			}
			else { //no found
				c2s_data.rx_out_last++;//search next byte
				if (c2s_data.rx_out_last==c2s_data.rx+GSM_BUF_LENGTH) {
					c2s_data.rx_out_last = c2s_data.rx; //地址到顶部,回到底部
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

		var_u32 = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		if ( var_u32 > 5 ) {
			//HEAD SEQ CMD Length(2 bytes) = 5 bytes

			//get PCB
			if ( (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//PCB no in the end of buffer
				buf->pcb = *(buf->start + 2);
			}
			else { //PCB in the end of buffer
				buf->pcb = *(buf->start + 2 - GSM_BUF_LENGTH);
			}//end of (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH)

			if ( my_icar.debug > 1) {
				printf("PCB is %c\t",(buf->pcb)&0x7F);
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
				var_u8 = *(buf->start + 3);
			}
			else { //LEN in the end of buffer
				var_u8 = *(buf->start + 3 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//printf("respond LEN H: %02d\t",var_u8);

			//get len low
			if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				buf->len = var_u8 << 8 | *(buf->start + 4) ;
			}
			else { //LEN in the end of buffer
				buf->len = var_u8 << 8 | *(buf->start + 4 - GSM_BUF_LENGTH) ;
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)

			if ( my_icar.debug > 1) {
				printf("Len: %d\t",buf->len);
			}

			//update the status & timer
			buf->status = S_CHK ;//search check byte
			buf->timer = OSTime ;
		}//end of (c2s_data.rx_in_last > c2s_data.rx_out_last) ...
	}//end of if ( buf->status == S_PCB )

	//3, find CHK
	if ( buf->status == S_CHK ) {

		if ( OSTime - buf->timer > 30*AT_TIMEOUT ) {//reset status
			prompt("In S_CHK timeout, reset to S_HEAD status!!! Got %d Bytes\r\n",\
					c2s_data.rx_out_last-buf->start);
			buf->status = S_HEAD ;
			c2s_data.rx_out_last++	 ;
		}

		var_u32 = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		if ( var_u32 > (5+buf->len) ) {
			//buffer > HEAD SEQ CMD Length(2 bytes) = 5 bytes + LEN + CHK(1)

			//get CHK
			if ( (buf->start + 5+buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//CHK no in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len );
				if ( my_icar.debug > 1) {
					printf("CHK: %X @ %X\r\n",buf->chk,buf->start + 5 + buf->len);
				}
			}
			else { //CHK in the end of buffer
				buf->chk = *(buf->start + 5 + buf->len - GSM_BUF_LENGTH);
				if ( my_icar.debug > 1) {
					printf("CHK: %X @ %X\r\n",buf->chk,buf->start + 5 + buf->len - GSM_BUF_LENGTH);
				}
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//2012/1/4 19:54:50 已验证边界情况，正常

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
							printf("queue: %d seq: %d match, free CMD %c\r\n",queue_index,\
								buf->seq, c2s_data.queue_sent[queue_index].send_pcb);
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
				if ( my_icar.debug > 2 ) {
					prompt("Rec %c CMD return, Len: %d\r\n",buf->pcb&0x7F, buf->len);
				}
				switch (buf->pcb&0x7F) {

				case GSM_ASK_IST://0x3F,'?'
					//C9 CD BF 00 01 xx E8, xx is new instruction
					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {
					case 0x0://no new instruction
						if ( my_icar.debug ) {
							prompt("No new instruction! CMD_seq: %02X\r\n",\
								*((buf->start)+1));
							c2s_data.next_ist = 0 ;
						}
						break;

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("Upload SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));
						break;

					default://get a new instruction

						for ( chkbyte = 0 ; chkbyte < sizeof(commands) ; chkbyte++ ) {
							if ( commands[chkbyte] == *((buf->start)+5) ) {//legal instruction

								//Check this IST runing or no
								for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
									if ( *((buf->start)+5)==(c2s_data.queue_sent[queue_index].send_pcb)) { 

										prompt("New instruction %c in Q:%d\r\n",*((buf->start)+5),queue_index);
										c2s_data.next_ist = 0 ;//no need run again
										queue_index = MAX_CMD_QUEUE+1;chkbyte = 0xFD ;//found and end loop
									}
								}

								if ( queue_index == MAX_CMD_QUEUE ){//no found

									prompt("New instruction %c will be run\r\n",*((buf->start)+5));
									c2s_data.next_ist = *((buf->start)+5) ;
									chkbyte = 0xFD ; //end
								}
							}
						}

						if ( chkbyte == sizeof(commands) ) {
							prompt("Unknow instruction : 0x%02X CMD_seq: %02X ",\
									*((buf->start)+5),*((buf->start)+1));
							printf("check %s: %d\r\n",__FILE__,__LINE__);
						}
						break;
					}

					break;

				case 0x4C://'L':
					break;

				case GSM_CMD_ERROR://0x45,'E'  C9 3B C5 00 01 00 36
					//prompt("Record respond PCB @ %08X, %08X~%08X\r\n",\
						//buf->start,c2s_data.rx,c2s_data.rx+GSM_BUF_LENGTH);
					//my_icar.err_log_send_timer = 0 ;//can send others

					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

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
					//						11~8:upgrade fw failure code
					//						7~4:GPRS disconnect reason
					//						3~0:RSV
					case 0x10://record err log part 1: GSM module poweroff success
						prompt("Upload GSM module poweroff err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						prompt("!!! Update server program, the GSM power off had cancle!!! %d\r\n",__LINE__);
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

						//reset err_q_idx
						my_icar.err_q_idx[1] = MAX_CMD_QUEUE + 1 ;
						break;

					case 0x30://record err log part 3: fw upgrade or para update
						prompt("Upload upgrade/update log success, CMD_seq: %02X\r\n",*((buf->start)+1));
						//Clear the error flag
						//BKP_DR6, upgrade fw time(UTC Time) high
						//BKP_DR7, upgrade fw time(UTC Time) low
					    BKP_WriteBackupRegister(BKP_DR6, 0);//time high
					    BKP_WriteBackupRegister(BKP_DR7, 0);//time low
						BKP_WriteBackupRegister(BKP_DR1, \
							((BKP_ReadBackupRegister(BKP_DR1))&0xF0FF));

						//reset err_q_idx
						my_icar.err_q_idx[2] = MAX_CMD_QUEUE + 1 ;
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

						//reset err_q_idx
						my_icar.err_q_idx[3] = MAX_CMD_QUEUE + 1 ;
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

					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

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

					if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 4);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 4 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

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

					if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 4);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 4 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

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

				case GSM_CMD_UPGRADE://0x55,'U'
					//C9 57 D5 00 xx yy data
					//xx: data len, 
					//yy: KB sequence, 00: data is latest firmware revision
					//                 01: 1st KB of FW, 02: 2nd KB, 03: 3rd KB

					if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 4);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 4 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

					case 0x1://need upload SN first, C9 77 D5 00 01 01 6B
						my_icar.need_sn = 3 ;
						prompt("Upgrade CMD failure, need product SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));

						break;

					default:
						if ( my_icar.debug > 2) {
							prompt("Upgrade CMD success, CMD_seq: %02X\r\n",*((buf->start)+1));
						}
						//Check each KB and save to flash
						my_icar.upgrade.err_no = flash_upgrade_rec(buf->start,c2s_data.rx) ;
						if ( my_icar.upgrade.err_no && \
								!((BKP_ReadBackupRegister(BKP_DR1))&0x0F00) ) {

							//BKP_DR1, ERR index: 	15~12:MCU reset 
							//						11~8:upgrade fw failure code
							//						7~4:GPRS disconnect reason
							//						3~0:GSM module poweroff reason
							var_u32 = (BKP_ReadBackupRegister(BKP_DR1))&0xF0FF;
							var_u32 = var_u32 | (my_icar.upgrade.err_no<<8) ;
						    BKP_WriteBackupRegister(BKP_DR1, var_u32&0xFFFF);

							//BKP_DR6, upgrade fw time(UTC Time) high
							//BKP_DR7, upgrade fw time(UTC Time) low
						    BKP_WriteBackupRegister(BKP_DR6, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
						    BKP_WriteBackupRegister(BKP_DR7, (RTC_GetCounter( ))&0xFFFF);//low

							prompt("Upgrade fw err: %d, check %s: %d\r\n",\
									my_icar.upgrade.err_no,__FILE__,__LINE__);
						}
						break;
					}
					break;

				case GSM_CMD_UPDATE://0x75,'u' update parameter
					//C9 57 D5 00 xx yy data

					if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 4);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 4 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

					case 0x1://need upload SN first, C9 77 D5 00 01 01 6B
						my_icar.need_sn = 3 ;
						break;

					default:
						prompt("GSM_CMD_UPDATE return %d\r\n",*((buf->start)+4));
						my_icar.update.err_no = para_update_rec(buf->start,c2s_data.rx) ;
						if ( my_icar.update.err_no && \
								!((BKP_ReadBackupRegister(BKP_DR1))&0x0F00) ) {

							//BKP_DR1, ERR index: 	15~12:MCU reset 
							//						11~8:upgrade fw or update para failure code
							//						7~4:GPRS disconnect reason
							//						3~0:GSM module poweroff reason
							var_u32 = (BKP_ReadBackupRegister(BKP_DR1))&0xF0FF;
							var_u32 = var_u32 | (my_icar.update.err_no<<8) ;
						    BKP_WriteBackupRegister(BKP_DR1, var_u32&0xFFFF);

							//BKP_DR6, upgrade fw time(UTC Time) high
							//BKP_DR7, upgrade fw time(UTC Time) low
						    BKP_WriteBackupRegister(BKP_DR6, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
						    BKP_WriteBackupRegister(BKP_DR7, (RTC_GetCounter( ))&0xFFFF);//low

							prompt("Upgrade fw err: %d, check %s: %d\r\n",\
									my_icar.update.err_no,__FILE__,__LINE__);
						}

						break;
					}
					break;

				case GSM_CMD_WARN://0x57,'W' warn msg report
					//C9 16 D7 00 01 00 01

					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {

					case 0x0://report warn msg success
						if ( my_icar.debug > 2) {
							prompt("Report warn msg success, CMD_seq: %02X\r\n",*((buf->start)+1));
						}

						if ( (buf->start + 6)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
							var_u8 = *(buf->start + 6);
						}
						else { //in the end of buffer
							var_u8 = *(buf->start + 6 - GSM_BUF_LENGTH);
						}

						if ( var_u8 < MAX_WARN_MSG ) {//correct
							my_icar.warn[var_u8].msg = 0;
							my_icar.warn[var_u8].queue_idx = MAX_CMD_QUEUE + 1;
						}
						//prompt("Report warn msg in %d\r\n",var_u8);
						break;

					case 0x1://need upload SN first
						my_icar.need_sn = 3 ;
						prompt("Report warn msg failure, need product SN first! CMD_seq: %02X\r\n",\
							*((buf->start)+1));

						break;

					default:
						prompt("Report warn msg failure, unknow error code: 0x%02X CMD_seq: %02X ",\
								*((buf->start)+5),*((buf->start)+1));
						printf("check %s: %d\r\n",__FILE__,__LINE__);
						break;
					}
					break;

				default:
					prompt("Unknow respond PCB: 0x%X %s:%d\r\n",buf->pcb&0x7F,__FILE__,__LINE__);

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
			if ( my_icar.debug > 2) {
				prompt("Buffer no enough. %d\r\n",__LINE__);
			}
		}
	}//end of if ( buf->status == S_CHK )

	return 0 ;
}

static unsigned int calc_free_buffer(unsigned char *in,unsigned char *out,unsigned int len)
{//调用前，请确保 buffer 不为空

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
static unsigned char gsm_send_pcb( unsigned char *sequence, unsigned char out_pcb,\
								 unsigned int *record_seq, unsigned char queue_cnt)
{
	unsigned char chkbyte, index;
	u16 i ;
	unsigned int seq ;

#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	i = 0 , seq = 0 ;
	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < queue_cnt ; index++) {

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

				if ( my_icar.debug > 2 ) {
					prompt("Snd %c CMD , seq:%02X, queue: %02d\r\n",\
							out_pcb,*sequence,index);
				}

				if ( out_pcb == GSM_ASK_IST ) {//Max. 6 Bytes

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
					//						11~8:upgrade fw failure code
					//						7~4:GPRS disconnect reason
					//						3~0:RSV
					//BKP_DR2,DR3 RSV
					//BKP_DR4, GPRS disconnect time(UTC Time) high
					//BKP_DR5, GPRS disconnect time(UTC Time) low
					//BKP_DR6, upgrade fw time(UTC Time) high
					//BKP_DR7, upgrade fw time(UTC Time) low
					//BKP_DR8, MCU reset time(UTC Time) high
					//BKP_DR9, MCU reset time(UTC Time) low

					c2s_data.tx[c2s_data.tx_len+3] = 0 ;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 6 ;//length low

					i = (unsigned char *)record_seq - &my_icar.err_q_idx[0];
					my_icar.err_q_idx[i] = index ;
					//prompt("Send ERR in Q: %d i=% d\r\n",index,i);

					switch ( i ) {
					case 0x0://GSM module poweroff err
						//GSM reason
						prompt("!!! Update firmware, the GSM power off had cancle!!! %d\r\n",__LINE__);

						//will be removed...
						seq = (BKP_ReadBackupRegister(BKP_DR1))&0x0F;
						//err_code, 2 byte
						c2s_data.tx[c2s_data.tx_len+9] = (seq>>8)&0xFF;
						c2s_data.tx[c2s_data.tx_len+10]= (seq)&0xFF;

						//GSM power off time, 4 bytes
						seq = BKP_ReadBackupRegister(BKP_DR2) ;
						c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
						seq = BKP_ReadBackupRegister(BKP_DR2) ;//Change to DR3
						c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low

						break;

					case 0x1://GPRS disconnect err
						//GPRS reason
						seq = (BKP_ReadBackupRegister(BKP_DR1))&0xF0;
						//err_code, 2 byte
						c2s_data.tx[c2s_data.tx_len+9] = (seq>>8)&0xFF;
						c2s_data.tx[c2s_data.tx_len+10]= (seq)&0xFF;

						//gprs disconnect time, 4 bytes
						seq = BKP_ReadBackupRegister(BKP_DR4) ;
						c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
						seq = BKP_ReadBackupRegister(BKP_DR5) ;
						c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low

						break;

					case 0x2: //upgrade fw msg
						//FW msg
						seq = (BKP_ReadBackupRegister(BKP_DR1))&0xF00;
						//err_code, 2 byte
						c2s_data.tx[c2s_data.tx_len+9] = (seq>>8)&0xFF;
						c2s_data.tx[c2s_data.tx_len+10]= (seq)&0xFF;

						seq = BKP_ReadBackupRegister(BKP_DR6) ;
						c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//rev high
						c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//rev high
						seq = BKP_ReadBackupRegister(BKP_DR7) ;
						c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//size high
						c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//size low

						break;

					case 0x3: //highest priority, send first
						//MCU reset reason
						seq = (BKP_ReadBackupRegister(BKP_DR1))&0xF000;
						//err_code, 2 byte
						c2s_data.tx[c2s_data.tx_len+9] = (seq>>8)&0xFF;
						c2s_data.tx[c2s_data.tx_len+10]= (seq)&0xFF;

						//MCU reset time, 4 bytes
						seq = BKP_ReadBackupRegister(BKP_DR8) ;
						c2s_data.tx[c2s_data.tx_len+5] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+6] = (seq)&0xFF  ;//time high
						seq = BKP_ReadBackupRegister(BKP_DR9) ;
						c2s_data.tx[c2s_data.tx_len+7] = (seq>>8)&0xFF  ;//time high
						c2s_data.tx[c2s_data.tx_len+8] =  seq&0xFF  ;//time low

						break;

					default://logic err, report to server
						//TBD, save to warn_msg
						break;
					}
	
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

				if ( out_pcb == GSM_CMD_UPGRADE ) {//Max. TBD Bytes

					if ( flash_upgrade_ask( &c2s_data.tx[c2s_data.tx_len] )){
						//failure, maybe error
						prompt("Flash upgrade CMD err, %s:%d\r\n",\
							__FILE__,__LINE__);
					}
					else { //ok
						if ( my_icar.debug > 3) {
							prompt("GSM CMD: %02X ",c2s_data.tx[c2s_data.tx_len]);
						}
						chkbyte = GSM_HEAD ;
						for ( i = 1 ; i < c2s_data.tx[c2s_data.tx_len+4]+5 ; i++ ) {//calc chkbyte
							chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
							if ( my_icar.debug > 2) {
								printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
							}
						}
						c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;
						if ( my_icar.debug > 2) {
							printf("%02X\r\n",c2s_data.tx[c2s_data.tx_len+i]);
						}

						//update buf length
						c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
					}
				}

				if ( out_pcb == GSM_CMD_UPDATE ) {//Max. 6 Bytes
					//HEAD SEQ PCB Length(2 bytes) HW rev + FW rev + xx(1 byte) check
					//xx:  parameter_revision

					c2s_data.tx[c2s_data.tx_len+3] = 0;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 4;//length low
					c2s_data.tx[c2s_data.tx_len+5] = my_icar.hw_rev;
					c2s_data.tx[c2s_data.tx_len+6] = (my_icar.fw_rev>>8)&0xFF;//fw rev. high
					c2s_data.tx[c2s_data.tx_len+7] = (my_icar.fw_rev)&0xFF;//fw rev. low
					c2s_data.tx[c2s_data.tx_len+8] = parameter_revision;

					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < c2s_data.tx[c2s_data.tx_len+4]+5 ; i++ ) {//calc chkbyte
						chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
					}
					c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;

					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
				}

				if ( out_pcb == GSM_CMD_WARN ) {//Max. 11 Bytes
					//HEAD SEQ PCB Length(2 bytes) DATA(4 Bytes) check
					//DATA: file(1 byte)+warn_code(1 byte)+line(2 B) + idx
					//warn_msg in: *record_seq

					i = (record_seq - &my_icar.warn[0].msg)>>1 ;//calc position
					//prompt("Warn MSG idx: %d, %08X - %08X = %d\r\n",i,\
						record_seq,&my_icar.warn[0].msg,record_seq-&my_icar.warn[0].msg);

					my_icar.warn[i].queue_idx = index ;//save index
					//prompt("Send msg %08X in Q: %d\r\n",*record_seq,index);
					c2s_data.tx[c2s_data.tx_len+3] = 0 ;//length high
					c2s_data.tx[c2s_data.tx_len+4] = 5 ;//length low

					c2s_data.tx[c2s_data.tx_len+5] = (*record_seq >> 24)&0xFF; //file name
					c2s_data.tx[c2s_data.tx_len+6] = (*record_seq >> 16)&0xFF; //warn code
					c2s_data.tx[c2s_data.tx_len+7] = (*record_seq >> 8)&0xFF;  //line, high
					c2s_data.tx[c2s_data.tx_len+8] = *record_seq & 0xFF;       //line, low
					c2s_data.tx[c2s_data.tx_len+9] = i;//idx

					//prompt("GSM CMD: %02X ",c2s_data.tx[c2s_data.tx_len]);
					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < c2s_data.tx[c2s_data.tx_len+4]+5 ; i++ ) {//calc chkbyte
						chkbyte ^= c2s_data.tx[c2s_data.tx_len+i];
						//printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					}
					c2s_data.tx[c2s_data.tx_len+i] = chkbyte ;
					//printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + i + 1 ;
				}

				OS_ENTER_CRITICAL();
				c2s_data.tx_lock = false ;
				OS_EXIT_CRITICAL();

				return 0 ;
			}//end of if ( !c2s_data.tx_lock && c2s_data.tx_len < (GSM_BUF_LENGTH-20))
			else {//no buffer or had been locked by other task
				if ( c2s_data.tx_lock ) {
					prompt("TCP tx buffer lock: %d, can't add CMD: %c ",\
							c2s_data.tx_lock,out_pcb);
				}
				else {
					prompt("TCP tx free buffer: %d ",GSM_BUF_LENGTH-c2s_data.tx_len);
				}
				printf("check %s:%d\r\n",__FILE__, __LINE__);
				return 2;
			}
		}//end of c2s_data.queue_sent[index].send_pcb == 0
		else { //show what's the PCB
			prompt("Q: %d is %c\r\n",index,c2s_data.queue_sent[index].send_pcb);
		}
	}

	prompt("No free queue: %d check %s:%d\r\n",index,__FILE__, __LINE__);
	return 1;
}

//return 0: ok
//return 1: no send queue
//return 2: no free buffer or buffer busy
static unsigned char gsm_send_sn( unsigned char *sequence, unsigned char queue_cnt)
{
	unsigned char i, chkbyte, index,ip_length;

	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < queue_cnt ; index++) {

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

void console_cmd( unsigned char cmd, unsigned char *flag )
{
	CPU_INT08U	os_err;

	prompt("%c\r\n",cmd);

	switch ( cmd ) {

	case 'b' :
		prompt("Print Backup register info ... \r\n");
		show_err_log( );
		break;

	case 'c' ://OBD CAN TX test
		prompt("CAN TX...\r\n");
		my_icar.obd.can_tx_cnt++;
		OSSemPost( sem_obd );
		break;

	case 'd' ://set debug flag
		my_icar.debug++ ;
		prompt("Increase debug lever, my_icar.debug:%d\r\n",my_icar.debug);
		break;

	case 'D' ://reset debug flag
		my_icar.debug = 0 ;
		prompt("Reset debug flag... my_icar.debug:%d\r\n",my_icar.debug);
		break;

	/*
	if ( var_uchar == 'f' ) {//show flash page67
		//show flash content
		prompt("Page:%d, %08X: %08X ",\
			(my_icar.upgrade.base+flash_idx*16-0x08000000)/my_icar.upgrade.page_size,\
			my_icar.upgrade.base+flash_idx*16,\
			*(vu32*)(my_icar.upgrade.base+flash_idx*16));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+flash_idx*16+4));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+flash_idx*16+8));
		printf("%08X \r\n",*(vu32*)(my_icar.upgrade.base+flash_idx*16+12));
		flash_idx=flash_idx++;
	}*/

	case 'g' ://Suspend GSM task for debug
		os_err = OSTaskSuspend(APP_TASK_GSM_PRIO);
		if ( os_err == OS_NO_ERR ) {
			prompt("Suspend GSM task ok.\r\n");
		}
		else {
			prompt("Suspend GSM task err: %d\r\n",os_err);
		}
		break;

	case 'G' ://Resume GSM task for debug
		//reset timer here, prevent reset GSM module
		my_icar.mg323.at_timer = OSTime ;

		os_err = OSTaskResume(APP_TASK_GSM_PRIO);
		if ( os_err == OS_NO_ERR ) {
			prompt("Resume GSM task ok.\r\n");
		}
		else {
			prompt("Resume GSM task err: %d\r\n",os_err);
		}
		break;

	case 'P' ://turn on/off GSM power
		prompt("Turn ON/OFF GSM Power\r\n");
		led_toggle( GSM_PM );
		break;

	case 'R' ://reset mcu
		*flag = 1;
		prompt("Are you sure reset MCU? Y/N \r\n");
		break;

	case 'v' ://show revision
		show_sys_info( );
		prompt("$URL$\r\n");
		prompt("$Id$\r\n");
		break;

	case 'W' ://send a warn msg for verify this function

		break;

	case 'Y' ://sure to reset mcu
		if ( *flag ) {
			prompt("*** System will reset! ***");
			my_icar.upgrade.new_fw_ready = true ;//will be rst by this flag
		}
		break;

	default:
		prompt("Unknow CMD %c, current support: b,c,d,D,g,G,P,R,v\r\n",cmd);
		*flag = 0 ;
		break;
	}
}
