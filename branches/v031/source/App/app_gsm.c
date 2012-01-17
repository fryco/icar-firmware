#include "main.h"

struct GSM_STATUS mg323_status ;
struct GSM_COMMAND mg323_cmd ;// tx 缓冲处理待改进
extern struct UART_RX u2_rx_buf;
extern unsigned char dest_server[];

const unsigned char callback_phone[] = "13828431106";

static unsigned char gsm_string_decode( unsigned char *, unsigned int *);
static void read_tcp_data( unsigned char * );
static void send_tcp_data( void );

void  App_TaskGsm (void *p_arg)
{
	unsigned char rec_str[AT_CMD_LENGTH];
	unsigned int  relay_timer=0;

#if OS_CRITICAL_METHOD == 3    /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	(void)p_arg;

	mg323_status.server_ip_port = dest_server;
	mg323_status.power_on = false;
	mg323_status.gprs_count = 0 ;
	mg323_status.gprs_ready = false;
	mg323_status.tcp_online = false;
	mg323_status.ask_online = false;
	mg323_status.try_online = 0;
	mg323_status.apn_index = NULL;
	mg323_status.roam = false;
	mg323_status.cgatt= false;
	mg323_status.rx_empty = true ;
	mg323_status.at_time = OSTime ;
	mg323_status.ring_count = 0 ;
	mg323_status.dial_timer=0 ;
	mg323_status.need_dial = false ;
	mg323_status.voice_confirm = true ;

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
					mg323_status.at_time = OSTime ;

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
			if ( OSTime - mg323_status.at_time > 10*AT_TIMEOUT ) {
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

				if ( mg323_status.gprs_ready ) {
					//reset counter
					mg323_status.gprs_count = 0 ;

					if ( !mg323_status.tcp_online ) { //no online
						//send online command
						putstring(COM2,"AT^SISO=0\r\n");
						//will be return ^SISW: 0,1,1xxx
						//confirm this return in later
						mg323_status.try_online++;
						prompt("Try %d to online...\r\n",mg323_status.try_online);
						if ( mg323_status.try_online > 15 ) {//failure > 15
							mg323_status.gprs_ready = false;
							mg323_status.tcp_online = false ;
							mg323_status.power_on = false;
							gsm_pwr_off( );
							prompt("Try %d to online, still failure, \
									reboot GSM module.\r\n",mg323_status.try_online);
							//will be auto power on because ask_power is true
							mg323_status.gprs_count = 0 ;
							OSTimeDlyHMSM(0, 0, 1, 0);
						}
					}
				}
				else { //GPRS network no ready
					putstring(COM2, "AT+CGREG?\r\n");
					mg323_status.gprs_count++;
					//wait... timeout => restart
					if ( mg323_status.gprs_count > 120 ) {//about 120s
							mg323_status.gprs_ready = false;
							mg323_status.tcp_online = false ;
							mg323_status.power_on = false;
							gsm_pwr_off( );
							prompt("Find GPRS network timeout! check %s: %d\r\n",\
								__FILE__, __LINE__);
							//will be auto power on because ask_power is true
							mg323_status.try_online = 0 ;
							OSTimeDlyHMSM(0, 0, 1, 0);
					}
				}
			}//end of if ( mg323_status.ask_online )

			//Send GSM signal and tcp status cmd every 3 sec.
			
			putstring(COM2,"AT+CSQ\r\n");//Signal
			if ( (OSTime/1000)%3 == 0 ) {// GPRS
				putstring(COM2, "AT+CGREG?\r\n");
				putstring(COM2, "AT+CGATT?\r\n");
			}
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
			if ( mg323_status.need_dial && !mg323_status.voice_confirm) {

				if ( OSTime - mg323_status.dial_timer > 2*60*1000 ) {//re-dial after 2 mins
					prompt("Call my phone for confirm...");
					mg323_status.dial_timer = OSTime ;
					putstring(COM2,"ATD");
					putstring(COM2,(unsigned char *)callback_phone);
					putstring(COM2,";\r\n");
				}
			}

			//Check GSM output string
			while ( !u2_rx_buf.empty ) {//have data ...
				//reset timer here
				mg323_status.at_time = OSTime ;				

				memset(rec_str, 0x0, AT_CMD_LENGTH);
				if ( get_respond(rec_str) ) {
					;//prompt("Rec:%s\r\n",rec_str);

					gsm_string_decode( rec_str ,&relay_timer );
				}
			}//end of Check GSM output string


			//Check mg323 has data or not
			if ( !mg323_status.rx_empty && mg323_status.tcp_online) { //has data

				read_tcp_data( rec_str );

			}

			//Check mg323_cmd.tx_len, if > 0, then send it
			if ( mg323_status.tcp_online \
				&& mg323_cmd.tx_len > 0 \
				&& !mg323_cmd.lock ) { //can send data

				send_tcp_data( );
			}
		}
		else { //gsm power off
			led_off(OBD_CAN10);
		}

		//update relay status even gsm power off
		if ( OSTime - relay_timer > mg323_status.ring_count*10*60*1000 ) { //10 mins.
			led_off(OBD_CAN20);//shutdown relay
			mg323_status.ring_count = 0 ;
			mg323_status.need_dial = true ;
		}
		else {// < mg323_status.ring_count*10 mins, open relay
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

	mg323_cmd.rx_time = OSTime ;

	while ( !strstr((char *)buf,"^SISR: 0,") \
			&& !mg323_cmd.rx_full \
			&& (OSTime - mg323_cmd.rx_time) < 5*AT_TIMEOUT ) {

		while ( u2_rx_buf.empty && \
			(OSTime - mg323_cmd.rx_time) < AT_TIMEOUT ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		if ( get_respond(buf) ) {
			buf_len = strlen((char*)buf);
			//prompt("buf:%s, len: %d\r\n",buf,buf_len);
			if ( strstr((char *)buf,"^SISR: 0,") ) {//found
				//^SISR: 0,10 收到数据10个 or 
				//^SISR: 0,0  no data, update mg323_status.rx_empty

				//提取 buffer 长度
				switch (buf_len) {

				case 12://^SISR: 0,1
					gsm_tcp_len = buf[9] - 0x30 ;
					break;

				case 13://^SISR: 0,12
					gsm_tcp_len = (buf[10] - 0x30)+\
									 ((buf[9] - 0x30)*10) ;
					break;

				case 14://^SISR: 0,123
					gsm_tcp_len = (buf[11] - 0x30)+\
									((buf[10] - 0x30)*10)+\
									((buf[9]  - 0x30)*100) ;
					break;

				case 15://^SISR: 0,1234
					gsm_tcp_len = (buf[12] - 0x30)+\
									((buf[11] - 0x30)*10)+\
									((buf[10] - 0x30)*100)+\
									((buf[9]  - 0x30)*1000) ;
					break;

				default:
					prompt("Illegal length %d, check %s, line:	%d\r\n",\
							buf_len,__FILE__, __LINE__);
					break;

				}//end of switch

				if ( gsm_tcp_len == 0 ) {//no data
					mg323_status.rx_empty = true ;
					//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
				}
				else {
					//push data to mg323_cmd.rx
					//prompt("Push to rx:\t");
					for ( i = 0 ; i < gsm_tcp_len; i++ ) {
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
		}//end of if ( get_respond(buf) ) 

	}

}

static void send_tcp_data( )
{

#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif
	
	OS_ENTER_CRITICAL();
	mg323_cmd.lock = true ;
	OS_EXIT_CRITICAL();

	if ( gsm_ask_tcp(mg323_cmd.tx_len) ) {//GSM buffer ready
		if ( gsm_send_tcp(mg323_cmd.tx,mg323_cmd.tx_len) ) {
			;//send ok, but need return ^SISW: 0,1 for confirm
			//mg323_cmd.tx_len = 0 ;
			//printf("\tOK!\r\n");
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

	//unlock when receive ^SISW: 0,1
	//OS_ENTER_CRITICAL();
	//mg323_cmd.lock = false ;
	//OS_EXIT_CRITICAL();
}

//return 0:	ok
//return 1: unknow string
static unsigned char gsm_string_decode( unsigned char *buf , unsigned int *timer )
{
	static unsigned char i ;
#if OS_CRITICAL_METHOD == 3  /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0;
#endif

	//found GSM auto report
	if (strstr((char *)buf,"network is unavailable")) {
		mg323_status.tcp_online = false ;
		return 0;
	}

	//found voice call
	if (strstr((char *)buf,"RING")) {
		putstring(COM2,"ATH\r\n");
		*timer = OSTime;
		mg323_status.ring_count++;
		mg323_status.need_dial = false ;
		mg323_status.voice_confirm = false ;
		putstring(COM2,"ATH\r\n");
		prompt("Receive %d time call.\r\n",mg323_status.ring_count);
		return 0;
	}

	//found GPRS status  respond
	if (strstr((char *)buf,"CGREG: 0,")) {
		prompt("Rec:%s\r\n",buf);
		switch (buf[9]) {

			case 0x31://CGREG: 0,1\r\n
				mg323_status.gprs_ready = true ;
				mg323_status.roam = false;
				break;

			case 0x35://CGREG: 0,5\r\n
				mg323_status.gprs_ready = true ;
				mg323_status.roam = true;
				break;

			default:
				mg323_status.gprs_ready = false ;
				prompt("Unknow GPRS status: %s, check %s: %d\r\n",\
							buf,__FILE__, __LINE__);
				break;

		}//end of switch
		return 0;
	}
			
	//found GPRS att respond
	if (strstr((char *)buf,"+CGATT")) {
		prompt("Rec:%s\r\n",buf);
		if ( buf[8] == '1' )
			{ mg323_status.cgatt= true; }
		else
			{ mg323_status.cgatt= false; }
		return 0;
	}

	//found signal respond
	if (strstr((char *)buf,"+CSQ:")) {
        if(buf[7] == 0x2c)
        {
            mg323_status.signal = buf[6] - 0x30; 
        }
        else
        {
            buf[6] = buf[6] - 0x30;
            buf[7] = buf[7] - 0x30;
            mg323_status.signal = buf[6]*10 + buf[7];   
        }
		prompt("Signal:%02d\r\n",mg323_status.signal);
		return 0;
	}

	//Internet profile status
	// 2: allocated
	// 3: connecting
	// 4: up
	// 5: closing : dest_svr close
	// 6: down
	if (strstr((char *)buf,"SISI: 0,")) {
		//^SISI: 0,5,0,0,0,0
		if ( buf[9]== 0x34 ) {
			mg323_status.tcp_online = true ;
			mg323_status.try_online = 0 ;
		}
		else {
			mg323_status.tcp_online = false ;
			prompt("!!! TCP off line. !!!\r\n");
			//maybe some problem
			OSTimeDlyHMSM(0, 0, 1, 0);
		}
		return 0;
	}

	//found call confirm
	if (strstr((char *)buf,"BUSY")) {//call success
		mg323_status.need_dial = false ;
		mg323_status.voice_confirm = true ;
		mg323_status.dial_timer = 0 ; //prepare for next call
		prompt("Confirmed by voice.\r\n");
		return 0;
	}

	//found ^SISW: 0,1,1460 or Rec:^SISW: 0,1,1360
	if (strstr((char *)buf,"^SISW: 0,1,1")) {
		//need to double check
		prompt("TCP online: %s\r\n",buf);
		mg323_status.tcp_online = true ;
		//ask the IP, return:^SICI: 0,2,1,"10.156.174.147"
		putstring(COM2,"AT^SICI?\r\n");
		return 0;
	}

	//found IP message
	if (strstr((char *)buf,"SICI: 0,2,1,")) {
		i = 0 ;//222.222.222.222
		while ( (buf[i+14] != 0x22) && i < 15) { //"
			mg323_status.local_ip[i] = buf[i+14];
			i++ ;
		}
		return 0;
	}

	//GSM TCP rec data: ^SISR: 0,0
	if (strstr((char *)buf,"^SISR: 0,")) {//Rec tcp data
		//^SISR: 0,0  :no data
		if ( buf[10] > 0x30 ) {//have data
			//prompt("TCP rec data: %s\r\n",buf);
			mg323_status.rx_empty = false ;
		}
		else {
			mg323_status.rx_empty = true ;
			//prompt("\r\nGSM TCP no data. %s\tline: %d\r\n",__FILE__, __LINE__);
		}
		return 0;
	}

	//Send TCP data by gprs success
	if ( cmpmem(buf,"^SISW: 0,1\r\n",12 )) {
		prompt("Rec:%s\r\n",buf);
		if ( mg323_cmd.lock ) {
			mg323_cmd.tx_len = 0 ;
			printf("\t...Send OK!\r\n");
	
			OS_ENTER_CRITICAL();
			mg323_cmd.lock = false ;
			OS_EXIT_CRITICAL();
		}
		else {
			prompt("Logic error, mg323_cmd.lock should lock, but now is unlock! ");
			printf("check %s: %d\r\n",__FILE__, __LINE__);
		}
		return 0;
	}

	if ( cmpmem(buf,"OK\r\n",4 )) {
		return 0;
	}

	if ( cmpmem(buf,"\r\n",2 )) {
		return 0;
	}

	prompt("Unknow respond:%s\r\n",buf);
	return 1;
	//add others respond string here
}
