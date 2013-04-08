#include "main.h"

#define	BUILD_DATE "iCar v04, built at "__DATE__" "__TIME__

#define WATCHDOG	//enable independent watchdog

unsigned char BUILD_REV[] __attribute__ ((section ("FW_REV"))) ="$Rev$";//Don't modify this line

static	OS_STK		app_task_gsm_stk[APP_TASK_GSM_STK_SIZE];
static	OS_STK		app_task_obd_stk[APP_TASK_OBD_STK_SIZE];

OS_EVENT 	*sem_obd_task	;//for develop CAN
OS_EVENT 	*sem_obd_prot	;//for develop CAN

static void calc_sn( void );
static void show_sys_info( void );
static void conv_rev( unsigned char * );
static void flash_led( unsigned int );
static unsigned int calc_free_buffer(unsigned char *,unsigned char *,unsigned int);
static unsigned char mcu_id_eor( unsigned int );
static unsigned char gsm_send_login( unsigned char *, unsigned char );
static unsigned char gsm_send_pcb( unsigned char *, unsigned char, unsigned int *, unsigned char);//protocol control byte
static unsigned char gsm_rx_decode( struct GSM_RX_RESPOND *);
static void console_cmd( unsigned char, unsigned char *, unsigned char * );
//static unsigned char record_queue_update( unsigned char *buf, unsigned char *buf_start) ;

struct CAR2SERVER_COMMUNICATION c2s_data ;// tx 缓冲处理待改进

extern struct ICAR_DEVICE my_icar;

//Must same as in main.h define
const unsigned char commands[] = {\
GSM_CMD_ERROR,\
GSM_CMD_RECORD,\
GSM_CMD_LOGIN,\
GSM_CMD_TIME,\
GSM_CMD_UPGRADE,\
GSM_CMD_UPDATE,\
GSM_CMD_WARN\
};

static unsigned char pro_sn[]="DEMOxxxxxx";
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
#define	W_NO_REC_IDX		10	//No this record item

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
#if OS_CRITICAL_METHOD == 3   /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	unsigned char var_uchar, gsm_sequence=0, rst_flag=0, flash_idx = 1, increase_var=0;
	unsigned int record_sequence = 0;
	struct GSM_RX_RESPOND mg323_rx_cmd;

	unsigned short adc_temperature;

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

	/* Initialize my_icar */
	my_icar.debug = 0 ;
	my_icar.login_timer = 0 ;//will be update in RTC_update_calibrate

	my_icar.need_sn = 3 ;
	my_icar.mg323.ask_power = true ;
	//my_icar.mg323.ask_power = false ;//for develop CAN only
	my_icar.upgrade.err_no = 0 ;
	my_icar.upgrade.prog_fail_addr = 0 ;

	my_icar.upgrade.status = NO_UPGRADE ;

	my_icar.obd.can_tx_cnt = 0 ;

	/* Initialize the err msg	*/
	for ( var_uchar = 0 ; var_uchar < MAX_ERR_MSG ; var_uchar++) {
		my_icar.err_q_idx[var_uchar] = MAX_CMD_QUEUE + 1 ;
	}

	/* Initialize the warn msg	*/
	for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
		my_icar.warn[var_uchar].msg = 0 ;
		my_icar.warn[var_uchar].queue_idx = MAX_CMD_QUEUE + 1 ;
	}

	/* Initialize the record queue	*/
	my_icar.record_lock = false; my_icar.record_cnt = 0;
	for ( var_uchar = 0 ; var_uchar < MAX_RECORD_CNT ; var_uchar++) {
		my_icar.rec_new[var_uchar].time = 0 ;
		my_icar.rec_new[var_uchar].val  = 0 ;
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
	sem_obd_task = OSSemCreate(0);
	sem_obd_prot = OSSemCreate(0);

	//Suspend GSM task for develop CAN
	//os_err = OSTaskSuspend(APP_TASK_GSM_PRIO);

#if	(OS_TASK_STAT_EN > 0)
	OSStatInit();
#endif

	//independent watchdog init
	#ifdef WATCHDOG
		iwdg_init( );
	#endif

	//OSSemPost( sem_obd_task );//Ask obd task to start
	
	while	(1)
	{
		/* Reload IWDG counter */
		#ifdef WATCHDOG
			IWDG_ReloadCounter();  
		#endif

		if ( my_icar.upgrade.status == UPGRADED) {
			// new fw ready
			// check others conditions, like: ECU off? ...
			// TBD

			//prompt("Reboot and upgrade new FW!\r\n");
			__set_FAULTMASK(1);	// close all interrupt
			NVIC_SystemReset();	// reset
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
			gsm_send_login( &gsm_sequence, MAX_CMD_QUEUE-1 );//will be return time also
		}

		if ( my_icar.login_timer ) {//send others CMD after login

			//middle task
			//Connection maybe lose because of GPRS, but still need push err log to send buffer
			for ( var_uchar = MAX_ERR_MSG ; var_uchar > 0  ; var_uchar--) {
				//High 4 bit: MCU reset reason, highest priority
				//my_icar.err_q_idx[3,2,1,0] save the send seq, 3~0 map:
				//15~12:MCU reset, 11~8:upgrade fw failure code, 7~4:GPRS disconnect reason, 3~0:reserve
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

				//upload warn msg, each time start from my_icar.warn_msg[0]
				for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {

					if ( my_icar.warn[var_uchar].msg ) { //have msg
						if ( (my_icar.warn[var_uchar].queue_idx == MAX_CMD_QUEUE + 1) ||\
							(c2s_data.queue_sent[my_icar.warn[var_uchar].queue_idx].send_pcb != GSM_CMD_WARN)){
							//not init value or not in send queue

							//send it
							//prompt("Warn MSG is: %08X\r\n",my_icar.warn[var_uchar].msg);
							if ( !gsm_send_pcb(&gsm_sequence, GSM_CMD_WARN,&my_icar.warn[var_uchar].msg,MAX_CMD_QUEUE-3)){
								var_uchar = MAX_WARN_MSG ;//only send once, can be improved. TBD
							}
						}
					}
				}

				//respond server cmd
				for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
					if ( c2s_data.srv_cmd[var_uchar].pcb ) {

						//send respond to server
						if (gsm_send_pcb(&gsm_sequence, (c2s_data.srv_cmd[var_uchar].pcb)|0x80,\
								&record_sequence,var_uchar) == 0 ){//send respond success
						
							debug_ptc("Send respond: %c success!\r\n",c2s_data.srv_cmd[var_uchar].pcb);

							//Send this command to server, 需日后改进，直接在send respond 里处理
							if ( gsm_send_pcb(&gsm_sequence, c2s_data.srv_cmd[var_uchar].pcb,\
												&record_sequence,MAX_CMD_QUEUE-4) == 0 ){

								c2s_data.tx_timer = 0 ;//need send ASAP
								//free this recodrd
								c2s_data.srv_cmd[var_uchar].seq = 0 ;
								c2s_data.srv_cmd[var_uchar].pcb = 0 ;
							}
						}
					}
				}//end of ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++)
				
				//send record if have data
				if ( my_icar.record_cnt ) {
					gsm_send_pcb(&gsm_sequence, GSM_CMD_RECORD, &record_sequence,MAX_CMD_QUEUE-5);
				}

				//update time, use as heartbeat function for keep connection alive.
				if ( (RTC_GetCounter( ) - my_icar.stm32_rtc.update_timer) > RTC_UPDATE_PERIOD ) {
					//need update RTC by server time, will be return time from server
					;//gsm_send_pcb(&gsm_sequence, GSM_CMD_TIME,&record_sequence,MAX_CMD_QUEUE-4);
				}

				//low task 
				if ( my_icar.upgrade.status == UPGRADING ) { //upgrading

					//Check the GSM_CMD_UPGRADE runing or no
					for ( var_uchar = 0 ; var_uchar < MAX_CMD_QUEUE ; var_uchar++) {
						if ( GSM_CMD_UPGRADE==(c2s_data.queue_sent[var_uchar].send_pcb)) { 
							break ;//end loop
						}
					}
					
					if ( var_uchar == MAX_CMD_QUEUE ) {//can't find sent record
						gsm_send_pcb(&gsm_sequence, GSM_CMD_UPGRADE, &record_sequence,MAX_CMD_QUEUE-4);
					}
				}

				if ( (OSTime/100)%3 == 0 ) { //run every 3 sec
					;//TBD		

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
			console_cmd( var_uchar, &rst_flag, &flash_idx );
		}

		if ( my_icar.record_cnt ) {
			prompt("Total %d record\r\n",my_icar.record_cnt);

			for ( var_uchar = 0 ; var_uchar < my_icar.record_cnt ; var_uchar++) {
				//prompt("R[%02d] idx = %d\r\n",var_uchar,\
						(my_icar.rec_new[var_uchar].val)&0xFF);
			}
		}
		
		if( my_icar.stm32_adc.completed ) {//temperature convert complete
			my_icar.stm32_adc.completed = false;

			//滤波,只要数据的中间一段
			adc_temperature=digit_filter(my_icar.stm32_adc.converted,ADC_BUF_SIZE);

			//DMA_Cmd(DMA1_Channel1, ENABLE);
			DMA1_Channel1->CCR |= DMA_CCR1_EN;

			//adc_temperature=(1.42 - adc_temperature*3.3/4096)*1000/4.35 + 25;
			//已兼顾速度与精度
			adc_temperature = 142000 - ((adc_temperature*330*1000) >> 12);
			adc_temperature = adc_temperature*100/435+2500;

			if ( (my_icar.record_cnt < MAX_RECORD_CNT) && !my_icar.record_lock ) {//add adc_temperature to record

				OS_ENTER_CRITICAL();
				my_icar.record_lock = true;
				OS_EXIT_CRITICAL();

				my_icar.rec_new[my_icar.record_cnt].time = RTC_GetCounter( ) ;
				//my_icar.rec_new[my_icar.record_cnt].val  = REC_IDX_ADC_TEMP ;//low  8  bits: record item index
				//for test only
				my_icar.rec_new[my_icar.record_cnt].val  = increase_var ;//test data
				increase_var++;
				
				my_icar.rec_new[my_icar.record_cnt].val |= adc_temperature << 8 ;//high 24 bits: value, max: 0xFF FF FF = 16777216
				my_icar.record_cnt++;

				OS_ENTER_CRITICAL();
				my_icar.record_lock = false;
				OS_EXIT_CRITICAL();
			}				
				
			if ( (OSTime/100)%10 == 0 ) {
				prompt("IP: %s\tT: %d.%02d C\t",my_icar.mg323.ip_local,adc_temperature/100,adc_temperature%100);
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


		//For OBD CMD--Keep Link ,then can send data on time.
		if ( obd_read_canid_idx ) {//have can id index
		
			switch((OSTime/100)%40) //3s一次
			{
				case 0:
					my_icar.obd.cmd = READ_PID ;
					my_icar.obd.pid = OBD_PID_ENGINE_RPM;
					OSSemPost( sem_obd_task );
				break;
				case 1:
					my_icar.obd.cmd = READ_PID ;
					my_icar.obd.pid = OBD_PID_VECHIVE_SPEED;
					OSSemPost( sem_obd_task );
				break;
				case 2:
					my_icar.obd.cmd = READ_PID ;
					my_icar.obd.pid = OBD_PID_CLT;
					OSSemPost( sem_obd_task );
				break;
				case 3:
					my_icar.obd.cmd = READ_DTC ;
					OSSemPost( sem_obd_task );
				break;

			}
		}
		else { //no can id index
			if ( (OSTime/100)%15 == 0 ) {
				OSSemPost( sem_obd_task );//retry detect every 15s
			}
		}
		/* Insert delay, IWDG set to 2 second
		App_taskmanger: longest time, but highest priority 
		Othoers: shorter time, but lower priority */
		//if rpm is zero then systen will sleep.
		OSTimeDlyHMSM(0, 0,	1, 0);
		led_toggle( PWR_LED ) ;
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
	SG_LED_ON;
	OSTimeDlyHMSM(0, 0,	0, i);

	SG_LED_OFF;
	OSTimeDlyHMSM(0, 0,	0, i);

	SG_LED_OFF;
	OSTimeDlyHMSM(0, 0,	0, i);

	PWR_LED_ON;
	SG_LED_ON;
	OSTimeDlyHMSM(0, 0,	0, i);

	SG_LED_OFF;
	SG_LED_OFF;
	OSTimeDlyHMSM(0, 0,	0, i);

	PWR_LED_ON;
}

//return 0: ok
//return 1: failure
static unsigned char gsm_rx_decode( struct GSM_RX_RESPOND *buf )
{
	unsigned char queue_index=0, var_u8=0;
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

				//debug_ptc("Buf: %X ~ %X\r\n",c2s_data.rx,c2s_data.rx+GSM_BUF_LENGTH);
				//debug_ptc("Found HEAD @ %X\r\n",buf->start);
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

		//计算buf 里有多少字节
		var_u32 = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		//最短：Head SEQ CMD Len(2B) INFO CRC16 ==> 7 + data_len
		if ( var_u32 >= 7 ) {

			//get PCB
			if ( (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//PCB no in the end of buffer
				buf->pcb = *(buf->start + 2);
			}
			else { //PCB in the end of buffer
				buf->pcb = *(buf->start + 2 - GSM_BUF_LENGTH);
			}//end of (buf->start + 2)  < (c2s_data.rx+GSM_BUF_LENGTH)

			//get sequence
			if ( (buf->start + 1)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//SEQ no in the end of buffer
				buf->seq = *(buf->start + 1);
			}
			else { //SEQ in the end of buffer
				buf->seq = *(buf->start + 1 - GSM_BUF_LENGTH);
			}//end of (buf->start + 1)  < (c2s_data.rx+GSM_BUF_LENGTH)

			//get len high
			if ( (buf->start + 3)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				var_u8 = *(buf->start + 3);
			}
			else { //LEN in the end of buffer
				var_u8 = *(buf->start + 3 - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)

			//get len low
			if ( (buf->start + 4)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//LEN no in the end of buffer
				buf->len = var_u8 << 8 | *(buf->start + 4) ;
			}
			else { //LEN in the end of buffer
				buf->len = var_u8 << 8 | *(buf->start + 4 - GSM_BUF_LENGTH) ;
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)

			debug_ptc("Len: %d\r\n",buf->len);

			//update the status & timer
			buf->status = S_CHK ;//search check byte
			buf->timer = OSTime ;
		}//end of (c2s_data.rx_in_last > c2s_data.rx_out_last) ...
	}//end of if ( buf->status == S_PCB )

	//3, find CRC16
	if ( buf->status == S_CHK ) {

		if ( OSTime - buf->timer > 30*AT_TIMEOUT ) {//reset status
			prompt("In S_CHK timeout, reset to S_HEAD status!!! Got %d Bytes\r\n",\
					c2s_data.rx_out_last-buf->start);
			buf->status = S_HEAD ;
			c2s_data.rx_out_last++	 ;
		}

		//计算buf 里有多少字节
		var_u32 = calc_free_buffer(c2s_data.rx_in_last,c2s_data.rx_out_last,GSM_BUF_LENGTH);
		//debug_ptc("Free buf len: %d\r\n",var_u32);
		//最短：Head SEQ CMD Len(2B) INFO CRC16 ==> 7 + data_len
		if ( var_u32 >= (7 + buf->len) ) {
			//buffer > 最短

			//get CRC_H
			if ( (buf->start + 5 + buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//CRC_H no in the end of buffer
				var_u8 = *(buf->start + 5 + buf->len );
			}
			else { //CRC_H in the end of buffer
				var_u8 = *(buf->start + 5 + buf->len - GSM_BUF_LENGTH);
			}
			
			//get CRC_L
			if ( (buf->start + 6 + buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				//CRC_L no in the end of buffer
				buf->crc16 = *(buf->start + 6 + buf->len ) | (var_u8 << 8);
				//debug_ptc("CRC16: %X @ %X\r\n",buf->crc16,buf->start + 6 + buf->len);
			}
			else { //CRC_L in the end of buffer
				buf->crc16 = *(buf->start + 6 + buf->len - GSM_BUF_LENGTH) | (var_u8 << 8);
				//debug_ptc("CRC16: %X @ %X\r\n",buf->crc16,buf->start + 6 + buf->len - GSM_BUF_LENGTH);
			}//end of GSM_BUF_LENGTH - (buf->start - c2s_data.rx)
			//2012/1/4 19:54:50 已验证边界情况，正常

			//Calc CRC16:
			var_u32 = 0 ;
			for ( chk_index = 0 ; chk_index < buf->len + 5 ; chk_index++ ) {//calc chk_crc
				if ( (buf->start+chk_index) < c2s_data.rx+GSM_BUF_LENGTH ) {
					var_u32 = crc16tablefast(buf->start+chk_index , 1, var_u32);
				}
				else {//data in begin of buffer
					var_u32 = crc16tablefast(buf->start+chk_index-GSM_BUF_LENGTH , 1, var_u32);
				}
			}

			if ( (var_u32&0xFFFF) == buf->crc16 ) {//data correct

				//find the sent record in c2s_data.queue_sent by SEQ
				for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
					if ( c2s_data.queue_sent[queue_index].send_seq == buf->seq \
						&& buf->pcb==(c2s_data.queue_sent[queue_index].send_pcb | 0x80)) { 

						debug_ptc("queue: %d seq: %d match, free CMD %c\r\n",queue_index,\
								buf->seq, c2s_data.queue_sent[queue_index].send_pcb);

						//found, free this record
						c2s_data.queue_sent[queue_index].send_pcb = 0 ;
						if ( c2s_data.queue_count > 0 ) {
							c2s_data.queue_count--;
						}
						break ;
					}//end of c2s_data.queue_sent[queue_index].send_pcb == 0
				}

				//handle the respond
				debug_ptc("Rec %c CMD, Len: %d\r\n",buf->pcb&0x7F, buf->len);

				switch (buf->pcb&0x7F) {

				case GSM_CMD_ERROR://0x45,'E'  DE 05 C5 00 02 00 40 06 AC
					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) { //status
						case 0://CMD success
							debug_ptc("Get ERR log CMD respond, CMD_seq: %02X\r\n",\
									*((buf->start)+1));
									
							//check return error index
							if ( (buf->start + 6)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
								chk_index = *(buf->start + 6);
							}
							else { //in the end of buffer
								chk_index = *(buf->start + 6 - GSM_BUF_LENGTH);
							}
							switch (chk_index) {
								
							//BKP_DR1, ERR index: 	15~12:MCU reset 
							//						11~8:upgrade fw failure code
							//						7~4:GPRS disconnect reason
							//						3~0:RSV
							case 0x1://rsv		
								prompt("!!! Err, RSV item, check:%d !!!\r\n",__LINE__);
								break;

							case 0x2://record err log part 2: GPRS disconnect success
								debug_ptc("Upload GPRS disconnect err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
								//Clear the error flag
								//BKP_DR4, GPRS disconnect time(UTC Time) high
								//BKP_DR5, GPRS disconnect time(UTC Time) low
							    BKP_WriteBackupRegister(BKP_DR4, 0);//high
							    BKP_WriteBackupRegister(BKP_DR5, 0);//low
								BKP_WriteBackupRegister(BKP_DR1, \
									((BKP_ReadBackupRegister(BKP_DR1))&0xFF0F));
		
								//reset err_q_idx to default value
								my_icar.err_q_idx[1] = MAX_CMD_QUEUE + 1 ;
								break;

							case 0x3://record err log part 3: fw upgrade or para update
								debug_ptc("Upload upgrade/update log success, CMD_seq: %02X\r\n",*((buf->start)+1));
								//Clear the error flag
								//BKP_DR6, upgrade fw time(UTC Time) high
								//BKP_DR7, upgrade fw time(UTC Time) low
							    BKP_WriteBackupRegister(BKP_DR6, 0);//time high
							    BKP_WriteBackupRegister(BKP_DR7, 0);//time low
								BKP_WriteBackupRegister(BKP_DR1, \
									((BKP_ReadBackupRegister(BKP_DR1))&0xF0FF));
		
								//reset err_q_idx to default value
								my_icar.err_q_idx[2] = MAX_CMD_QUEUE + 1 ;
								break;

							case 0x4://record err log part 4: MCU reset
								prompt("Upload MCU reset err log success, CMD_seq: %02X\r\n",*((buf->start)+1));
								//Clear the error flag
								//BKP_DR8, MCU reset time(UTC Time) high
								//BKP_DR9, MCU reset time(UTC Time) low
							    BKP_WriteBackupRegister(BKP_DR8, 0);//high
							    BKP_WriteBackupRegister(BKP_DR9, 0);//low
								BKP_WriteBackupRegister(BKP_DR1, \
									((BKP_ReadBackupRegister(BKP_DR1))&0x0FFF));
		
								//reset err_q_idx to default value
								my_icar.err_q_idx[3] = MAX_CMD_QUEUE + 1 ;
								break;
							}
							
						break;

						case 0x1://need login first
							my_icar.need_sn = 3 ;
							prompt("Upload err log failure, need login first! CMD_seq: %02X\r\n",\
								*((buf->start)+1));
						break;

						default:
							prompt("Upload err log failure, unknow error code: 0x%02X CMD_seq: %02X %s:%d\r\n",\
									*((buf->start)+5),*((buf->start)+1),__FILE__,__LINE__);
						break;
					}
					break;

				case GSM_CMD_LOGIN://0x4C,'L'
					//Head SEQ CMD Len Status INFO CRC16
					//DE D8 CC 00 10 00 
					//50 DC 01 07  => Time
					//06 6B FF 48 56 48 67 49 87 10 12 37 => MCU ID
					//A3 7C =>CRC16

					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					if ( var_u8 ) { //Login failure
						my_icar.need_sn = 3 ;
						prompt("Login failure! return status: %02X\r\n",\
							*((buf->start)+5));
					}
					else { //CMD success
						my_icar.need_sn = 0 ;
						//Update and calibrate RTC
						prompt("Login ok!\r\n");
						RTC_update_calibrate(buf->start,c2s_data.rx) ;
						my_icar.need_sn = 0 ;
						c2s_data.tx_timer = 0 ;//need send others queue ASAP
					}

					break;

				case GSM_CMD_RECORD://0x52,'R'
					//DE 03 D2 00 01 00 BC 9F
					if ( buf->pcb >= 0x80 ) { //respond reply
						if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
							var_u8 = *(buf->start + 5);
						}
						else { //in the end of buffer
							var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
						}
	
						switch (var_u8) {
	
						case 0x0://record success
							debug_ptc("Record CMD ok, seq: %02X\r\n",*((buf->start)+1));

							//check return record index
							//DE 19 D2 00 04 00 0A 32 5A 53 5D

							break;
	
						case 0x1://need login first
							my_icar.need_sn = 3 ;
							debug_ptc("Record cmd failure, need login first! CMD_seq: %02X\r\n",\
									*((buf->start)+1));
							break;
							
						default:
							prompt("Record cmd failure, unknow err: 0x%02X CMD_seq: %02X %s:%d\r\n",\
									*((buf->start)+5),*((buf->start)+1),__FILE__,__LINE__);
							break;
						}
						break;
					}
					else { //buf->pcb < 0x80, 服务器主动发送
						//Increase rec.req according to server require
						//record_queue_update(buf->start,c2s_data.rx);

						//save this CMD to srv_cmd
						for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
							if ( c2s_data.srv_cmd[queue_index].pcb == 0) { //empty record

								c2s_data.srv_cmd[queue_index].pcb = buf->pcb ;
								c2s_data.srv_cmd[queue_index].seq = buf->seq;
								break ;
							}//end if ( c2s_data.srv_cmd[queue_index].pcb == 0) 
							debug_ptc("srv_cmd[%d]= %c\r\n",queue_index,c2s_data.srv_cmd[queue_index].pcb);
						}//end of queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++
					}
					break;

				case GSM_CMD_UPGRADE://0x55,'U'
					//Head SEQ CMD Len INFO CRC16
					//DE 01 D5 00 08 
					//00 00 00 DA => status, indicate, FW rev
					//A8 3C 15 FE => FW len, FW CRC16
					//1A 59  =>CRC16

					if ( buf->pcb >= 0x80 ) { //respond reply
						if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
							var_u8 = *(buf->start + 5);
						}
						else { //in the end of buffer
							var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
						}
	
						switch (var_u8) { //status
							case 0://CMD success
								debug_ptc("Get upgrade CMD respond, CMD_seq: %02X\r\n",\
									*((buf->start)+1));
								my_icar.upgrade.status = UPGRADING ;//upgrading
								//Check each KB and save to flash
								my_icar.upgrade.err_no = flash_upgrade_rec(buf->start,c2s_data.rx) ;

								break;
							
							case 0x01://need login first
								prompt("Upgrade CMD failure: no login\r\n");
								my_icar.upgrade.status = NO_UPGRADE ;//error, stop upgrade
								my_icar.need_sn = 3 ;
								break;
							
							case 0x04://no this block
								prompt("Upgrade CMD failure: no this block!\r\n");
								break;

							default:
								prompt("Upgrade CMD failure, status: %02X\r\n",var_u8);
								//my_icar.upgrade.status = NO_UPGRADE ;
								break;
						}
					}
					else { //buf->pcb < 0x80, 服务器主动发送

						//save this CMD to srv_cmd
						for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
							if ( c2s_data.srv_cmd[queue_index].pcb == 0) { //empty record

								c2s_data.srv_cmd[queue_index].pcb = buf->pcb ;
								c2s_data.srv_cmd[queue_index].seq = buf->seq;
								break ;
							}//end if ( c2s_data.srv_cmd[queue_index].pcb == 0) 
							debug_ptc("srv_cmd[%d]= %c\r\n",queue_index,c2s_data.srv_cmd[queue_index].pcb);
						}//end of queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++
					}						
					break;

				case GSM_CMD_WARN://0x57,'W' warn msg report
					//DE 03 D7 00 02 00 01 93 71

					if ( (buf->start + 5)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
						var_u8 = *(buf->start + 5);
					}
					else { //in the end of buffer
						var_u8 = *(buf->start + 5 - GSM_BUF_LENGTH);
					}

					switch (var_u8) {//return status

					case 0x0://report warn msg success
						debug_ptc("Report warn msg success, CMD_seq: %02X\r\n",*((buf->start)+1));

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
						break;

					case 0x1://need login first
						my_icar.need_sn = 3 ;
						prompt("Report warn msg failure, need login first! CMD_seq: %02X\r\n",\
								*((buf->start)+1));
						break;
						
					default:
						prompt("Warn msg failure, unknow err: 0x%02X CMD_seq: %02X %s:%d\r\n",\
								*((buf->start)+5),*((buf->start)+1),__FILE__,__LINE__);
						break;
					}
					break;

				default:
					prompt("Unknow respond PCB: %c(0x%X) %s:%d\r\n",\
							buf->pcb&0x7F,buf->pcb&0x7F,__FILE__,__LINE__);
					break;
				}//end of handle the respond

/* old switch
				switch (buf->pcb&0x7F) {

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

				default:
					prompt("Unknow respond PCB: 0x%X %s:%d\r\n",buf->pcb&0x7F,__FILE__,__LINE__);

					break;
				}//end of handle the respond
*/
			}//end of if ( crc16 == buf->chk )
			else {//data no correct
				prompt("Rec data CRC16 ERR! %s:%d\r\n",__FILE__,__LINE__);
				prompt("Data: ");
				for ( chk_index = 0 ; chk_index < buf->len + 5 ; chk_index++ ) {//calc chk_crc
					if ( (buf->start+chk_index) < c2s_data.rx+GSM_BUF_LENGTH ) {
						printf("%02X ",*(buf->start+chk_index));
					}
					else {//data in begin of buffer
						printf("%02X ",*(buf->start+chk_index-GSM_BUF_LENGTH));
					}
				}
				printf("\r\n");
				prompt("Calc. CRC: %04X\t",var_u32&0xFFFF);
				printf("respond CRC: %04X\r\n",buf->crc16);
			}

			//Clear buffer content
			//最短：Head SEQ CMD Len(2B) INFO CRC16
			for ( chk_index = 0 ; chk_index < buf->len+7 ; chk_index++ ) {
				if ( (buf->start+chk_index) < c2s_data.rx+GSM_BUF_LENGTH ) {
					*(buf->start+chk_index) = 0x0;
				}
				else {//data in begin of buffer
					*(buf->start+chk_index-GSM_BUF_LENGTH) = 0x0;
				}
			}

			//update the buffer point
			//Head SEQ CMD Len(2B) INFO CRC16(2B)
			if ( (buf->start + 7 + buf->len)  < (c2s_data.rx+GSM_BUF_LENGTH) ) {
				c2s_data.rx_out_last = buf->start + 7 + buf->len ;
			}
			else { //CHK in the end of buffer
				c2s_data.rx_out_last = buf->start + 7 + buf->len - GSM_BUF_LENGTH ;
			}

			OS_ENTER_CRITICAL();
			c2s_data.rx_full = false; //reset the full flag
			if (c2s_data.rx_out_last==c2s_data.rx_in_last) {
				c2s_data.rx_empty = true ;//set the empty flag
			}
			OS_EXIT_CRITICAL();

			//debug_ptc("Next add: %X\r\n",c2s_data.rx_out_last);
			//debug_ptc("In last: %X\r\n",c2s_data.rx_in_last);
			//debug_ptc("rx_empty: %X\r\n",c2s_data.rx_empty);

			//update the next status
			buf->status = S_HEAD;

		}//end of (c2s_data.rx_in_last > c2s_data.rx_out_last) ...
		else { //
			debug_ptc("Buffer no enough. %d\r\n",__LINE__);
		}
	}//end of if ( buf->status == S_CHK )

	return 0 ;
}

static unsigned int calc_free_buffer(unsigned char *in,unsigned char *out,unsigned int len)
{
	//计算buf 里有多少字节
	//调用前，请确保 buffer 不为空

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
	unsigned char index;
	unsigned short i , data_len;
	unsigned int seq ;

#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	i = 0 , seq = 0 ;

	if ( out_pcb >= 0x80 ) {//响应服务器命令
		//queue_cnt use as srv_cmd index for this out_pcb

		//no process occupy && have enough buffer
		//Head SEQ CMD Len(2B) Status CRC16
		if ( !c2s_data.tx_lock \
			&& c2s_data.tx_len < (GSM_BUF_LENGTH-10) ) {

			OS_ENTER_CRITICAL();
			c2s_data.tx_lock = true ;
			OS_EXIT_CRITICAL();

			//prepare GSM command
			c2s_data.tx[c2s_data.tx_len]   = GSM_HEAD ;
			c2s_data.tx[c2s_data.tx_len+1] = c2s_data.srv_cmd[queue_cnt].seq ;//SEQ
			c2s_data.tx[c2s_data.tx_len+2] = out_pcb ;//PCB

			c2s_data.tx[c2s_data.tx_len+3] = 0 ;//length high
			c2s_data.tx[c2s_data.tx_len+4] = 1 ;//length low

			c2s_data.tx[c2s_data.tx_len+5] = 0 ;//Status

			i = 0xFFFF & crc16tablefast(&c2s_data.tx[c2s_data.tx_len] , 6 , 0);

			c2s_data.tx[c2s_data.tx_len+6] = (i >> 8) &0xFF ;
			c2s_data.tx[c2s_data.tx_len+7] = (i ) &0xFF ;

			debug_ptc("Respond srv %c CMD , crc:%04X\r\n",\
						(c2s_data.tx[c2s_data.tx_len+2])&0x7F,i&0xFFFF);

			//update buf length
			c2s_data.tx_len = c2s_data.tx_len + 8 ;

			OS_ENTER_CRITICAL();
			c2s_data.tx_lock = false ;
			OS_EXIT_CRITICAL();

			return 0 ;
		}//end of if ( !c2s_data.tx_lock && c2s_data.tx_len < (GSM_BUF_LENGTH-10))
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
	}//end of ( out_pcb >= 0x80 )
	else { //正常发送命令给服务器
		//find a no use SENT_QUEUE to record the SEQ/CMD
		for ( index = 0 ; index < queue_cnt ; index++) {
	
			if ( c2s_data.queue_sent[index].send_pcb == 0 ) { //no use
	
				//prompt("Send PCB: %c, tx_len: %d, tx_lock: %d, ",\
					out_pcb,c2s_data.tx_len,c2s_data.tx_lock);
				//printf("queue: %02d\r\n",index);
	
				if ( !c2s_data.tx_lock \
					&& c2s_data.tx_len < (GSM_BUF_LENGTH-512) ) {//can be reduced, check Max. record
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
					if ( *sequence >= 0x7F ) { //Client => server, seq: 0~7F
						*sequence = 0 ;
					}
					else {
						*sequence = c2s_data.tx[c2s_data.tx_len+1]+1;//increase seq
					}

					c2s_data.tx[c2s_data.tx_len+2] = out_pcb ;//PCB
	
					debug_ptc("Snd %c CMD , seq:%02X, queue: %02d\r\n",\
								out_pcb,*sequence,index);
	
					if ( out_pcb == GSM_CMD_TIME ) {//Max. 6 Bytes
						my_icar.stm32_rtc.update_timer = RTC_GetCounter( );
	
						c2s_data.tx[c2s_data.tx_len+3] = 0;//length high
						c2s_data.tx[c2s_data.tx_len+4] = 0;//length low
					}
	
					if ( out_pcb == GSM_CMD_ERROR ) {//Max. 13 Bytes
						//HEAD SEQ PCB Length(2 bytes) DATA(6 Bytes) crc16
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
	
						switch ( i ) {
						case 0x0://GSM module poweroff err ==>had been cancle
							//GSM reason
							prompt("!!! Update firmware, the GSM power off had cancle!!! %d\r\n",__LINE__);
		
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
					}
	
					if ( out_pcb == GSM_CMD_RECORD ) {//Max. 512
						//HEAD SEQ PCB Length(2 bytes) DATA(N bytes) CRC
						//DATA: UTC(4B)+Val(4B)+UTC(4B)+Val(4B)+...
			
						//move record data to send buffer
						OS_ENTER_CRITICAL();
						my_icar.record_lock = true;
						OS_EXIT_CRITICAL();

						for ( i = 0 ; i < my_icar.record_cnt ; i++) {
							
							if ( i*8 > 512 ) break; //consider: buffer overflow

							//debug_record("Copy R[%02d], idx = %d to send buf.\r\n",i,\
									(my_icar.rec_new[i].val)&0xFF);

							//copy to send buffer
							//record time
							c2s_data.tx[c2s_data.tx_len+i*8+5] = ((my_icar.rec_new[i].time)>>24)&0xFF;
							c2s_data.tx[c2s_data.tx_len+i*8+6] = ((my_icar.rec_new[i].time)>>16)&0xFF;
							c2s_data.tx[c2s_data.tx_len+i*8+7] = ((my_icar.rec_new[i].time)>>8)&0xFF;
							c2s_data.tx[c2s_data.tx_len+i*8+8] = ((my_icar.rec_new[i].time))&0xFF;
							//record val:
							c2s_data.tx[c2s_data.tx_len+i*8+9] = ((my_icar.rec_new[i].val)>>24)&0xFF;
							c2s_data.tx[c2s_data.tx_len+i*8+10]= ((my_icar.rec_new[i].val)>>16)&0xFF;
							c2s_data.tx[c2s_data.tx_len+i*8+11]= ((my_icar.rec_new[i].val)>>8)&0xFF;
							//record item index
							c2s_data.tx[c2s_data.tx_len+i*8+12] = ((my_icar.rec_new[i].val))&0xFF;
						}

						//move un-copy data to ahead
						for ( data_len = 0 ; data_len < (my_icar.record_cnt - i); data_len++) {
							my_icar.rec_new[data_len].time= my_icar.rec_new[data_len+i].time;
							my_icar.rec_new[data_len].val = my_icar.rec_new[data_len+i].val;
						}//2013/2/16 15:47:38 verified
							
						my_icar.record_cnt = my_icar.record_cnt - i ;

						//debug_record("record cnt= %d\r\n", my_icar.record_cnt);
						
						OS_ENTER_CRITICAL();
						my_icar.record_lock = false;
						OS_EXIT_CRITICAL();

						//update buffer length
						c2s_data.tx[c2s_data.tx_len+3] = (i*8)>>8 ; //length high
						c2s_data.tx[c2s_data.tx_len+4] = (i*8)&0xFF;//length low
					}
	
					if ( out_pcb == GSM_CMD_UPGRADE ) {//Max. 10 B
	
						if ( flash_upgrade_ask( &c2s_data.tx[c2s_data.tx_len] )){
							//failure, maybe error
							prompt("Flash upgrade CMD err, %s:%d\r\n",\
								__FILE__,__LINE__);
						}
						else { //ok
							debug_ptc("Flash upgrade CMD len: %d\r\n",\
							((c2s_data.tx[c2s_data.tx_len+3]<<8)|c2s_data.tx[c2s_data.tx_len+4]));
						}
					}
	
					if ( out_pcb == GSM_CMD_UPDATE ) {//Max. 6 Bytes
						//HEAD SEQ PCB Len(2B) HW(2B) + FW(2B) + Max.FW size(2B) CRC16
	
						c2s_data.tx[c2s_data.tx_len+3] = 0;//length high
						c2s_data.tx[c2s_data.tx_len+4] = 6;//length low
						c2s_data.tx[c2s_data.tx_len+5] = my_icar.hw_rev;
						c2s_data.tx[c2s_data.tx_len+6] = my_icar.hw_rev;
						c2s_data.tx[c2s_data.tx_len+7] = (my_icar.fw_rev>>8)&0xFF;//fw rev. high
						c2s_data.tx[c2s_data.tx_len+8] = (my_icar.fw_rev)&0xFF;//fw rev. low
						c2s_data.tx[c2s_data.tx_len+9] = 0;
						c2s_data.tx[c2s_data.tx_len+10] = 0; //Max. FW size
	
					}
	
					if ( out_pcb == GSM_CMD_WARN ) {//Max. 11 Bytes
						//HEAD SEQ PCB Length(2 bytes) DATA(4 Bytes) check
						//DATA: file(1 byte)+warn_code(1 byte)+line(2 B) + idx
						//warn_msg in: *record_seq
	
						i = (record_seq - &my_icar.warn[0].msg)>>1 ;//calc position
						debug_ptc("Warn MSG idx: %d, %08X - %08X = %d\r\n",i,\
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
					}
	
					data_len = ((c2s_data.tx[c2s_data.tx_len+3]<<8)|c2s_data.tx[c2s_data.tx_len+4]);
					//generate CRC16
					i = 0xFFFF & crc16tablefast(&c2s_data.tx[c2s_data.tx_len] , \
						data_len+5,0);
		
					c2s_data.tx[c2s_data.tx_len+data_len+5] = (i >> 8) &0xFF ;
					c2s_data.tx[c2s_data.tx_len+data_len+6] = (i ) &0xFF ;

					debug_ptc("Tx: ");
					for ( i = 0 ; i < data_len+7 ; i++ ) {
						printf("%02X ",c2s_data.tx[c2s_data.tx_len+i]);
					}
					printf("to server\r\n");

					//update buf length
					c2s_data.tx_len = c2s_data.tx_len + data_len + 7 ;

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
}

//return 0: ok
//return 1: no send queue
//return 2: no free buffer or buffer busy
static unsigned char gsm_send_login( unsigned char *sequence, unsigned char queue_cnt)
{
	unsigned char index,ip_length;
	u16 crc16;
	//unsigned int var_u32;

	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < queue_cnt ; index++) {

		if ( c2s_data.queue_sent[index].send_pcb == 0 ) { //no use

			my_icar.stm32_rtc.update_timer = RTC_GetCounter( ) ;

			//HEAD SEQ CMD Length(2 bytes) OSTime SN(char 10) HW/FW_REV(4B) IP CRC16
			//Max.: 1+1+1+2+4+10+4+16+2=41, need < tx_sn[48] in app_taskmanager.h
			//IE: The MCU ID is 669FF48 56486749 87101637  SN:DEMOD830B6
			//Login CMD: (38 Bytes)
			//DE 00 4C 00 1F 00 00 0A 1D 44 45 4D 4F 44 38 33 30 42 36 00
			//00 00 0D 31 30 2E 31 38 33 2E 32 31 33 2E 34 33 75 83 

			if ( strlen((char *)my_icar.mg323.ip_local) > 7 ) {//1.2.3.4

				//set protocol string value
				c2s_data.queue_sent[index].send_timer= OSTime ;
				c2s_data.queue_sent[index].send_seq = *sequence ;
				c2s_data.queue_sent[index].send_pcb = GSM_CMD_LOGIN ;
				c2s_data.queue_count++;
	
				ip_length = strlen((char *)my_icar.mg323.ip_local) ;

				//prepare GSM command
				c2s_data.tx_sn[0]   = GSM_HEAD ;
				c2s_data.tx_sn[1] = *sequence ;//SEQ
				if ( *sequence >= 0x7F ) { //Client => server, seq: 0~7F
					*sequence = 0 ;
				}
				else {
					*sequence = c2s_data.tx_sn[1]+1;//increase seq
				}
				c2s_data.tx_sn[2] = GSM_CMD_LOGIN ;//PCB
				c2s_data.tx_sn[3] = 0  ;//length high
				c2s_data.tx_sn[4] = ip_length+18;//len(SN)+len(OSTime)+len(HW/FW_REV(4B))

				//record OSTime, 4 bytes
				c2s_data.tx_sn[5] = (OSTime>>24)&0xFF  ;//OSTime high
				c2s_data.tx_sn[6] = (OSTime>>16)&0xFF  ;//OSTime high
				c2s_data.tx_sn[7] = (OSTime>>8)&0xFF  ;//OSTime low
				c2s_data.tx_sn[8] =  OSTime&0xFF  ;//OSTime low

				//Product serial number
				strncpy((char *)&c2s_data.tx_sn[9], (char *)my_icar.sn, 10);

				//Product HW rev, FW rev
				c2s_data.tx_sn[19] =  my_icar.hw_rev  ;//hw revision, 1 byte
				c2s_data.tx_sn[20] =  0  ;//reverse
				c2s_data.tx_sn[21] =  (my_icar.fw_rev >> 8)&0xFF ;//FW rev. high
				c2s_data.tx_sn[22] =  (my_icar.fw_rev )&0xFF ;//FW rev. low
				
				//Local IP
				strncpy((char *)&c2s_data.tx_sn[23], \
						(char *)my_icar.mg323.ip_local, IP_LEN);

				crc16 = 0xFFFF & crc16tablefast(c2s_data.tx_sn , 23+ip_length, 0);
				c2s_data.tx_sn[23+ip_length] = (crc16 >> 8) &0xFF ;
				c2s_data.tx_sn[24+ip_length] = crc16 &0xFF ;

				c2s_data.tx_sn_len = 25+ip_length ;

				debug_ptc("%c CMD, Len: %d Bytes, CRC16: %X\r\n",\
							c2s_data.tx_sn[0],c2s_data.tx_sn_len,crc16);
				
				return 0 ;
			}//end of if ( strlen(my_icar.mg323.ip_local) > 7 )
			else {
				return 2 ;
			}//No IP
		}//end of c2s_data.queue_sent[index].send_pcb == 0
	}

	prompt("No free queue: %d check %s:%d\r\n",index,__FILE__, __LINE__);
	return 1;
}

//Return 0: ok, others error.
/*
static unsigned char record_queue_update( unsigned char *buf, unsigned char *buf_start) 
{
	unsigned char var_u8, rec_idx;
	unsigned short buf_index, buf_len ;

	//Head SEQ CMD Len INFO CRC16
	//DE 01 52 00 xx 
	//01 02 03 04 => require record index
	//1A 59  =>CRC16

	if ( (buf+4) < buf_start+GSM_BUF_LENGTH ) {
		buf_len = *(buf+4);
	}
	else {
		buf_len = *(buf+4-GSM_BUF_LENGTH);
	}

	if ( (buf+3) < buf_start+GSM_BUF_LENGTH ) {
		buf_len = ((*(buf+3))<<8) | buf_len;
	}
	else {
		buf_len = ((*(buf+3-GSM_BUF_LENGTH))<<8) | buf_len;
	}

	debug_record("Len= %d \tAsk: ",buf_len);

	for ( buf_index= 0 ; buf_index < buf_len ; buf_index++ ) {
		if ( (buf+buf_index+5) < buf_start+GSM_BUF_LENGTH ) {
			rec_idx = *(buf+buf_index+5);
		}
		else {//data in begin of buffer
			rec_idx = *(buf+buf_index+5-GSM_BUF_LENGTH);
		}
		
		#ifdef DEBUG_RECORD 
			printf("%02X ",rec_idx); 
		#endif
		
		if ( (rec_idx < MAX_RECORD_IDX) && rec_idx != 0xFF) {//leagle record require
			my_icar.rec[rec_idx].req++;
		}
		else { //illegal require, report to server
			prompt("Exceed MAX_RECORD_IDX! %d\r\n",__LINE__);
			//Report warn to server
			for ( var_u8 = 0 ; var_u8 < MAX_WARN_MSG ; var_u8++) {
				if ( !my_icar.warn[var_u8].msg ) { //empty msg	
					//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
					my_icar.warn[var_u8].msg = (F_APP_TASKMANAGER) << 24 ;
					my_icar.warn[var_u8].msg |= W_NO_REC_IDX << 16 ;//No record index
					my_icar.warn[var_u8].msg |= __LINE__ ;
					break  ;//end the loop
				}
			}
		}
	}

	debug_record("\r\n");

	return 0 ;
}

*/
void console_cmd( unsigned char cmd, unsigned char *flag, unsigned char *flash_idx )
{
	CPU_INT08U	os_err;
	unsigned char var_u8;

	prompt("%c\r\n",cmd);

	switch ( cmd ) {

	case 'b' :
		prompt("Print Backup register info ... \r\n");
		show_err_log( );
		break;

	case 'c' ://OBD CAN TX test
		prompt("CAN TX...\r\n");
		my_icar.obd.can_tx_cnt++;
		OSSemPost( sem_obd_task );
		break;

	case 'C' ://Clear OBD type
		prompt("Clean OBD type ...\r\n");
		BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, for dev. only
		BKP_WriteBackupRegister(BKP_DR3, 0);//clean all flag, for dev. only
		break;

	case 'd' ://set debug flag
		my_icar.debug++ ;
		prompt("Increase debug lever, my_icar.debug:%d\r\n",my_icar.debug);
		break;

	case 'D' ://reset debug flag
		my_icar.debug = 0 ;
		prompt("Reset debug flag... my_icar.debug:%d\r\n",my_icar.debug);
		break;

	case 'f' ://show flash page67
		//show flash content
		prompt("Page:%d, %08X: %08X ",\
			(my_icar.upgrade.base+(*flash_idx)*16-0x08000000)/my_icar.upgrade.page_size,\
			my_icar.upgrade.base+(*flash_idx)*16,\
			*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+4));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+8));
		printf("%08X \r\n",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+12));

		var_u8 = *flash_idx; var_u8++; *flash_idx = var_u8; //increase flash_idx
		break;

	case 'F' ://show flash page67
		var_u8 = *flash_idx; var_u8=var_u8-2; *flash_idx = var_u8; //decrease flash_idx
		//show flash content
		prompt("Page:%d, %08X: %08X ",\
			(my_icar.upgrade.base+(*flash_idx)*16-0x08000000)/my_icar.upgrade.page_size,\
			my_icar.upgrade.base+(*flash_idx)*16,\
			*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+4));
		printf("%08X ",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+8));
		printf("%08X \r\n",*(vu32*)(my_icar.upgrade.base+(*flash_idx)*16+12));
		break;

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
		prompt("Send a warn msg to server\r\n");
		//Report warn to server
		for ( var_u8 = 0 ; var_u8 < MAX_WARN_MSG ; var_u8++) {
			if ( !my_icar.warn[var_u8].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_u8].msg = (F_MAIN) << 24 ;
				my_icar.warn[var_u8].msg |= W_TEST << 16 ;//Error CAN ID index
				my_icar.warn[var_u8].msg |= __LINE__ ;
				break  ;//end the loop
			}
		}

		break;

	case 'Y' ://sure to reset mcu
		if ( *flag ) {
			prompt("*** System will reset! ***");
			my_icar.upgrade.status = UPGRADED ;//MCU will be rst by this flag
		}
		break;

	default:
		prompt("Unknow CMD %c, current support: b,c,C,d,D,f,F,g,G,P,R,v,W\r\n",cmd);
		*flag = 0 ;
		break;
	}
}
