#include "main.h"

extern struct ICAR_DEVICE my_icar;
extern struct CAR2SERVER_COMMUNICATION c2s_data ;

extern unsigned char dest_server[];

const unsigned char callback_phone[] = "13828431106";

static unsigned char gsm_string_decode( unsigned char *, unsigned int *);
static void read_tcp_data( unsigned char * );
static void send_tcp_data( void );

void  App_TaskGsm (void *p_arg)
{
	unsigned char rec_str[AT_CMD_LENGTH];
	unsigned int  relay_timer=0;

	(void)p_arg;

	my_icar.mg323.server_ip_port = dest_server;
	my_icar.mg323.power_on = false;

	//Does it need replace by BKP_DR2,3 value?
	my_icar.mg323.power_off_reason = NO_ERR ;
	my_icar.mg323.power_off_timer=0;

	my_icar.mg323.gprs_count = 0 ;
	my_icar.mg323.gprs_ready = false;
	my_icar.mg323.tcp_online = false;
	my_icar.mg323.ask_online = false;
	my_icar.mg323.try_online = 0;
	my_icar.mg323.ip_updating = false;
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
	memset(my_icar.mg323.ip_old, 0x0, IP_LEN-1);

	uart2_init( );

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//1,需增加，长时间不能连线，复位GSM模块
//2,Uart2 无反馈时，超时，重置状态 online ...
	while ( 1 ) {

		if ( my_icar.mg323.ask_power ) {//ask power on the GSM module
			if ( !my_icar.mg323.power_on ) {
				my_icar.mg323.err_no = gsm_power_on() ;
				if ( my_icar.mg323.err_no == 0 ) {
					my_icar.mg323.power_on = true;
					my_icar.mg323.at_timer = OSTime ;

					//check gprs network also
					if ( !my_icar.mg323.gprs_ready ) {
						my_icar.mg323.err_no = gsm_check_gprs( );
						if ( my_icar.mg323.err_no == 0 ) {
							my_icar.mg323.gprs_ready = true;
						}
						else { //error, maybe no gprs network or APN error
							prompt("Reg gprs failure:%d will try later.\r\n",my_icar.mg323.err_no);
							my_icar.mg323.gprs_ready = false;
							my_icar.mg323.tcp_online = false ;
						}
					}
				}
				else { //error, maybe no GSM network
					prompt("GSM power on failure:%d will try later.\r\n",my_icar.mg323.err_no);
					OSTimeDlyHMSM(0, 0, 3, 0);
					my_icar.mg323.gprs_ready = false;
					my_icar.mg323.tcp_online = false ;
					my_icar.mg323.power_on = false;
					if ( my_icar.mg323.err_no == 8 ) {
						my_icar.mg323.power_off_reason = SIM_CARD_ERR ;
					}
					else {
						my_icar.mg323.power_off_reason = POWER_ON_FAILURE ;
					}
					my_icar.mg323.power_off_timer=RTC_GetCounter( );
				    BKP_WriteBackupRegister(BKP_DR2, my_icar.mg323.power_off_timer);
				    BKP_WriteBackupRegister(BKP_DR3, my_icar.mg323.power_off_reason);
					gsm_pwr_off( );
				}
			}
		}
		else {
			;//TBD: power off to save power
		}

		if ( my_icar.mg323.power_on ) {//below action need power on

			//should receive feedback < 10 seconds, because we enquire every 4 sec.
			if ( OSTime - my_icar.mg323.at_timer > 10*AT_TIMEOUT ) {
				//GSM module no respond, reset it
				prompt("\r\nGSM Module no respond, will be reset...%s\tline: %d\r\n",\
						__FILE__, __LINE__);
				my_icar.mg323.gprs_ready = false;
				my_icar.mg323.tcp_online = false ;
				my_icar.mg323.power_on = false;

				my_icar.mg323.power_off_reason = NO_RESPOND ;
				my_icar.mg323.power_off_timer=RTC_GetCounter( );
			    BKP_WriteBackupRegister(BKP_DR2, my_icar.mg323.power_off_timer);
			    BKP_WriteBackupRegister(BKP_DR3, my_icar.mg323.power_off_reason);
				gsm_pwr_off( );
			}

			//Check need online or not?
			if ( my_icar.mg323.ask_online ) {

				if ( my_icar.mg323.gprs_ready ) {
					//reset counter
					my_icar.mg323.gprs_count = 0 ;

					if ( !my_icar.mg323.tcp_online ) { //no online
						prompt("IP: %s\r\n",my_icar.mg323.ip_local);
						memset(my_icar.mg323.ip_old, 0x0, IP_LEN-1);//update IP to server when online
						//send online command
						putstring(COM2,"AT^SISO=0\r\n");
						//will be return ^SISW: 0,1,1xxx
						//confirm this return in later
						c2s_data.check_timer = 0 ;//need check GSM TCP buffer
						my_icar.mg323.try_online++;
						prompt("Try %d to online...\r\n",my_icar.mg323.try_online);
	
						if ( my_icar.stm32_rtc.update_count > 1 ) {//waiting server return
							;//no need send again
						}
						else {
							my_icar.stm32_rtc.update_timer = 0 ;//need run gsm_send_time
						}

						if ( my_icar.mg323.try_online > MAX_ONLINE_TRY ) {//failure
							my_icar.mg323.gprs_ready = false;
							my_icar.mg323.tcp_online = false ;
							my_icar.mg323.power_on = false;

							my_icar.mg323.power_off_reason = TRY_ONLINE ;
							my_icar.mg323.power_off_timer=RTC_GetCounter( );
						    BKP_WriteBackupRegister(BKP_DR2, my_icar.mg323.power_off_timer);
						    BKP_WriteBackupRegister(BKP_DR3, my_icar.mg323.power_off_reason);
							gsm_pwr_off( );
							prompt("Try %d to online, still failure, \
									reboot GSM module.\r\n",my_icar.mg323.try_online);
							//will be auto power on because ask_power is true
							my_icar.mg323.gprs_count = 0 ;
							OSTimeDlyHMSM(0, 0, 1, 0);
						}
					}
				}
				else { //GPRS network no ready
					putstring(COM2, "AT+CGREG?\r\n");
					my_icar.mg323.gprs_count++;
					//wait... timeout => restart
					if ( my_icar.mg323.gprs_count > 180 ) {//about 180s
							my_icar.mg323.gprs_ready = false;
							my_icar.mg323.tcp_online = false ;
							my_icar.mg323.power_on = false;

							my_icar.mg323.power_off_reason = NO_GPRS ;
							my_icar.mg323.power_off_timer=RTC_GetCounter( );
						    BKP_WriteBackupRegister(BKP_DR2, my_icar.mg323.power_off_timer);
						    BKP_WriteBackupRegister(BKP_DR3, my_icar.mg323.power_off_reason);
							gsm_pwr_off( );
							prompt("Find GPRS network timeout! check %s: %d\r\n",\
								__FILE__, __LINE__);
							//will be auto power on because ask_power is true
							my_icar.mg323.try_online = 0 ;
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
					//send below when SISI return error
					//putstring(COM2,"AT^SICI?\r\n");
					//putstring(COM2, "AT+CGREG?\r\n");
				}
			}
			else {
				putstring(COM2,"AT+CSQ\r\n");//Signal
			}

			//waiting GSM respond
			OSTimeDlyHMSM(0, 0, 0, 20);

			//if need dial
			if ( my_icar.mg323.need_dial && !my_icar.mg323.voice_confirm) {

				if ( OSTime - my_icar.mg323.dial_timer > RE_DIAL_PERIOD ) {//re-dial after 2 mins
					prompt("Call my phone for confirm...");
					my_icar.mg323.dial_timer = OSTime ;
					putstring(COM2,"ATD");
					putstring(COM2,(unsigned char *)callback_phone);
					putstring(COM2,";\r\n");
				}
			}

			//Check GSM output string
			while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
				//reset timer here
				my_icar.mg323.at_timer = OSTime ;				

				memset(rec_str, 0x0, AT_CMD_LENGTH);
				if ( get_respond(rec_str) ) {
					;//prompt("Rec:%s\r\n",rec_str);

					gsm_string_decode( rec_str ,&relay_timer );
				}
			}//end of Check GSM output string


			//Check mg323 has data or not
			if ( (c2s_data.check_timer == 0) \
				|| ((OSTime-c2s_data.check_timer)>TCP_CHECK_PERIOD) \
				|| (!my_icar.mg323.rx_empty && my_icar.mg323.tcp_online)) { //has data

				read_tcp_data( rec_str );
				c2s_data.check_timer = OSTime ;//Update timer
			}

			//Check c2s_data.tx_len, if > 0, then send it
			if ( my_icar.mg323.tcp_online \
				&& c2s_data.tx_len > 0 ) { //can send data
				//&& !c2s_data.tx_lock ) { //can send data

				if ( (c2s_data.tx_len > GSM_BUF_LENGTH/2) \
					|| c2s_data.tx_timer == 0 \
					|| (OSTime-c2s_data.tx_timer) > TCP_SEND_PERIOD) {

					send_tcp_data( );//will update c2s_data.tx_timer if send success
				}
				else {//no need send immediately, send later
					;//prompt("Send delay, tx_len: %d Time: %d\r\n",\
						//c2s_data.tx_len,OSTime-c2s_data.tx_timer);
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
			prompt("!!!  TCP offline  !!!\r\n");
		}

		//1, release CPU
		//2, GSM module can't respond if enquire too fast
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}

static void read_tcp_data( unsigned char *buf )
{
	static unsigned char gsm_tcp_len=0, buf_len=0 ;
	static unsigned int i ;
#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    static OS_CPU_SR  cpu_sr = 0;
#endif

	//get the buffer data
	putstring(COM2,"AT^SISR=0,");
	snprintf((char *)buf,AT_CMD_LENGTH-1,"%d\r\n",16);
	//不知为何增大会出问题？
	//each time can't get > AT_CMD_LENGTH, else overflow
	putstring(COM2,buf);

	//data will be sent out after ^SISR: 0,xx
	memset(buf, 0x0, AT_CMD_LENGTH);

	c2s_data.rx_timer = OSTime ;

	while ( !strstr((char *)buf,"^SISR: 0,") \
			&& !c2s_data.rx_full \
			&& (OSTime - c2s_data.rx_timer) < 5*AT_TIMEOUT ) {

		while ( my_icar.stm32_u2_rx.empty && \
			(OSTime - c2s_data.rx_timer) < AT_TIMEOUT ) {//no data...
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
				//prompt("buf:%s, len: %d\r\n",buf,buf_len);
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
					//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
				}
				else {
					//push data to c2s_data.rx
					//prompt("Push to rx:\t");

					for ( i = 0 ; i < gsm_tcp_len; i++ ) {
						while ( my_icar.stm32_u2_rx.empty && \
							(OSTime - c2s_data.rx_timer) < AT_TIMEOUT ) {//no data...
							OSTimeDlyHMSM(0, 0,	0, 100);
							prompt("Line: %d, gsm_tcp_len: %d, i: %d\r\n",\
								__LINE__,gsm_tcp_len,i);
						}

						if ( my_icar.stm32_u2_rx.empty ) {//
							prompt("Rec buf: %s\r\n",buf);
							prompt("Wait TCP data timeout! check %s: %d\r\n",\
								__FILE__,__LINE__);
							i =  gsm_tcp_len; //end this loop
						}
						else {
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
							}
							OS_EXIT_CRITICAL();
						}	
					}
				}
			}
		}
		else {//may GSM module no respond, need to enquire
			putstring(COM2,"AT^SISI?\r\n");//enquire online?
			prompt("No return %s\tline: %d.\r\n",__FILE__, __LINE__);
			c2s_data.rx_timer = 0 ;//make it timeout and end this while
			//putstring(COM2,"AT^SICI?\r\n");//enquire IP
		}//end of if ( get_respond(buf) ) 

	}

}

static void send_tcp_data( )
{

#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif
	
	if ( !c2s_data.tx_lock ) {
		OS_ENTER_CRITICAL();
		c2s_data.tx_lock = true ;
		OS_EXIT_CRITICAL();
	
		if ( gsm_ask_tcp(c2s_data.tx_len) ) {//GSM buffer ready
			//prompt("Wil send data %s\tline: %d.\r\n",__FILE__, __LINE__);
			if ( gsm_send_tcp(c2s_data.tx,c2s_data.tx_len) ) {
				//Check: ^SISW: 0,1 
				prompt("Send %d bytes OK!\r\n",c2s_data.tx_len);
				c2s_data.tx_len = 0 ;
				c2s_data.tx_timer = OSTime ;//update timer
			}
			else {
				printf("\r\nGSM send TCP data error.\t");
				prompt("Check %s, line:	%d\r\n",__FILE__, __LINE__);
			}
		}
		else {//may GSM module no respond, need to enquire
			putstring(COM2,"AT^SISI?\r\n");//enquire online or not
			prompt("Can not send data %s\tline: %d.\r\n",__FILE__, __LINE__);
		}
	
		OS_ENTER_CRITICAL();
		c2s_data.tx_lock = false ;
		OS_EXIT_CRITICAL();
	}
}

//return 0:	ok
//return 1: unknow string
static unsigned char gsm_string_decode( unsigned char *buf , unsigned int *timer )
{
	static unsigned char i ;

	//found GSM auto report
	if (strstr((char *)buf,"network is unavailable")) {
		my_icar.mg323.tcp_online = false ;
		return 0;
	}

	//^SIS: 0, 0, 48, Remote Peer has closed the connection
	if (strstr((char *)buf,"^SIS: 0, 0, 48")) {

		putstring(COM2,"AT^SICI?\r\n");//enquire for further info
		prompt("MG323 report: %s\r\n",buf);
		OSTimeDlyHMSM(0, 0, 0, 500);
		putstring(COM2, "AT+CGREG?\r\n");

		if ( my_icar.mg323.tcp_online  ) {
			//previous status is online, now is offline, close connect.
			prompt("Close connection@ %d.\r\n",__LINE__);
			my_icar.mg323.tcp_online = false ;
			OSTimeDlyHMSM(0, 0, 0, 500);
			putstring(COM2,"AT^SISC=0\r\n");
			my_icar.mg323.try_online = 0 ;

			//maybe some problem
			OSTimeDlyHMSM(0, 0, 0, 500);
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
		}
		else {
			putstring(COM2,"AT^SICI?\r\n");//enquire for further info
			prompt("SISI return: %s, check %s: %d\r\n",\
							buf,__FILE__, __LINE__);
			OSTimeDlyHMSM(0, 0, 0, 500);
			putstring(COM2, "AT+CGREG?\r\n");

			if ( my_icar.mg323.tcp_online  ) {
				//previous status is online, now is offline, close connect.
				my_icar.mg323.tcp_online = false ;
				prompt("Close connection@ %d.\r\n",__LINE__);
				OSTimeDlyHMSM(0, 0, 0, 500);
				putstring(COM2,"AT^SISC=0\r\n");//close channel 0
				my_icar.mg323.try_online = 0 ;

				//maybe some problem
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
		}
		return 0;
	}

	//found IP message, i.e ^SICI: 0,2,0,"10.7.60.209"
	if (strstr((char *)buf,"^SICI: 0,2,")) {
		//prompt("SICI return: %s\r\n",buf);

		switch (buf[11]) {

			case 0x30://0：Down 状态，Internet 连接已经定义但还没连接

				//my_icar.mg323.tcp_online = false ;
				prompt("!!!Connection is down!!! %d\r\n",__LINE__);

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
		//^SISR: 0,0  :no data
		if ( buf[10] > 0x30 ) {//have data
			//prompt("TCP rec data: %s\r\n",buf);
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

	prompt("Unknow report:%s\r\n",buf);
	return 1;
	//add others respond string here
}
