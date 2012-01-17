#include "main.h"

#ifdef DEBUG_GSM
#define debug_gsm(x, args...)  prompt(x, ##args);
#else
#define debug_gsm(x, args...)  ;
#endif

extern struct UART_RX u2_rx_buf;
extern struct GSM_STATUS mg323_status ;

const unsigned char dest_server[] = "cqt.8866.org:24";
const unsigned char at_set_channel0_para[] = "AT^SICS=0,";

//查询连接状态： AT^SISI?
//正确返回: ^SISI: 0,4,0,0,0,0
//错误返回: ^SISI: 0,2,0,0,0,0
// 2：allocated
// 3：connecting
// 4：up
// 5：closing
// 6：down

//IMSI共有15位，其结构如下：MCC+MNC+MIN
//MCC：Mobile Country Code，移动国家码，共3位，中国为460;
//MNC:Mobile Network Code，移动网络码，共2位，
const unsigned char *apn_list[][4]= {\
//MCC+MNC,apn,user,passwd
//apn carrier="China-Mobile"
"46000","CMNET","","",\
//apn carrier="China-Mobile"
"46002","CMNET","","",\
//apn carrier="China-Mobile"
"46007","CMNET","","",\
//apn carrier="Vodafone NL"
"20404","live.vodafone.com","vodafone","vodafone",\
//apn carrier="T-Mobile Internet"
"20416","internet","","",\
//apn carrier="Orange NL"
"20420","internet","",""\
};

/*if match return 1 , else return 0*/
unsigned char cmpmem(unsigned char *buffer, unsigned char *cmpbuf,unsigned char count)
{
	static unsigned char equal;    

	while((count--) && (equal =(*buffer++ == *cmpbuf++)));  

	return equal;  
}

bool get_respond( unsigned char* rec_str)
{
	u32  respond_time;
	u8 i = 0 ;

	respond_time = OSTime ;
	while( OSTime - respond_time < AT_TIMEOUT ) {
		OSTimeDlyHMSM(0, 0, 0, 10);
		while ( !u2_rx_buf.empty ) {//receive some data...

			//reset timer here
			mg323_status.at_timer = OSTime ;				

			rec_str[i] = getbyte( COM2 );
	
#ifdef DEBUG_GSM
			;//putbyte( COM1, rec_str[i]);
#endif
			if ( rec_str[i] != 0 ) { //prevent 0x0
				i++;
			}
			if ( i > AT_CMD_LENGTH ) {//overflow
				return false ;
			}
			if ( rec_str[i-1] == 0x0A && rec_str[i-2] == 0x0D ) {
				return true ;
			}
		}
	}
	return false ;
}

bool test_at_uart(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		//uart2_buf_clean( );
		putstring(COM2, "AT\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"OK\r\n")) {//found
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

//------------------------------------------------------------------------------
//原型：
//功能：设置回显模式
//参数：
//返回：
//备注：1-回显  0-不回显
//------------------------------------------------------------------------------
void set_at_echo(u8 f_mod)
{
	if (f_mod)  // 1 回显  不需要
	{
		putstring(COM2, "ATE1\r\n"); //enable echo
	}
	else  // 0 禁止回显
	{
		putstring(COM2, "ATE0\r\n"); //disable echo
	}
}

bool check_sim_state(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2, "AT+CPIN?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}

				if (strstr((char *)respond_str,"CPIN: READY\r\n")) {//found
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

//------------------------------------------------------------------------------
//原型：
//功能：关闭流控制
//参数：
//返回：
//备注：
//------------------------------------------------------------------------------
bool close_ifc(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2, "AT+IFC=0,0\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"OK\r\n")) {//found
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

bool factory_setting(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2, "AT&F\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}

				if (strstr((char *)respond_str,"OK\r\n")) {//found
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT&F no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

bool show_income_number(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2, "AT+CLIP=1\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"OK\r\n")) {//found
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT CLIP no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

unsigned char check_gsm_CSQ( )
{
	u8 retry , l_tmp = 0 ;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2,"AT+CSQ\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}

		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"+CSQ:")) {//found
		            if(respond_str[7] == 0x2c)//信号为个位数
		            {
		                l_tmp = respond_str[6] - 0x30; 
		            }
		            else
		            {
		                respond_str[6] = respond_str[6] - 0x30;
		                respond_str[7] = respond_str[7] - 0x30;
		                l_tmp = respond_str[6]*10 + respond_str[7];   
		            }
					if ( l_tmp > 12 ) {
			            return  l_tmp;
					}
					else {
						OSTimeDlyHMSM(0, 0,	1, 0);
						break ;
					}
				}
			}
			else {
				debug_gsm("CMD:AT no respond, timeout\r\n");
			}
		}
	}
	return false ;      
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：GSM网络登记查询
//参数：
//返回：0-OK  1-未得到GSM网络
//备注：
//---------------------------------------------------------------------------------------------//
bool check_gsm_net(void)
{	//测到SIM 卡后最长应当在120s 内成功搜网
	//返回0,1 表示已经注册到本地网络
	//而0,5 表示已经注册到漫游网络

	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 100 ; retry++) {
		putstring(COM2, "AT+CREG?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"CREG: 0,1\r\n")) {//normal
					return true;
				}
				if (strstr((char *)respond_str,"CREG: 0,5\r\n")) {//roam
					mg323_status.roam = true ;
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool check_gsm_cops( )
{

	u8 i, retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2, "AT+COPS?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"+COPS: ")) {//normal
					i = 0 ;
					while ( respond_str[i] != '\r' ) {
						mg323_status.carrier[i] = respond_str[i] ;
						i++;
					}
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT+COPS? no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	0, 200);
	}
	return false ;      
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：gprs网络登记查询
//参数：
//返回：0-OK  1-未得到GPRS网络
//备注：
//---------------------------------------------------------------------------------------------//
bool check_gprs_net(void)
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 100 ; retry++) {
		putstring(COM2, "AT+CGREG?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"CGREG: 0,1\r\n")) {
					return true;
				}
				if (strstr((char *)respond_str,"CGREG: 0,5\r\n")) {//roam
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT+CGREG? no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool check_gprs_att( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 20 ; retry++) {
		putstring(COM2, "AT+CGATT?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"+CGATT:")) {
					if ( respond_str[8] == '1' )
						{ mg323_status.cgatt= true; }
					else
						{ mg323_status.cgatt= false; }
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT+CGATT? no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}


bool get_apn_by_imsi( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];
	unsigned int i, apn_count ;

	apn_count = sizeof(apn_list)/sizeof(apn_list[0]);

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2, "AT+CIMI\r\n");//get imsi
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				
				if ( (respond_str[0] >= 0x30) && (respond_str[0] <= 0x39) ){
					debug_gsm("IMSI: %s\r\n",respond_str);
					strncpy((char *)mg323_status.imsi, (char *)respond_str, 15);
					for ( i = 0 ; i < apn_count ; i++ ) {
						if ( cmpmem(respond_str,(unsigned char *)apn_list[i][0],5 )) {
							mg323_status.apn_index = i ;
							return true;
						}
					}
				}
			}
			else {
				debug_gsm("CMD:AT+CIMI no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool set_conn_type( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SICS=0,conType, GPRS0\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool set_gprs_apn( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2, (unsigned char *)at_set_channel0_para);
		putstring(COM2, "apn,");
		putstring(COM2, (unsigned char *)apn_list[mg323_status.apn_index][1]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	0, 500);
	}
	return false ;      
}

bool set_gprs_user( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2, (unsigned char *)at_set_channel0_para);
		putstring(COM2, "user,");
		putstring(COM2, (unsigned char *)apn_list[mg323_status.apn_index][2]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}

				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	0, 500);
	}
	return false ;      
}

bool set_gprs_passwd( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2, (unsigned char *)at_set_channel0_para);
		putstring(COM2, "passwd,");
		putstring(COM2, (unsigned char *)apn_list[mg323_status.apn_index][3]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	0, 500);
	}
	return false ;      
}

bool set_conn_id( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISS=0,conId,0\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool set_svr_type( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISS=0,srvType, Socket\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool set_dest_ip( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISS=0,address,\"socktcp:\/\/");
		putstring(COM2,mg323_status.server_ip_port);
		putstring(COM2,"\"\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool start_tcp_conn( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISO=0\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool close_tcp_conn( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISC=0\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"OK\r\n")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SISC no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

u8 check_tcp_status( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISI?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"SISI: 0,")) {
				//^SISI: 0,2,0,0,0,0
					return respond_str[9]-0x30;
				}
			}
			else {
				debug_gsm("CMD:AT^SISI no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

bool get_local_ip( )
{
	u8 i, retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2,"AT^SICI?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				//^SICI: 0,2,1,"10.156.174.147"
				i = 0 ;
				if (strstr((char *)respond_str,"SICI: 0,2,1,")) {
					while ( respond_str[i+14] != 0x22 ) { //"
						mg323_status.local_ip[i] = respond_str[i+14];
						if ( i > 15 ) return false ;//prevent overflow
						i++ ;
					}
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SICS no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

//Ask GSM ready to send data to server by TCP
//return ture if ready, else return false
bool gsm_ask_tcp( unsigned int dat_len ) 
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SISW=0,");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		snprintf((char *)respond_str,AT_CMD_LENGTH-1,"%d\r\n",dat_len);
		putstring(COM2,respond_str);

		//wait GSM return: ^SISW: 0,xx,xx
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {

				if (strstr((char *)respond_str,"^SISR: 0,") \
					&& respond_str[10] > 0x30 ) {
					mg323_status.rx_empty = false ;
				}

				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"^SISW: 0,")) {
					//GSM ready, can send data
					//need to verify the xx == dat_len ???
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SISW no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	10, 0);
	}

	return false ;      
}

bool gsm_send_tcp( unsigned char *send_buf, unsigned int dat_len )
{
	unsigned int i ;
	u8 respond_str[AT_CMD_LENGTH];

	//debug_gsm("TCP Send: ");
	for ( i = 0 ; i < dat_len ; i++ ) {
		putbyte(COM2, send_buf[i]) ;
		#ifdef DEBUG_GSM
			;//printf("%02X ",send_buf[i]);
		#endif
	}

	#ifdef DEBUG_GSM
		;//printf("\r\n");
	#endif

	//wait GSM return: ERROR or OK
	memset(respond_str, 0x0, AT_CMD_LENGTH);
	if ( u2_rx_buf.empty ) {//no data...
		OSTimeDlyHMSM(0, 0,	0, 500);
	}
	while ( !u2_rx_buf.empty ) {//have data ...
		if ( get_respond(respond_str) ) {

			if (strstr((char *)respond_str,"^SISR: 0,") \
				&& respond_str[10] > 0x30 ) {
				mg323_status.rx_empty = false ;
			}

			if (strstr((char *)respond_str,"ERROR")) {
				return false;
			}
			if (strstr((char *)respond_str,"OK")) {
				OSTimeDlyHMSM(0, 0,	0, 50);//wait ^SISW: 0,1
			}

			if (strstr((char *)respond_str,"^SISW: 0,1")) {
				return true;
			}
		}
		else {
			debug_gsm("CMD:Send TCP data timeout!\r\n");
			return false ;
		}
	}

	return false ;
}

bool gsm_dial( unsigned char * phone_number )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2,"ATD");
		putstring(COM2,phone_number);
		putstring(COM2,";\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"CONF: 1")) {
				//^CONF: 1
					return true;
				}
			}
			else {
				debug_gsm("CMD:ATD no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	10, 0);
	}
	return false ;      
}

bool shutdown_mg323( )
{
	u8 retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 3 ; retry++) {
		putstring(COM2,"AT^SMSO\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( u2_rx_buf.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 500);
		}
		while ( !u2_rx_buf.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				if (strstr((char *)respond_str,"SHUTDOWN")) {
					return true;
				}
			}
			else {
				debug_gsm("CMD:AT^SMSO no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	1, 0);
	}
	return false ;      
}

//Function: connect to server by tcp
//return: 0 success
//        4  failure, GSM module: ME3000 no respond
//        8  failure, SIM card error
//        10 failure, no gsm network
//        15 failure, get gsm carrier error
//        18 failure, gsm signal too week
//        20 failure, no gprs network
//        25 failure, gprs attached failure
//        30 failure, set connect type error
//        32 failure, get APN error
//        34 failure, set APN error
//        36 failure, set APN user error
//        38 failure, set APN pwd error
//        40 failure, set conID error
//        45 failure, set svr type error
//        50 failure, set dest IP and port error
//        60 failure, start connection error
//        70 failure, get IP error

//        90 failure, connect to server error, check network or server
unsigned char gsm_power_on( )
{
	prompt("Turn on GSM power...\r\n");
	GSM_PM_ON;

	//check module: mg323
	OSTimeDlyHMSM(0, 0, 2, 0);
	if( !test_at_uart() )  {
		//module no respond, need to reset
		debug_gsm("\r\nGSM module no respond, force shutdown...\r\n");
		GSM_PM_OFF;
		mg323_status.power_on = false;
		OSTimeDlyHMSM(0, 0, 30, 0);
		return 4 ; //mg323 no respond
	}

	OSTimeDlyHMSM(0, 0, 0, 100);
	set_at_echo(0);//设置禁止回显

	//check SIM card status
	OSTimeDlyHMSM(0, 0, 0, 100);
	if( !check_sim_state() )  {
		return 8 ; //SIM Card error
	}
	prompt("Check SIM card status ok.\r\n");

	factory_setting( );
	OSTimeDlyHMSM(0, 0, 0, 500);
	close_ifc();//关闭硬件流控制

	//check gsm network
	set_at_echo(0);//设置禁止回显
	if( !check_gsm_net() )  {
		return 10 ; //no gsm network
	}
	debug_gsm("Check GSM net ok\r\n");
	return 0 ;
}

unsigned char gsm_check_gprs( )
{
	//check carrier
	set_at_echo(0);//设置禁止回显
	if( !check_gsm_cops() )  {
		return 15 ; //get carrier error
	}
	prompt("Carrier is %s\r\n",mg323_status.carrier);

	//来电显示 AT+CLIP=1
	show_income_number( );

	//检测网络信号强度 <12 不登陆
	mg323_status.signal = check_gsm_CSQ();

	debug_gsm("Current singal is: %d\r\n",mg323_status.signal);
	if( mg323_status.signal < 12) {
		prompt("GSM Signal too weak:%d\r\n",mg323_status.signal);
		return 18 ;
	}

	//check gprs
	set_at_echo(0);//设置禁止回显
	if( !check_gprs_net() )  {
		return 20 ; //no gprs network
	}

	//check gprs attached
	set_at_echo(0);//设置禁止回显
	if( !check_gprs_att() )  {
		return 25 ; //gprs attach failure
	}
	debug_gsm("GPRS attache status is: %d\r\n", mg323_status.cgatt);

	//set connect type
	if( !set_conn_type( ) )  {
		debug_gsm("Set connect type error\r\n");
		return 30 ; //set connect type error
	}

	//check apn
	if( !get_apn_by_imsi( ) )  {
		debug_gsm("Get APN error\r\n");
		return 32 ; //get apn error
	}
	debug_gsm("Get APN ok: %s %s %s\r\n", apn_list[mg323_status.apn_index][1],\
									apn_list[mg323_status.apn_index][2],\
									apn_list[mg323_status.apn_index][3]);

	//set APN
	if( !set_gprs_apn( ) )  {
		debug_gsm("Set APN error\r\n");
		return 34 ; //set apn error
	}
	prompt("Set APN ok.\r\n");

	if ( *(apn_list[mg323_status.apn_index][2]) ) {	
		if( !set_gprs_user( ) )  {
			debug_gsm("Set APN user error\r\n");
			return 36 ; //set apn user error
		}
	}

	if ( *(apn_list[mg323_status.apn_index][3]) ) {	
		if( !set_gprs_passwd( ) )  {
			debug_gsm("Set APN passwd error\r\n");
			return 38 ; //set apn passwd error
		}
	}

	//set connect ID
	if( !set_conn_id( ) )  {
		debug_gsm("Set connect id error\r\n");
		return 40 ; //set connect type error
	}

	//set service type, only support socket
	if( !set_svr_type( ) )  {
		debug_gsm("Set service type error\r\n");
		return 45 ; //set svr type error
	}

	//set destination IP and port
	if( !set_dest_ip( ) )  {
		debug_gsm("Set dest IP and port error\r\n");
		return 50 ; //set dest IP and port error
	}
	return 0 ;
}

bool gsm_pwr_off(void)
{
	u8 result_temp;

	//close connect... AT^SISC=0
	if( !close_tcp_conn( ) )  {
		debug_gsm("Close connection error\r\n");
	}

	//Check link status... AT^SISI?
	result_temp = check_tcp_status( );
	debug_gsm("AT^SISI return %d\r\n",result_temp);

	if ( result_temp != 2 ) { //error, force shutdown
		close_tcp_conn( );
		OSTimeDlyHMSM(0, 0, 5, 0);
		close_tcp_conn( );
		OSTimeDlyHMSM(0, 0, 5, 0);
	}

	//shutdown module ... AT^SMSO
	if( !shutdown_mg323( ) )  {
		debug_gsm("shutdown error\r\n");
		OSTimeDlyHMSM(0, 0, 5, 0);
		shutdown_mg323( );
	}
	else {
		debug_gsm("shutdown CMD ok.\r\n");
	}

	OSTimeDlyHMSM(0, 0, 1, 0);//不能太长，否则会自动重开机
	prompt("Turn off GSM power.\r\n");
	GSM_PM_OFF;

	return true ;
}
