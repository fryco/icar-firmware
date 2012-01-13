#include "main.h"

struct gsm_status mg323_status ;
struct gsm_command mg323_cmd ;
extern struct icar_rx u2_rx_buf;
extern unsigned char dest_server[];

const unsigned char callback_phone[] = "13828431106";

void  App_TaskGsm (void *p_arg)
{
	unsigned char rec_str[AT_CMD_LENGTH];
	unsigned char gsm_rx_buf_len=0, rec_str_len=0;
	unsigned int i , relay_on_time=0, dial_time=0 ;
	unsigned char ring_timer = 0 , online_try_timer=0;
	bool dial_flag = false, voice_confirm = true ;

#if OS_CRITICAL_METHOD == 3                      /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	(void)p_arg;

	mg323_status.server_ip_port = dest_server;
	mg323_status.power_on = false;
	mg323_status.gprs_ready = false;
	mg323_status.tcp_online = false;
	mg323_status.ask_online = false;
	mg323_status.apn_index = NULL;
	mg323_status.roam = false;
	mg323_status.cgatt= false;
	mg323_status.rx_empty = true ;
	mg323_status.time = OSTime ;

	OS_ENTER_CRITICAL();
	mg323_cmd.lock = false ;
	mg323_cmd.rx_out_last = mg323_cmd.rx;
	mg323_cmd.rx_in_last  = mg323_cmd.rx;
	mg323_cmd.rx_empty = true ;
	mg323_cmd.rx_full = false ;
	OS_EXIT_CRITICAL();

	uart2_init( );

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//1,需增加，长时间不能连线，复位GSM模块
//2,Uart2 无反馈时，超时，重置状态 online ...
	while ( 1 ) {

		if ( mg323_status.ask_power ) {//ask power on the GSM module
			if ( !mg323_status.power_on ) {
				mg323_status.err_no = gsm_power_on() ;
				if ( mg323_status.err_no == 0 ) {
					mg323_status.power_on = true;
					mg323_status.time = OSTime ;

					//check gprs network also
					if ( !mg323_status.gprs_ready ) {
						mg323_status.err_no = gsm_check_gprs( );
						if ( mg323_status.err_no == 0 ) {
							mg323_status.gprs_ready = true;
						}
						else { //error, maybe no gprs network or APN error
							prompt("Reg gprs failure:%d will try later.\r\n",mg323_status.err_no);
							mg323_status.gprs_ready = false;
							mg323_status.tcp_online = false ;
						}
					}
				}
				else { //error, maybe no GSM network
					prompt("GSM power on failure:%d will try later.\r\n",mg323_status.err_no);
					OSTimeDlyHMSM(0, 0, 3, 0);
					mg323_status.gprs_ready = false;
					mg323_status.tcp_online = false ;
					mg323_status.power_on = false;
					gsm_pwr_off( );
				}
			}
		}
		else {
			;//TBD: power off to save power
		}

		if ( mg323_status.power_on ) {//below action need power on

			//should receive feedback < 10 seconds, because we enquire every 4 sec.
			if ( OSTime - mg323_status.time > 10*AT_TIMEOUT ) {
				//GSM module no respond, reset it
				prompt("\r\nGSM Module no respond, will be reset...%s\tline: %d\r\n",\
						__FILE__, __LINE__);
				mg323_status.gprs_ready = false;
				mg323_status.tcp_online = false ;
				mg323_status.power_on = false;
				gsm_pwr_off( );
			}

			//Check need online or not?
			if ( mg323_status.ask_online ) {
				if ( !mg323_status.tcp_online ) { //no online
					//send online command
					putstring(COM2,"AT^SISO=0\r\n");
					//will be return ^SISW: 0,1,1460
					//confirm this return in later
					online_try_timer++;
					prompt("Try %d to online...\r\n",online_try_timer);
					if ( online_try_timer > 15 ) {//failure > 15
						mg323_status.gprs_ready = false;
						mg323_status.tcp_online = false ;
						mg323_status.power_on = false;
						gsm_pwr_off( );
						prompt("Try %d to online, still failure, \
								reboot GSM module.\r\n",online_try_timer);
						//will be auto power on because ask_power is true
						online_try_timer = 0 ;
						OSTimeDlyHMSM(0, 0, 1, 0);
					}
				}
			}//end of if ( mg323_status.ask_online )

			//Send GSM signal and tcp status cmd every 3 sec.
			
			//if ( (OSTime/1000)%3 == 0 ) {//Signal
				putstring(COM2,"AT+CSQ\r\n");
			//}
			if ( (OSTime/1000)%3 == 1 ) {//connection status
				putstring(COM2,"AT^SISI?\r\n");
			}

			if ( (OSTime/1000)%3 == 2 ) {
				//ask the IP, return:^SICI: 0,2,1,"10.156.174.147"
				putstring(COM2,"AT^SICI?\r\n");
			}

			//waiting GSM respond
			OSTimeDlyHMSM(0, 0, 0, 10);

			//if need dial
			if ( dial_flag && !voice_confirm) {

				if ( OSTime - dial_time > 2*60*1000 ) {//re-dial after 2 mins
					prompt("Call my phone for confirm...");
					dial_time = OSTime ;
					putstring(COM2,"ATD");
					putstring(COM2,(unsigned char *)callback_phone);
					putstring(COM2,";\r\n");
				}
			}

			//Check GSM output string
			while ( !u2_rx_buf.empty ) {//have data ...
				//reset timer here
				mg323_status.time = OSTime ;				

				memset(rec_str, 0x0, AT_CMD_LENGTH);
				if ( get_respond(rec_str) ) {
					;//prompt("Rec:%s\r\n",rec_str);
				}

				//found GSM auto report
				if (strstr((char *)rec_str,"network is unavailable")) {
					mg323_status.tcp_online = false ;
				}

				//found voice call
				if (strstr((char *)rec_str,"RING")) {
					putstring(COM2,"ATH\r\n");
					relay_on_time = OSTime;
					ring_timer++;
					dial_flag = false ;
					voice_confirm = false ;
					putstring(COM2,"ATH\r\n");
					prompt("Receive %d time call.\r\n",ring_timer);
				}

				//found signal respond
				if (strstr((char *)rec_str,"+CSQ:")) {
		            if(rec_str[7] == 0x2c)
		            {
		                mg323_status.signal = rec_str[6] - 0x30; 
		            }
		            else
		            {
		                rec_str[6] = rec_str[6] - 0x30;
		                rec_str[7] = rec_str[7] - 0x30;
		                mg323_status.signal = rec_str[6]*10 + rec_str[7];   
		            }
					prompt("Signal:%02d\r\n",mg323_status.signal);
				}

				//Internet profile status
				// 2: allocated
				// 3: connecting
				// 4: up
				// 5: closing
				// 6: down
				if (strstr((char *)rec_str,"SISI: 0,")) {
					//^SISI: 0,5,0,0,0,0
					if ( rec_str[9]== 0x34 ) {
						mg323_status.tcp_online = true ;
						online_try_timer = 0 ;
					}
					else {
						mg323_status.tcp_online = false ;
						prompt("!!! TCP off line. !!!\r\n");
						//maybe some problem
						OSTimeDlyHMSM(0, 0, 1, 0);
					}
				}

				//found call confirm
				if (strstr((char *)rec_str,"BUSY")) {//call success
					dial_flag = false ;
					voice_confirm = true ;
					dial_time = 0 ; //prepare for next call
					prompt("Confirmed.\r\n");
				}

				//found ^SISW: 0,1,1460
				if (strstr((char *)rec_str,"^SISW: 0,1,1460")) {
					//need to double check
					prompt("TCP online: %s\r\n",rec_str);
					mg323_status.tcp_online = true ;
					//ask the IP, return:^SICI: 0,2,1,"10.156.174.147"
					putstring(COM2,"AT^SICI?\r\n");
				}

				//found IP message
				if (strstr((char *)rec_str,"SICI: 0,2,1,")) {
					i = 0 ;//222.222.222.222
					while ( (rec_str[i+14] != 0x22) && i < 15) { //"
						mg323_status.local_ip[i] = rec_str[i+14];
						i++ ;
					}
					//prompt("IP: %s\t%s\tline: %d\r\n",\
						//mg323_status.local_ip,__FILE__, __LINE__);
				}

				//GSM TCP rec data: ^SISR: 0,0
				if (strstr((char *)rec_str,"^SISR: 0,")) {//Rec tcp data
					//^SISR: 0,0  :no data
					if ( rec_str[10] > 0x30 ) {//have data
						//prompt("TCP rec data: %s\r\n",rec_str);
						mg323_status.rx_empty = false ;
					}
					else {
						mg323_status.rx_empty = true ;
						//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
					}
				}

				//add others respond string here

			}//end of Check GSM output string


			//Check mg323 has data or not
			if ( !mg323_status.rx_empty && mg323_status.tcp_online) { //has data
				//prompt("GSM Module TCP buffer have data.\r\n");

				//get the buffer data
				putstring(COM2,"AT^SISR=0,");
				snprintf((char *)rec_str,AT_CMD_LENGTH-1,"%d\r\n",16);
				//不知为何增大会出问题？
				//each time can't get > AT_CMD_LENGTH, else overflow
				putstring(COM2,rec_str);

				//data will be sent out after ^SISR: 0,xx
				memset(rec_str, 0x0, AT_CMD_LENGTH);

				mg323_cmd.rx_time = OSTime ;

				while ( !strstr((char *)rec_str,"^SISR: 0,") \
						&& !mg323_cmd.rx_full \
						&& (OSTime - mg323_cmd.rx_time) < 5*AT_TIMEOUT ) {

					while ( u2_rx_buf.empty && \
						(OSTime - mg323_cmd.rx_time) < AT_TIMEOUT ) {//no data...
						OSTimeDlyHMSM(0, 0,	0, 100);
					}

					if ( get_respond(rec_str) ) {
						rec_str_len = strlen((char*)rec_str);
						//prompt("rec_str:%s, len: %d\r\n",rec_str,rec_str_len);
						if ( strstr((char *)rec_str,"^SISR: 0,") ) {//found
							//^SISR: 0,10 收到数据10个 or 
							//^SISR: 0,0  no data, update mg323_status.rx_empty

							for ( i = 0 ; i < rec_str_len; i++ ) {
								;//prompt("%02d\t\t%02X\r\n",i,rec_str[i]);
							}

							//提取 buffer 长度
							switch (rec_str_len) {
		
							case 12://^SISR: 0,1
								gsm_rx_buf_len = rec_str[9] - 0x30 ;
								break;

							case 13://^SISR: 0,12
								gsm_rx_buf_len = (rec_str[10] - 0x30)+\
												 ((rec_str[9] - 0x30)*10) ;
								break;

							case 14://^SISR: 0,123
								gsm_rx_buf_len = (rec_str[11] - 0x30)+\
												((rec_str[10] - 0x30)*10)+\
												((rec_str[9]  - 0x30)*100) ;
								break;

							case 15://^SISR: 0,1234
								gsm_rx_buf_len = (rec_str[12] - 0x30)+\
												((rec_str[11] - 0x30)*10)+\
												((rec_str[10] - 0x30)*100)+\
												((rec_str[9]  - 0x30)*1000) ;
								break;

							default:
								prompt("Illegal length %d, check %s, line:	%d\r\n",\
										rec_str_len,__FILE__, __LINE__);
								break;

							}//end of switch

							if ( gsm_rx_buf_len == 0 ) {//no data
								mg323_status.rx_empty = true ;
								//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
							}
							else {
								//push data to mg323_cmd.rx
								//prompt("Push to rx:\t");
								for ( i = 0 ; i < gsm_rx_buf_len; i++ ) {
									while ( u2_rx_buf.empty ) {//no data...
										OSTimeDlyHMSM(0, 0,	0, 10);
									}
									*mg323_cmd.rx_in_last = getbyte( COM2 );
	
									//printf("%02X ",*mg323_cmd.rx_in_last);
	
									mg323_cmd.rx_in_last++;
								   	if (mg323_cmd.rx_in_last==mg323_cmd.rx+GSM_BUF_LENGTH) {
										mg323_cmd.rx_in_last=mg323_cmd.rx;//地址到顶部回到底部
									}
	
									OS_ENTER_CRITICAL();
									mg323_cmd.rx_empty = false ;
						    		if (mg323_cmd.rx_in_last==mg323_cmd.rx_out_last)	{
										mg323_cmd.rx_full = true;  //set buffer full flag
									}
									OS_EXIT_CRITICAL();
	
								}
							}
						}
					}
					else {//may GSM module no respond, need to enquire
						putstring(COM2,"AT^SISI?\r\n");//enquire online?
						prompt("No return %s\tline: %d.\r\n",__FILE__, __LINE__);
						mg323_cmd.rx_time = 0 ;//make it timeout and end this while
						putstring(COM2,"AT^SICI?\r\n");//enquire IP
					}//end of if ( get_respond(rec_str) ) 

				}
				//prompt("Check %s, line:	%d\r\n",__FILE__, __LINE__);
			}

			//Check mg323_cmd.tx_len, if > 0, then send it
			if ( mg323_status.tcp_online \
				&& mg323_cmd.tx_len > 0 \
				&& !mg323_cmd.lock ) { //can send data
	
				OS_ENTER_CRITICAL();
				mg323_cmd.lock = true ;
				OS_EXIT_CRITICAL();
	
				if ( gsm_ask_tcp(mg323_cmd.tx_len) ) {//GSM buffer ready
					if ( gsm_send_tcp(mg323_cmd.tx,mg323_cmd.tx_len) ) {
						//send success
						mg323_cmd.tx_len = 0 ;
						printf("\tOK!\r\n");
					}
					else {
						printf("\r\nGSM send TCP data error.\t");
						prompt("Check %s, line:	%d\r\n",__FILE__, __LINE__);
					}
				}
				else {//may GSM module no respond, need to enquire
					putstring(COM2,"AT^SISI?\r\n");//enquire online or not
					prompt("Can not send data %s\tline: %d.\r\n",__FILE__, __LINE__);
					putstring(COM2,"AT^SICI?\r\n");//enquire IP
				}
	
				OS_ENTER_CRITICAL();
				mg323_cmd.lock = false ;
				OS_EXIT_CRITICAL();
			}//end of if ( mg323_status.tcp_online ...

		}
		else { //gsm power off
			led_off(OBD_CAN10);
		}

		//update relay status even gsm power off
		if ( OSTime - relay_on_time > ring_timer*2*60*1000 ) { //2 mins.
			led_off(OBD_CAN20);//shutdown relay
			ring_timer = 0 ;
			dial_flag = true ;
		}
		else {// < ring_timer*2 mins, open relay
			led_on(OBD_CAN20);
		}

		//update LED status
		if ( mg323_status.tcp_online ) {
			led_on(OBD_CAN10);
			//prompt("TCP online %s.\r\n",mg323_status.local_ip);
		}
		else {
			led_off(OBD_CAN10);
			prompt("!!!  TCP offline  !!!\r\n");
		}

		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}
