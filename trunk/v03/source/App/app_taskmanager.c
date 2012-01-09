#include "main.h"

static	OS_STK		   App_TaskGsmStk[APP_TASK_GSM_STK_SIZE];
static unsigned char mcu_id_eor( void );

extern struct icar_rx u1_rx_buf;
extern struct icar_tx u1_tx_buf;
extern struct gsm_status mg323_status ;
extern struct gsm_command mg323_cmd ;
extern struct rtc_status stm32_rtc;
extern struct icar_adc_buf adc_temperature;

unsigned char pro_sn[]="02P11AH0xx";
//Last 2 bytes replace by MCU ID xor result
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
	unsigned char u1_cmd , chkbyte=0, gsm_sequence=0;
	unsigned char *respond_start=NULL;
	unsigned char  respond_pcb=0, respond_seq=0, respond_chk=0;
	unsigned char  respond_len_h=0, respond_len_l=0;
	unsigned int   respond_len = 0, respond_time=0;
	//note:从S_PCB开始计时respond_time,如果>TCP_TIMEOUT,则重置状态为S_HEAD
	unsigned int i , rtc_update_timer=0;
	protocol_status cur_status = S_HEAD;
	struct protocol_string pro_str[MAX_CMD_QUEUE];
	unsigned char pro_str_index=0 ;
	u16 adc;
#if OS_CRITICAL_METHOD == 3                      /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	(void)p_arg;

	/* Initialize the protocol string queue.	*/
	for ( pro_str_index = 0 ; pro_str_index < MAX_CMD_QUEUE ; pro_str_index++) {
		pro_str[pro_str_index].send_time= 0 ;//free queue if > 1 hours
		pro_str[pro_str_index].send_pcb = 0 ;
	}

	/* Initialize the SysTick.								*/
	OS_CPU_SysTickInit();

	//prompt("\r\n\r\n%s, line:	%d\r\n",__FILE__, __LINE__);
	prompt("Micrium	uC/OS-II V%d.%d\r\n", OSVersion()/100,OSVersion()%100);
	prompt("TickRate: %d\t\t", OS_TICKS_PER_SEC);
	printf("OSCPUUsage: %d\r\n", OSCPUUsage);
	chkbyte = mcu_id_eor( );
	prompt("The MCU ID is %X %X %X\tEOR:%02X\r\n",\
		*(vu32*)(0x1FFFF7E8),*(vu32*)(0x1FFFF7EC),*(vu32*)(0x1FFFF7F0),chkbyte);

	snprintf((char *)&pro_sn[8],3,"%02X",chkbyte);

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

	//enable temperature adc DMA
	//DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA1_Channel1->CCR |= DMA_CCR1_EN;

	//wait power stable
	OSTimeDlyHMSM(0, 0,	1, 0);

	//independent watchdog init
	iwdg_init( );

	mg323_status.ask_power = true ;
	respond_time = OSTime ;

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

		if ( !stm32_rtc.updating ) {//no in updating process
			if ( (RTC_GetCounter( ) - stm32_rtc.update_time) > 1*60*60 || \
					stm32_rtc.update_time == 0 ) {//need update RTC by server time
				prompt("Need update RTC, mg323_cmd.tx_len= %d\r\n",mg323_cmd.tx_len);
				rtc_update_timer = OSTime ;

				//find a no use protocol_string to record the SEQ/CMD
				for ( pro_str_index = 0 ; pro_str_index < MAX_CMD_QUEUE ; pro_str_index++) {
					if ( pro_str[pro_str_index].send_pcb == 0 ) { //no use
						//prompt("pro_str %d is no use.\r\n",pro_str_index);
						break ;
					}//end of pro_str[pro_str_index].send_pcb == 0
				}

				//HEAD SEQ CMD Length(2 bytes) SN(char 10) check
				if ( !mg323_cmd.lock \
					&& mg323_cmd.tx_len < (GSM_BUF_LENGTH-20) \
					&& pro_str_index < MAX_CMD_QUEUE ) {
					//no process occupy && have enough buffer

					OS_ENTER_CRITICAL();
					mg323_cmd.lock = true ;
					OS_EXIT_CRITICAL();

					//set protocol string value
					pro_str[pro_str_index].send_time= OSTime ;
					pro_str[pro_str_index].send_seq = gsm_sequence ;
					pro_str[pro_str_index].send_pcb = GSM_CMD_TIME ;

					//prepare GSM command
					mg323_cmd.tx[mg323_cmd.tx_len]   = GSM_HEAD ;
					mg323_cmd.tx[mg323_cmd.tx_len+1] = gsm_sequence ;//SEQ
					gsm_sequence++;
					mg323_cmd.tx[mg323_cmd.tx_len+2] = GSM_CMD_TIME ;//PCB
					mg323_cmd.tx[mg323_cmd.tx_len+3] = 0  ;//length high
					mg323_cmd.tx[mg323_cmd.tx_len+4] = 10 ;//length low
					strncpy((char *)&mg323_cmd.tx[mg323_cmd.tx_len+5], (char *)pro_sn, 10);

					prompt("GSM CMD: %02X ",mg323_cmd.tx[mg323_cmd.tx_len]);
					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < 15 ; i++ ) {//calc chkbyte
						chkbyte ^= mg323_cmd.tx[mg323_cmd.tx_len+i];
						printf("%02X ",mg323_cmd.tx[mg323_cmd.tx_len+i]);
					}
					mg323_cmd.tx[mg323_cmd.tx_len+15] = chkbyte ;
					printf("%02X\r\n",mg323_cmd.tx[mg323_cmd.tx_len+15]);
					//update buf length
					mg323_cmd.tx_len = mg323_cmd.tx_len + 16 ;
					stm32_rtc.updating = true ;

					OS_ENTER_CRITICAL();
					mg323_cmd.lock = false ;
					OS_EXIT_CRITICAL();
				}//end of if ( !mg323_cmd.lock && mg323_cmd.tx_len < (GSM_BUF_LENGTH-20))
			}//end of f ( (RTC_GetCounter( ) ...
		}//end of if ( !stm32_rtc.updating ) ...
		else {
			//if ( OSTime - rtc_update_timer > 2*60*1000 ) { //timeout
			if ( OSTime - rtc_update_timer > 10*1000 ) { //short for test
				stm32_rtc.updating = false ;//restart update process again
			}
		}

		//prompt("Sat:%d\trx_empty=%d\t In:%X\r\n",\
		//cur_status,mg323_cmd.rx_empty,mg323_cmd.rx_in_last);
		if ( !mg323_cmd.rx_empty ) {//receive some TCP data from GSM
			//printf("<%02X  ",*mg323_cmd.rx_out_last);

			//status transfer: S_HEAD -> S_PCB -> S_CHK ==> S_HEAD...
			//1, find HEAD
			if ( cur_status == S_HEAD ) {
				while ( !mg323_cmd.rx_empty && cur_status == S_HEAD) {

					if ( *mg323_cmd.rx_out_last == GSM_HEAD ) {//found
						respond_start = mg323_cmd.rx_out_last ;//mark the start
						cur_status = S_PCB ;//search protocol control byte
						respond_time = OSTime ;
						//prompt("Found HEAD: %X\r\n",respond_start);
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
			if ( cur_status == S_PCB ) {

				if ( OSTime - respond_time > 5*AT_TIMEOUT ) {//reset status
					prompt("In S_PCB timeout, reset to S_HEAD status!!!\r\n");
					cur_status = S_HEAD ;
					mg323_cmd.rx_out_last++	 ;
				}

				if ( ((mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) \
					&&  (mg323_cmd.rx_in_last - mg323_cmd.rx_out_last > 5))\
					|| ((mg323_cmd.rx_out_last > mg323_cmd.rx_in_last) \
					&& (GSM_BUF_LENGTH - \
					(mg323_cmd.rx_out_last - mg323_cmd.rx_in_last) > 5))) {
					//HEAD SEQ CMD Length(2 bytes) = 5 bytes

					//get PCB
					if ( (respond_start + 2)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						//PCB no in the end of buffer
						respond_pcb = *(respond_start + 2);
					}
					else { //PCB in the end of buffer
						respond_pcb = *(respond_start + 2 - GSM_BUF_LENGTH);
					}//end of (respond_start + 2)  < (mg323_cmd.rx+GSM_BUF_LENGTH)
					//printf("\r\nrespond PCB is %02X\t",respond_pcb);

					//get sequence
					if ( (respond_start + 1)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						//SEQ no in the end of buffer
						respond_seq = *(respond_start + 1);
					}
					else { //SEQ in the end of buffer
						respond_seq = *(respond_start + 1 - GSM_BUF_LENGTH);
					}//end of (respond_start + 1)  < (mg323_cmd.rx+GSM_BUF_LENGTH)
					//printf("SEQ is %02X\t",respond_seq);

					//get len high
					if ( (respond_start + 3)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						//LEN no in the end of buffer
						respond_len_h = *(respond_start + 3);
					}
					else { //LEN in the end of buffer
						respond_len_h = *(respond_start + 3 - GSM_BUF_LENGTH);
					}//end of GSM_BUF_LENGTH - (respond_start - mg323_cmd.rx)
					//printf("respond LEN H: %02d\t",respond_len_h);

					//get len low
					if ( (respond_start + 4)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						//LEN no in the end of buffer
						respond_len_l = *(respond_start + 4);
					}
					else { //LEN in the end of buffer
						respond_len_l = *(respond_start + 4 - GSM_BUF_LENGTH);
					}//end of GSM_BUF_LENGTH - (respond_start - mg323_cmd.rx)
					//printf("L: %02d ",respond_len_l);

					respond_len = respond_len_h << 8 | respond_len_l ;
					//printf("respond LEN: %d\r\n",respond_len);


					//update the status & timer
					cur_status = S_CHK ;//search check byte
					respond_time = OSTime ;
				}//end of (mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) ...
			}//end of if ( cur_status == S_PCB )

			//3, find CHK
			if ( cur_status == S_CHK ) {

				if ( OSTime - respond_time > 10*AT_TIMEOUT ) {//reset status
					prompt("In S_CHK timeout, reset to S_HEAD status!!!\r\n");
					cur_status = S_HEAD ;
					mg323_cmd.rx_out_last++	 ;
				}

				if ( ((mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) \
					&&  (mg323_cmd.rx_in_last - mg323_cmd.rx_out_last > (5+respond_len)))\
					|| ((mg323_cmd.rx_out_last > mg323_cmd.rx_in_last) \
					&& (GSM_BUF_LENGTH - \
					(mg323_cmd.rx_out_last - mg323_cmd.rx_in_last) > (5+respond_len)))) {
					//buffer > HEAD SEQ CMD Length(2 bytes) = 5 bytes + LEN + CHK(1)

					//prompt("Search chk...\r\n");
					//prompt("mg323_cmd.rx: %X\tEnd:%X\trespond_start: %X\r\n",\
						//mg323_cmd.rx,mg323_cmd.rx+GSM_BUF_LENGTH,respond_start);

					//get CHK
					if ( (respond_start + 5+respond_len)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						//CHK no in the end of buffer
						respond_chk = *(respond_start + 5 + respond_len );
						//printf("CHK add: %X\t",respond_start + 5 + respond_len);
					}
					else { //CHK in the end of buffer
						respond_chk = *(respond_start + 5 + respond_len - GSM_BUF_LENGTH);
						//printf("CHK add: %X\t",respond_start + 5 + respond_len - GSM_BUF_LENGTH);
					}//end of GSM_BUF_LENGTH - (respond_start - mg323_cmd.rx)
					//printf("respond CHK is %02X\n",respond_chk);
					//2012/1/4 19:54:50 已验证边界情况，正常

					chkbyte = GSM_HEAD ;
					for ( i = 1 ; i < respond_len+5 ; i++ ) {//calc chkbyte
						if ( (respond_start+i) < mg323_cmd.rx+GSM_BUF_LENGTH ) {
							chkbyte ^= *(respond_start+i);
							//printf("%02X ",*(respond_start+i));
							//printf("Data add: %X\r\n",(respond_start+i));
						}
						else {//data in begin of buffer
							chkbyte ^= *(respond_start+i-GSM_BUF_LENGTH);
							//printf("%02X ",*(respond_start+i-GSM_BUF_LENGTH));
							//printf("Data add: %X\r\n",(respond_start+i-GSM_BUF_LENGTH));
						}
					}
					//printf("Calc. CHK: %X\r\n",chkbyte);

					if ( chkbyte == respond_chk ) {//data correct
						//find the sent record in pro_str by SEQ
						for ( pro_str_index = 0 ; pro_str_index < MAX_CMD_QUEUE ; pro_str_index++) {
							if ( pro_str[pro_str_index].send_seq == respond_seq \
								&& respond_pcb==(pro_str[pro_str_index].send_pcb | 0x80)) { 
	
								//prompt("pro_str %d is correct record.\r\n",pro_str_index);
								//found, release this record
								pro_str[pro_str_index].send_pcb = 0 ;
								break ;
							}//end of pro_str[pro_str_index].send_pcb == 0
						}

						//handle the respond
						switch (respond_pcb&0x7F) {
	
						case 0x4C://'L':
							break;

						case GSM_CMD_TIME://0x54,'T'
							prompt("Time respond PCB: 0x%X\r\n",respond_pcb&0x7F);

							break;

						default:
							prompt("Unknow respond PCB: 0x%X\r\n",respond_pcb&0x7F);

							break;
						}//end of handle the respond

					}//end of if ( chkbyte == respond_chk )
					else {//data no correct
						prompt("Rec data CHK no correct!\r\n");
					}

					//update the buffer point
					if ( (respond_start + 6+respond_len)  < (mg323_cmd.rx+GSM_BUF_LENGTH) ) {
						mg323_cmd.rx_out_last = respond_start + 6 + respond_len ;
					}
					else { //CHK in the end of buffer
						mg323_cmd.rx_out_last = respond_start + 6 + respond_len - GSM_BUF_LENGTH ;
					}
					//printf("Next add: %X\r\n",mg323_cmd.rx_out_last);

					OS_ENTER_CRITICAL();
					mg323_cmd.rx_full = false; //reset the full flag
					if (mg323_cmd.rx_out_last==mg323_cmd.rx_in_last) {
						mg323_cmd.rx_empty = true ;//set the empty flag
					}
					OS_EXIT_CRITICAL();

					//update the next status
					cur_status = S_HEAD;

				}//end of (mg323_cmd.rx_in_last > mg323_cmd.rx_out_last) ...
				else { //
					;//prompt("Buffer no enough.\r\n");
				}
			}//end of if ( cur_status == S_CHK )

		}

		if ( u1_rx_buf.lost_data ) {//error! lost data
			prompt("Uart1 lost data, check: %s: %d\r\n",__FILE__, __LINE__);
			u1_rx_buf.lost_data = false ;
		}

		while ( !u1_rx_buf.empty ) {//receive some data from console...
			u1_cmd = getbyte( COM1 ) ;
			putbyte( COM1, u1_cmd );
			if ( u1_cmd == 'o' || u1_cmd == 'O' ) {//online
				prompt("Ask GSM online...\r\n");
				mg323_status.ask_online = true ;
			}
			if ( u1_cmd == 'c' || u1_cmd == 'C' ) {//close
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

			if ( (OSTime/1000)%3 == 0 ) {
				prompt("T: %d.%02d C",adc/100,adc%100);
				printf("\tRTC:%d\r\n",RTC_GetCounter());
			}
		}
				 
		/* Insert delay	*/
		//OSTimeDlyHMSM(0, 0,	1, 0);
		OSTimeDlyHMSM(0, 0,	0, 800);
		//printf("L%010d\r\n",OSTime);
		led_toggle( OBD_UNKNOW ) ;
		//led_toggle( OBD_KWP ) ;

		if ( (OSTime/1000)%10 == 0 ) {//check every 10 sec
			for ( pro_str_index = 0 ; pro_str_index < MAX_CMD_QUEUE ; pro_str_index++) {
				if ( OSTime - pro_str[pro_str_index].send_time > 60*60*1000 ) {
					pro_str[pro_str_index].send_time= 0 ;//free queue if > 1 hours
					pro_str[pro_str_index].send_pcb = 0 ;
				}
			}
		}//end of check every 10 sec
	}
}

static unsigned char mcu_id_eor( )
{
	static unsigned char chkbyte ;

	//Calc. MCU ID eor result as product SN
	chkbyte =  ((*(vu32*)(0x1FFFF7E8)) >> 24)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7E8)) >> 16)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7E8)) >> 8)&0xFF ;
	chkbyte ^= (*(vu32*)(0x1FFFF7E8))&0xFF ;

	chkbyte ^= ((*(vu32*)(0x1FFFF7EC)) >> 24)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7EC)) >> 16)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7EC)) >> 8)&0xFF ;
	chkbyte ^= (*(vu32*)(0x1FFFF7EC))&0xFF ;

	chkbyte ^= ((*(vu32*)(0x1FFFF7F0)) >> 24)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7F0)) >> 16)&0xFF ;
	chkbyte ^= ((*(vu32*)(0x1FFFF7F0)) >> 8)&0xFF ;
	chkbyte ^= (*(vu32*)(0x1FFFF7F0))&0xFF ;

	return chkbyte ;
}
