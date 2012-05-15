#include "main.h"

extern struct ICAR_DEVICE my_icar;
extern struct CAR2SERVER_COMMUNICATION c2s_data ;

extern unsigned char dest_server[];

const unsigned char callback_phone[] = "1008611";

static unsigned char gsm_string_decode( unsigned char *, unsigned int *);
static unsigned char read_tcp_data( unsigned char *,unsigned int * );
static unsigned char send_tcp_data( unsigned char *, unsigned int * );

void  app_task_gsm (void *p_arg)
{
	unsigned char rec_str[AT_CMD_LENGTH], err_code;
	unsigned int  relay_timer=0, var_int_data = 0;
#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	(void)p_arg;

	my_icar.mg323.server_ip_port = dest_server;
	my_icar.mg323.power_on = false;

	my_icar.mg323.gprs_count = 0 ;
	my_icar.mg323.gprs_ready = false;
	my_icar.mg323.tcp_online = false;
	my_icar.mg323.ask_online = false;
	my_icar.mg323.try_online_cnt = 1;
	my_icar.mg323.try_online_time = 0;

	my_icar.mg323.apn_index = NULL;
	my_icar.mg323.roam = false;
	my_icar.mg323.cgatt= false;
	my_icar.mg323.rx_empty = true ;
	my_icar.mg323.at_timer = OSTime ;
	my_icar.mg323.ring_count = 0 ;
	my_icar.mg323.dial_timer=0 ;
	my_icar.mg323.need_dial = false ;
	my_icar.mg323.voice_confirm = true ;

	memset(my_icar.mg323.ip_local, 0x0, IP_LEN-1);

	uart2_init( );

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//1,需增加，长时间不能连线，复位GSM模块
//2,Uart2 无反馈时，超时，重置状态 online ...
	while ( 1 ) {

		if ( my_icar.mg323.ask_power ) {//ask power on the GSM module
			if ( !my_icar.mg323.power_on ) {
				err_code = gsm_power_on() ;
				if ( err_code == 0 ) {
					my_icar.mg323.power_on = true;
					my_icar.mg323.at_timer = OSTime ;

					//check gprs network also
					if ( !my_icar.mg323.gprs_ready ) {
						err_code = gsm_check_gprs( );
						if ( err_code == 0 ) {
							my_icar.mg323.gprs_ready = true;
						}
						else { //error, maybe no gprs network or APN error
							prompt("Reg gprs failure:%d will try later.\r\n",err_code);
							my_icar.mg323.gprs_ready = false;
							my_icar.mg323.tcp_online = false ;

							//save to BK reg
							//BKP_DR4, GPRS disconnect time(UTC Time) high
							//BKP_DR5, GPRS disconnect time(UTC Time) low
						    BKP_WriteBackupRegister(BKP_DR4, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
						    BKP_WriteBackupRegister(BKP_DR5, (RTC_GetCounter( ))&0xFFFF);//low
						
							//BKP_DR1, ERR index: 	15~12:MCU reset 
							//						11~8:reverse
							//						7~4:GPRS disconnect reason
							//						3~0:GSM module poweroff reason
							var_int_data = (BKP_ReadBackupRegister(BKP_DR1))&0xFF0F;
							var_int_data = var_int_data | ((err_code<<4)&0xF0) ;
						    BKP_WriteBackupRegister(BKP_DR1, var_int_data);
						}
					}
				}
				else { //error, maybe no GSM network
					prompt("GSM power on failure:%d will try later.\r\n",err_code);
					gsm_pwr_off( (POWEROFF_REASON)err_code );
				}
			}
		}
		else {
			;//TBD: power off to save power
		}

		if ( my_icar.mg323.power_on ) {//below action need power on

			//should receive feedback < 10 seconds, because we enquire every sec.
			if ((OSTime > my_icar.mg323.at_timer) && \
				(OSTime - my_icar.mg323.at_timer > 10*AT_TIMEOUT) ) {
				//GSM module no respond, reset it
				prompt("\r\nGSM Module no respond, will be reset...%s\tline: %d\r\n",\
						__FILE__, __LINE__);

				gsm_pwr_off( NO_RESPOND );
			}

			//Check need online or not?
			if ( my_icar.mg323.ask_online ) {

				if ( my_icar.mg323.gprs_ready ) {
					//reset counter
					my_icar.mg323.gprs_count = 0 ;

					if ( !my_icar.mg323.tcp_online ) { //no online
						//try_online <= 4 when normally
						if ( my_icar.mg323.try_online_cnt%9 == 0 ) {//Try close first
							prompt("will re-init GSM again... try_online: %d\r\n",my_icar.mg323.try_online_cnt);
							my_icar.mg323.gprs_count = 0 ;
							my_icar.mg323.gprs_ready = false;
							my_icar.mg323.tcp_online = false ;
							my_icar.mg323.power_on = false;
						}

						if ( OSTime - my_icar.mg323.try_online_time > 3*OS_TICKS_PER_SEC ) {
							my_icar.mg323.try_online_time = OSTime ;

							//send online command
							putstring(COM2,"AT^SISO=0\r\n");
							//will be return ^SISW: 0,1,1xxx
							//confirm this return in later
	
							c2s_data.check_timer = OSTime ;//Prevent check when offline
							my_icar.need_sn = true ;//need upload SN
	
							my_icar.mg323.try_online_cnt++;
							prompt("Try %d to online...\r\n",my_icar.mg323.try_online_cnt);
						}

						if ( my_icar.mg323.try_online_cnt > MAX_ONLINE_TRY ) {//failure

							prompt("Try %d to online, still failure, \
								reboot GSM module.\r\n",my_icar.mg323.try_online_cnt);
							//will be auto power on because ask_power is true

							//need to double check this logic
							gsm_pwr_off( TRY_ONLINE );
							OSTimeDlyHMSM(0, 0, 0, 500);
						}
					}
				}
				else { //GPRS network no ready
					putstring(COM2, "AT+CGREG?\r\n");
					my_icar.mg323.gprs_count++;
					//wait... timeout => restart
					if ( my_icar.mg323.gprs_count > 60 ) {//about 60s

							gsm_pwr_off( NO_GPRS );
							prompt("Find GPRS network timeout! check %s: %d\r\n",\
								__FILE__, __LINE__);
							//will be auto power on because ask_power is true

							OSTimeDlyHMSM(0, 0, 1, 0);
					}
				}
			}//end of if ( my_icar.mg323.ask_online )

			//Send GSM signal and tcp status cmd every 3 sec.
			
			if ( (OSTime/100)%3 == 0 ) {//connection status
				if ( (OSTime/100)%6 == 0 ) {
					putstring(COM2,"AT^SICI?\r\n");
				}
				else {
					putstring(COM2,"AT^SISI?\r\n");
				}
			}
			else {
				putstring(COM2,"AT+CSQ\r\n");//Signal
			}

			//waiting GSM respond
			OSTimeDlyHMSM(0, 0, 0, 100);

			//if need dial
			if ( my_icar.mg323.need_dial && !my_icar.mg323.voice_confirm) {

				if ( OSTime - my_icar.mg323.dial_timer > RE_DIAL_PERIOD ) {//re-dial after 2 mins
					prompt("Call my phone for confirm...");
					my_icar.mg323.dial_timer = OSTime ;
					putstring(COM2,"ATD");
					putstring(COM2,(unsigned char *)callback_phone);
					putstring(COM2,";\r\n");
					c2s_data.rx_timer = OSTime + VOICE_RESPOND_TIMEOUT ;//GPRS stop when voice
					my_icar.mg323.at_timer = OSTime + VOICE_RESPOND_TIMEOUT ;//GPRS stop when voice
				}
			}

			//Check GSM output string
			while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
				//reset timer here
				my_icar.mg323.at_timer = OSTime ;

				memset(rec_str, 0x0, AT_CMD_LENGTH);
				if ( get_respond(rec_str) ) {
					if ( my_icar.debug > 3) {
						prompt("Rec:%s\r\n",rec_str);
					}//here

					err_code = gsm_string_decode( rec_str ,&relay_timer );
				}
			}//end of Check GSM output string

			if ( my_icar.mg323.tcp_online ) { //can send data

				//Receive data
				//Check mg323 has data or not
				if ( (c2s_data.check_timer == 0) \
					|| ((OSTime-c2s_data.check_timer)>TCP_CHECK_PERIOD) \
					|| (!my_icar.mg323.rx_empty)) { //has data
	
					var_int_data = 0 ;
					if ( read_tcp_data( rec_str, &var_int_data )) {
						prompt("Rec TCP data error! Line: %d\r\n",__LINE__);
					}
					else {
						if ( var_int_data ) {//success rec data
							c2s_data.rx_timer = OSTime ;//reset GSM module if rx timeout
						}
						if ( my_icar.debug ) {
							prompt("Rec %d bytes\r\n",var_int_data);
						}
					}
					c2s_data.check_timer = OSTime ;//Update timer
				}

				//Send data
				//Check c2s_data.tx_len, if > 0, then send it
				if ( c2s_data.tx_sn_len > 0 && my_icar.need_sn ) {
					//prompt("Sending SN...\t");
					if ( !send_tcp_data(c2s_data.tx_sn,&c2s_data.tx_sn_len )){
						//success
						if ( my_icar.need_sn ) {
							my_icar.need_sn-- ;
						}
					}
				}
				else {//Send normal data
					if ( (c2s_data.tx_len > GSM_BUF_LENGTH/2) \
						|| c2s_data.tx_timer == 0 \
						|| (OSTime-c2s_data.tx_timer) > TCP_SEND_PERIOD) {
	
						if ( !c2s_data.tx_lock && !my_icar.need_sn && c2s_data.tx_len ) {
							OS_ENTER_CRITICAL();
							c2s_data.tx_lock = true ;
							OS_EXIT_CRITICAL();
	
							//will update c2s_data.tx_timer if send success						
							send_tcp_data(c2s_data.tx, &c2s_data.tx_len );
	
							OS_ENTER_CRITICAL();
							c2s_data.tx_lock = false ;
							OS_EXIT_CRITICAL();
						}
					}
					else {//no need send immediately, send later
						;//prompt("Send delay, tx_len: %d Time: %d\r\n",\
							//c2s_data.tx_len,OSTime-c2s_data.tx_timer);
					}
				}

				//Reset GSM module if rx timeout
				if ((OSTime > c2s_data.rx_timer) && \
					(OSTime - c2s_data.rx_timer > TCP_SEND_PERIOD*2) ) {
					prompt("Rec TCP timeout, reset GSM module! Line: %d\r\n",__LINE__);
					prompt("OSTime:%d - rx_timer:%d = %d\r\n",OSTime,c2s_data.rx_timer,OSTime-c2s_data.rx_timer);
					gprs_disconnect( RX_TIMEOUT );
				}
			}
		}
		else { //gsm power off
			led_off(ONLINE_LED);
		}

		//update relay status even gsm power off
		if ( OSTime - relay_timer > my_icar.mg323.ring_count*RELAY_ON_PERIOD ) { //10 mins.
			led_off(RELAY_LED);//shutdown relay
			my_icar.mg323.ring_count = 0 ;
			my_icar.mg323.need_dial = true ;
		}
		else {// < my_icar.mg323.ring_count*10 mins, open relay
			led_on(RELAY_LED);
		}

		//update LED status
		if ( my_icar.mg323.tcp_online ) {
			led_on(ONLINE_LED);
			//prompt("TCP online %s.\r\n",my_icar.mg323.ip_local);
		}
		else {
			led_off(ONLINE_LED);
			if ( my_icar.mg323.ask_online ) {
				prompt("!!!  TCP offline  !!!\r\n");
			}
		}

		//1, release CPU
		//2, GSM module can't respond if enquire too fast
		OSTimeDlyHMSM(0, 0, 0, 950);
	}
}

//0: ok
//others: failure
static unsigned char read_tcp_data( unsigned char *buf, unsigned int *rec_len )
{
	static unsigned char i, gsm_tcp_len, buf_len, retry;
	static unsigned int data_timer ; //prevent rx data timeout
#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    static OS_CPU_SR  cpu_sr = 0;
#endif

	gsm_tcp_len=0, buf_len=0 , retry = 0;

	//GPRS max. speed: 115kbit/s = 11KB/s, but GSM_BUF_LENGTH is 1088
	//retry: 1088/(AT_CMD_LENGTH-16) = 1*1088/48 = 22.6
	//so if > 22, release cpu, let taskmanager to handle receive buf

	while ( retry < 22 ) {
		//get the buffer data
		putstring(COM2,"AT^SISR=0,");
		snprintf((char *)buf,AT_CMD_LENGTH-1,"%d\r\n",AT_CMD_LENGTH-16);
		//each time can't get > AT_CMD_LENGTH, else overflow
		putstring(COM2,buf);
	
		//data will be sent out after ^SISR: 0,xx
		memset(buf, 0x0, AT_CMD_LENGTH);
	
		data_timer = OSTime ;
	
		while ( !strstr((char *)buf,"^SISR: 0,") \
				&& !c2s_data.rx_full \
				&& (OSTime - data_timer) < 5*AT_TIMEOUT ) {
	
			while ( my_icar.stm32_u2_rx.empty && \
				(OSTime - data_timer) < AT_TIMEOUT ) {//no data...
				OSTimeDlyHMSM(0, 0,	0, 100);
			}
	
			memset(buf, 0x0, AT_CMD_LENGTH);
			if ( get_respond(buf) ) {
				gsm_tcp_len = 0 ;//set default value
				if ( strstr((char *)buf,"^SISR: 0,") ) {//found
					//^SISR: 0,10 收到数据10个 or 
					//^SISR: 0,0  no data, update my_icar.mg323.rx_empty
					//search first \r\n
					buf_len=(strstr((char *)buf,"\r\n")-(char *)buf);
					//提取 buffer 长度
					switch (buf_len) {
	
					case 10://^SISR: 0,1
						gsm_tcp_len = buf[9] - 0x30 ;
						break;
	
					case 11://^SISR: 0,12
						gsm_tcp_len = (buf[10] - 0x30)+\
										 ((buf[9] - 0x30)*10) ;
						break;
	
					case 12://^SISR: 0,123
						gsm_tcp_len = (buf[11] - 0x30)+\
										((buf[10] - 0x30)*10)+\
										((buf[9]  - 0x30)*100) ;
						break;
	
					case 13://^SISR: 0,1234
						gsm_tcp_len = (buf[12] - 0x30)+\
										((buf[11] - 0x30)*10)+\
										((buf[10] - 0x30)*100)+\
										((buf[9]  - 0x30)*1000) ;
						break;
	
					default:
						prompt("Buf: %s Illegal length %d, check %s: %d\r\n",\
								buf,buf_len,__FILE__, __LINE__);
						break;
	
					}//end of switch
	
					if ( gsm_tcp_len == 0 ) {//no data
						my_icar.mg323.rx_empty = true ;
						return 0;
						//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
					}
					else {
						if ( gsm_tcp_len == AT_CMD_LENGTH-16 ) {//Important, need same as above:
							//snprintf((char *)buf,AT_CMD_LENGTH-1,"%d\r\n",AT_CMD_LENGTH-16);
							//still have data in module
							retry++ ;
						}
						else { //no more data
							retry = 0xFF ;
						}
						//push data to c2s_data.rx
						//prompt("Push to rx:\t");
						//prompt("retry: %d gsm_tcp_len: %d\r\n",retry,gsm_tcp_len);
						*rec_len = *rec_len + gsm_tcp_len ;
						for ( i = 0 ; i < gsm_tcp_len; i++ ) {
							while ( my_icar.stm32_u2_rx.empty && \
								(OSTime - data_timer) < AT_TIMEOUT ) {//no data...
								OSTimeDlyHMSM(0, 0,	0, 100);
								prompt("Line: %d, gsm_tcp_len: %d, i: %d\r\n",\
									__LINE__,gsm_tcp_len,i);
							}
	
							if ( my_icar.stm32_u2_rx.empty ) {//Still empty, no data
								prompt("Rec buf: %s\r\n",buf);
								prompt("Wait TCP data timeout! check %s: %d\r\n",\
									__FILE__,__LINE__);
								return 1;
							}
							else {//have data
								*c2s_data.rx_in_last = getbyte( COM2 );
		
								//printf("%02X ",*c2s_data.rx_in_last);
		
								c2s_data.rx_in_last++;
							   	if (c2s_data.rx_in_last==c2s_data.rx+GSM_BUF_LENGTH) {
									c2s_data.rx_in_last=c2s_data.rx;//地址到顶部回到底部
								}
		
								OS_ENTER_CRITICAL();
								c2s_data.rx_empty = false ;
					    		if (c2s_data.rx_in_last==c2s_data.rx_out_last)	{
									c2s_data.rx_full = true;  //set buffer full flag
									i = gsm_tcp_len ;
									retry = 0xFF ;
								}
								OS_EXIT_CRITICAL();
							}	
						}
					}
				}
	
				if ( strstr((char *)buf,"ERROR") ) {//Module report error
					prompt("Rec err! %s: %d.\r\n",__FILE__, __LINE__);
					my_icar.mg323.rx_empty = true ;
					return 1 ;
				}
			}
			else {//may GSM module no respond, need to enquire
				OSTimeDlyHMSM(0, 0, 0, 500);
				putstring(COM2,"AT^SISI?\r\n");//enquire online?
				prompt("No return %s: %d.\r\n",__FILE__, __LINE__);
				OSTimeDlyHMSM(0, 0, 0, 500);
				return 1 ;
			}//end of if ( get_respond(buf) ) 
		}
	}//end while ( retry < 20 )
	return 0 ;
}

//0: ok
//others: err
static unsigned char send_tcp_data(unsigned char *buffer, unsigned int *buf_len )
{
	if ( gsm_ask_tcp(*buf_len) ) {//GSM buffer ready
		//prompt("Wil send data %s\tline: %d.\r\n",__FILE__, __LINE__);
		if ( gsm_send_tcp(buffer,*buf_len) ) {
			//Check: ^SISW: 0,1 
			if ( my_icar.debug ) {
				prompt("Send %d bytes OK!\r\n",*buf_len);
			}

			*buf_len = 0 ;
			c2s_data.check_timer = 0;//need check ASAP also
			c2s_data.tx_timer = OSTime ;//update timer
			return 0 ;
		}
		else {
			printf("GSM send err, len:%d\t",*buf_len);
			prompt("Check %s, line:	%d\r\n",__FILE__, __LINE__);
			return 1 ;
		}
	}
	else {//may GSM module no respond, need to enquire
		OSTimeDlyHMSM(0, 0, 0, 500);
		putstring(COM2,"AT^SISI?\r\n");//enquire online or not
		prompt("Can not send data %s\tline: %d.\r\n",__FILE__, __LINE__);
		OSTimeDlyHMSM(0, 0, 0, 500);
		return 2 ;
	}
}

//return 0:	ok
//return 1: unknow string
static unsigned char gsm_string_decode( unsigned char *buf , unsigned int *timer )
{
	static u16 i ;

	//found GSM module reboot message
	if (strstr((char *)buf,"SYSSTART")) {
		prompt("MG323 report: %s\r\n",buf);
		my_icar.mg323.try_online_cnt = 1;
		my_icar.mg323.gprs_count = 0 ;
		my_icar.mg323.gprs_ready = false;
		my_icar.mg323.tcp_online = false ;
		my_icar.mg323.power_on = false;
	
		//save to BK reg
		//BKP_DR2, GSM Module power off time(UTC Time) high
		//BKP_DR3, GSM Module power off time(UTC Time) low
	    BKP_WriteBackupRegister(BKP_DR2, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
	    BKP_WriteBackupRegister(BKP_DR3, (RTC_GetCounter( ))&0xFFFF);//low
	
		//BKP_DR1, ERR index: 	15~12:MCU reset 
		//						11~8:reverse
		//						7~4:GPRS disconnect reason
		//						3~0:GSM module poweroff reason
		i = (BKP_ReadBackupRegister(BKP_DR1))&0xFFF0;
		i = i | MODULE_REBOOT ;
	    BKP_WriteBackupRegister(BKP_DR1, i);

		return 0;
	}

	//found GSM auto report
	if (strstr((char *)buf,"network is unavailable")) {
		my_icar.mg323.tcp_online = false ;
		return 0;
	}

	//^SIS: 0, 0, 48, Remote Peer has closed the connection
	if (strstr((char *)buf,"^SIS: 0, 0, 48")) {

		if ( my_icar.mg323.tcp_online  ) {
			//previous status is online, now is offline, close connect.
			prompt("Rec:%s\r\n",buf);
			gprs_disconnect(PEER_CLOSED);
		}
		else {
			putstring(COM2,"AT^SICI?\r\n");//enquire for further info
			prompt("MG323 report: %s\r\n",buf);
			OSTimeDlyHMSM(0, 0, 0, 500);
			putstring(COM2, "AT+CGREG?\r\n");
		}
		return 0;
	}

	//found GPRS status  respond
	if (strstr((char *)buf,"+CGREG: 0,")) {
		prompt("Rec:%s\r\n",buf);

		switch (buf[10]) {

			case 0x31://CGREG: 0,1\r\n
				my_icar.mg323.gprs_ready = true ;
				my_icar.mg323.roam = false;
				break;

			case 0x35://CGREG: 0,5\r\n
				my_icar.mg323.gprs_ready = true ;
				my_icar.mg323.roam = true;
				break;

			default:
				my_icar.mg323.gprs_ready = false ;
				//enquire GPRS attach status:
				putstring(COM2, "AT+CGATT?\r\n");
				prompt("Unknow GPRS status: %s, check %s: %d\r\n",\
							buf,__FILE__, __LINE__);
				break;

		}//end of switch
		return 0;
	}
			
	//found GPRS att respond
	if (strstr((char *)buf,"+CGATT")) {
		prompt("Rec:%s\r\n",buf);
		if ( buf[8] == '1' ) {
			my_icar.mg323.cgatt= true; }
		else {
			my_icar.mg323.cgatt= false;
			prompt("GPRS not attached!\t%s, check %s: %d\r\n",\
						buf,__FILE__, __LINE__);
		}

		return 0;
	}

	//found signal respond
	if (strstr((char *)buf,"+CSQ:")) {
        if(buf[7] == 0x2c)
        {
            my_icar.mg323.signal = buf[6] - 0x30; 
        }
        else
        {
            buf[6] = buf[6] - 0x30;
            buf[7] = buf[7] - 0x30;
            my_icar.mg323.signal = buf[6]*10 + buf[7];   
        }
		prompt("Signal:%02d\r\n",my_icar.mg323.signal);
		return 0;
	}

	// Found Internet profile status
	// 2: allocated
	// 3: connecting
	// 4: up
	// 5: closing : dest_svr close
	// 6: down
	if (strstr((char *)buf,"SISI: 0,")) {
		//^SISI: 0,5,0,0,0,0
		if ( buf[9]== 0x34 ) {
			my_icar.mg323.tcp_online = true ;
			my_icar.mg323.try_online_cnt = 1 ;
		}
		else {
			prompt("SISI return: %s, check %s: %d\r\n",\
							buf,__FILE__, __LINE__);
			if ( my_icar.mg323.tcp_online  ) {
				//previous status is online, now is offline, close connect.
				gprs_disconnect(PROFILE_NO_UP);
			}
		}
		return 0;
	}

	//found IP message, i.e ^SICI: 0,2,0,"10.7.60.209"
	if (strstr((char *)buf,"^SICI: 0,2,")) {

		switch (buf[11]) {

			case 0x30://0：Down 状态，Internet 连接已经定义但还没连接

				prompt("SICI return: %s\r\n",buf);
				prompt("!!!Connection is down!!! %d\r\n",__LINE__);
				if ( my_icar.mg323.tcp_online  ) {
					//previous status is online, now is offline, close connect.
					gprs_disconnect(CONNECTION_DOWN);
				}
				break;

			case 0x31://1：连接状态，服务已经打开，Internet 连接已经初始化
				i = 0 ;//222.222.222.222
				memset(my_icar.mg323.ip_local, 0x0, IP_LEN-1);
				while ( (buf[i+14] != 0x22) && i < 15) { //"
					my_icar.mg323.ip_local[i] = buf[i+14];
					i++ ;
				}
				
				break;

			case 0x32://2：Up 状态，Internet 连接已经建立，正使用一种或多种服务
				i = 0 ;//222.222.222.222
				memset(my_icar.mg323.ip_local, 0x0, IP_LEN-1);
				while ( (buf[i+14] != 0x22) && i < 15) { //"
					my_icar.mg323.ip_local[i] = buf[i+14];
					i++ ;
				}

				break;

			case 0x33://3：限制状态，Internet 连接已经建立，但暂时没有网络覆盖
				prompt("!!!Connection is limit, no GSM network!!! %d\r\n",__LINE__);
				break;

			case 0x34://4：关闭状态，Internet 连接已经断开
				prompt("!!!Connection is close!!! %d\r\n",__LINE__);
				break;

			default:
				my_icar.mg323.gprs_ready = false ;
				prompt("Unknow SICI return: %s, check %s: %d\r\n",\
							buf,__FILE__, __LINE__);
				break;

		}//end of switch
		return 0;
	}

	//GSM TCP rec data: ^SISR: 0,0
	if (strstr((char *)buf,"^SISR: 0,")) {//Rec tcp data
		//prompt("TCP rec data: %s\r\n",buf);
		//^SISR: 0,0  :no data
		if ( buf[10] > 0x30 ) {//have data
			my_icar.mg323.rx_empty = false ;
		}
		else {
			my_icar.mg323.rx_empty = true ;
			//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
		}
		return 0;
	}

	//found ^SISW: 0,1,1460 or Rec:^SISW: 0,1,1360
	if (strstr((char *)buf,"^SISW: 0,1,1")) {
		//need to double check
		prompt("TCP online: %s\r\n",buf);
		my_icar.mg323.tcp_online = true ;
		c2s_data.rx_timer = OSTime ;//update timer
		//ask the IP, return:^SICI: 0,2,1,"10.156.174.147"
		putstring(COM2,"AT^SICI?\r\n");
		return 0;
	}

	//found voice call
	if (strstr((char *)buf,"RING")) {
		putstring(COM2,"ATH\r\n");
		*timer = OSTime;
		my_icar.mg323.ring_count++;
		my_icar.mg323.need_dial = false ;
		my_icar.mg323.voice_confirm = false ;
		putstring(COM2,"ATH\r\n");
		prompt("Receive %d time call.\r\n",my_icar.mg323.ring_count);
		return 0;
	}

	//found call confirm
	if (strstr((char *)buf,"BUSY")) {//call success
		my_icar.mg323.need_dial = false ;
		my_icar.mg323.voice_confirm = true ;
		my_icar.mg323.dial_timer = 0 ; //prepare for next call
		prompt("Confirmed by voice.\r\n");
		return 0;
	}

	if ( cmpmem(buf,"OK\r\n",4 )) {
		return 0;
	}

	if ( cmpmem(buf,"\r\n",2 )) {
		return 0;
	}

	if ( cmpmem(buf,"AT+CSQ\r\n",6 ) || cmpmem(buf,"AT^SICI?\r\n",8 ) \
		|| cmpmem(buf,"AT^SISI?\r\n",8 ) ) {//Module had been reboot, echo command
		prompt("MG323 reboot, echo CMD: %s\r\n",buf);
		my_icar.mg323.try_online_cnt = 1;
		my_icar.mg323.gprs_count = 0 ;
		my_icar.mg323.gprs_ready = false;
		my_icar.mg323.tcp_online = false ;
		my_icar.mg323.power_on = false;
	
		//save to BK reg
		//BKP_DR2, GSM Module power off time(UTC Time) high
		//BKP_DR3, GSM Module power off time(UTC Time) low
	    BKP_WriteBackupRegister(BKP_DR2, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
	    BKP_WriteBackupRegister(BKP_DR3, (RTC_GetCounter( ))&0xFFFF);//low
	
		//BKP_DR1, ERR index: 	15~12:MCU reset 
		//						11~8:reverse
		//						7~4:GPRS disconnect reason
		//						3~0:GSM module poweroff reason
		i = (BKP_ReadBackupRegister(BKP_DR1))&0xFFF0;
		i = i | MODULE_REBOOT ;
	    BKP_WriteBackupRegister(BKP_DR1, i);

		prompt("Check %s: %d\r\n",	__FILE__,__LINE__);
		return 2;
	}

	prompt("Unknow report:%s\r\n",buf);
	return 1;
	//add others respond string here
}
