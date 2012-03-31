#include "main.h"

#ifdef DEBUG_GSM
#define debug_gsm(x, args...)  prompt(x, ##args);
#else
#define debug_gsm(x, args...)  ;
#endif

extern struct ICAR_DEVICE my_icar;

const unsigned char dest_server[] = "cqt.8866.org:25";
const unsigned char at_set_channel0_para[] = "AT^SICS=0,";
//  Init sequence:
//  AT
//  ATE0
//  AT+CPIN?
//  AT&F
//  AT+IFC=0,0
//  ATE0
//  AT+CREG?
//  ATE0
//  AT+COPS?
//  AT+CLIP=1
//  AT+CSQ
//  ATE0
//  AT+CGREG?
//  ATE0
//  AT+CGATT?
//  AT^SICS=0,conType, GPRS0
//  AT+CIMI
//  AT^SICS=0,apn,CMNET
//  AT^SISS=0,conId,0
//  AT^SISS=0,srvType, Socket
//  AT^SISS=0,address,"socktcp://183.37.50.222:25"
//  AT^SISO=0
//  AT+CSQ
//  AT^SISO=0
//  AT^SISI?

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
		OSTimeDlyHMSM(0, 0, 0, 20);
		while ( !my_icar.stm32_u2_rx.empty ) {//receive some data...

			//reset timer here
			my_icar.mg323.at_timer = OSTime ;				

			rec_str[i] = getbyte( COM2 );
	
			if ( my_icar.debug > 2)
				putbyte( COM1, rec_str[i]);

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

	for ( retry = 0 ;retry < 50 ; retry++) {
		putstring(COM2, "AT\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				if (strstr((char *)respond_str,"ERROR")) {
					OSTimeDlyHMSM(0, 0,	0, 200);
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}

		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
						OSTimeDlyHMSM(0, 0,	0, 500);
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"CREG: 0,1\r\n")) {//normal
					return true;
				}
				if (strstr((char *)respond_str,"CREG: 0,5\r\n")) {//roam
					my_icar.mg323.roam = true ;
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"+COPS: ")) {//normal
					i = 0 ;
					while ( respond_str[i] != '\r' ) {
						my_icar.mg323.carrier[i] = respond_str[i] ;
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"+CGATT:")) {
					if ( respond_str[8] == '1' )
						{ my_icar.mg323.cgatt= true; }
					else
						{ my_icar.mg323.cgatt= false; }
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				
				if ( (respond_str[0] >= 0x30) && (respond_str[0] <= 0x39) ){
					debug_gsm("IMSI: %s\r\n",respond_str);
					strncpy((char *)my_icar.mg323.imsi, (char *)respond_str, 15);
					for ( i = 0 ; i < apn_count ; i++ ) {
						if ( cmpmem(respond_str,(unsigned char *)apn_list[i][0],5 )) {
							my_icar.mg323.apn_index = i ;
							return true;
						}
					}
				}
			}
			else {
				debug_gsm("CMD:AT+CIMI no respond, timeout\r\n");
			}
		}
		OSTimeDlyHMSM(0, 0,	0, 500);
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		putstring(COM2, (unsigned char *)apn_list[my_icar.mg323.apn_index][1]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		putstring(COM2, (unsigned char *)apn_list[my_icar.mg323.apn_index][2]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		putstring(COM2, (unsigned char *)apn_list[my_icar.mg323.apn_index][3]);
		putstring(COM2, "\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		putstring(COM2,my_icar.mg323.server_ip_port);
		putstring(COM2,"\"\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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

bool get_ip_local( )
{
	u8 i, retry;
	u8 respond_str[AT_CMD_LENGTH];

	for ( retry = 0 ;retry < 10 ; retry++) {
		putstring(COM2,"AT^SICI?\r\n");
		memset(respond_str, 0x0, AT_CMD_LENGTH);
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {
				//debug_gsm("Res_str:%s\r\n",respond_str);
				if (strstr((char *)respond_str,"ERROR")) {
					return false;
				}
				//^SICI: 0,2,1,"10.156.174.147"
				i = 0 ;
				if (strstr((char *)respond_str,"SICI: 0,2,1,")) {
					while ( respond_str[i+14] != 0x22 ) { //"
						my_icar.mg323.ip_local[i] = respond_str[i+14];
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
			if ( get_respond(respond_str) ) {

				if (strstr((char *)respond_str,"^SISR: 0,") \
					&& respond_str[10] > 0x30 ) {
					my_icar.mg323.rx_empty = false ;
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
		OSTimeDlyHMSM(0, 0,	1, 0);
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
	if ( my_icar.stm32_u2_rx.empty ) {//no data...
		OSTimeDlyHMSM(0, 0,	0, 100);
	}
	while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
		if ( get_respond(respond_str) ) {

			if (strstr((char *)respond_str,"^SISR: 0,") \
				&& respond_str[10] > 0x30 ) {
				my_icar.mg323.rx_empty = false ;
			}

			if (strstr((char *)respond_str,"ERROR")) {
				return false;
			}
			if (strstr((char *)respond_str,"OK")) {
				OSTimeDlyHMSM(0, 0,	0, 20);//wait ^SISW: 0,1
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
		OSTimeDlyHMSM(0, 0,	1, 0);
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
		if ( my_icar.stm32_u2_rx.empty ) {//no data...
			OSTimeDlyHMSM(0, 0,	0, 100);
		}
		while ( !my_icar.stm32_u2_rx.empty ) {//have data ...
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
//return: 0 success, same as POWEROFF_REASON define in app_gsm.h
//        1  failure, GSM module: MG323 no respond
//        2  failure, SIM card error
//        3  failure, no gsm network
//        4  failure, get gsm carrier error
//        5  failure, gsm signal too week
unsigned char gsm_power_on( )
{
	prompt("Turn on GSM power...\r\n");
	GSM_PM_ON;

	//check module: mg323
	OSTimeDlyHMSM(0, 0, 1, 0);//wait power stable
	if( !test_at_uart() )  {
		//module no respond, need to reset
		debug_gsm("\r\nGSM module no respond, force shutdown...\r\n");
		GSM_PM_OFF;
		my_icar.mg323.power_on = false;
		OSTimeDlyHMSM(0, 0, 10, 0);
		return 1 ; //mg323 no respond
	}

	OSTimeDlyHMSM(0, 0, 0, 100);
	set_at_echo(0);//设置禁止回显

	//check SIM card status
	OSTimeDlyHMSM(0, 0, 0, 100);
	if( !check_sim_state() )  {
		return 2 ; //SIM Card error
	}
	prompt("Check SIM card status ok.\r\n");

	factory_setting( );
	OSTimeDlyHMSM(0, 0, 1, 0);
	close_ifc();//关闭硬件流控制

	debug_gsm("Search GSM network...\r\n");
	//check gsm network
	set_at_echo(0);//设置禁止回显
	if( !check_gsm_net() )  {
		return 3 ; //no gsm network
	}
	debug_gsm("Register to GSM network ok\r\n");

	//check carrier
	set_at_echo(0);//设置禁止回显
	if( !check_gsm_cops() )  {
		prompt("Get GSM carrier info failure!");
		return 4 ; //get carrier error
	}
	prompt("Carrier is %s\r\n",my_icar.mg323.carrier);

	//来电显示 AT+CLIP=1
	show_income_number( );

	//检测网络信号强度 < MIN_GSM_SIGNAL 不登陆
	my_icar.mg323.signal = check_gsm_CSQ();

	debug_gsm("GSM signal is: %d\r\n",my_icar.mg323.signal);
	if( my_icar.mg323.signal < MIN_GSM_SIGNAL) {
		prompt("GSM Signal too weak:%d\r\n",my_icar.mg323.signal);
		return 5 ;
	}

	return 0 ;
}

//        6  failure, no gprs network
//        7  failure, gprs attached failure
//        8  failure, set connect type error
//        9	 failure, get APN error
//        10 failure, set APN error
//        10 failure, set APN user error
//        10 failure, set APN pwd error
//        11 failure, set conID error
//        12 failure, set svr type error
//        13 failure, set dest IP and port error

//        60 failure, start connection error
//        70 failure, get IP error
//        90 failure, connect to server error, check network or server

unsigned char gsm_check_gprs( )
{
	//check gprs
	set_at_echo(0);//设置禁止回显
	if( !check_gprs_net() )  {
		return 6 ; //no gprs network
	}

	//check gprs attached
	set_at_echo(0);//设置禁止回显
	if( !check_gprs_att() )  {
		prompt("GPRS attache failure! mg323.cgatt: %d\r\n", my_icar.mg323.cgatt);
		return 7 ; //gprs attach failure
	}

	//set connect type
	if( !set_conn_type( ) )  {
		prompt("Set connect type error\r\n");
		return 8 ; //set connect type error
	}

	//check apn
	if( !get_apn_by_imsi( ) )  {
		prompt("Get APN error\r\n");
		return 9 ; //get apn error
	}
	debug_gsm("Get APN ok: %s %s %s\r\n", apn_list[my_icar.mg323.apn_index][1],\
									apn_list[my_icar.mg323.apn_index][2],\
									apn_list[my_icar.mg323.apn_index][3]);

	//set APN
	if( !set_gprs_apn( ) )  {
		debug_gsm("Set APN error\r\n");
		return 10 ; //set apn error
	}
	prompt("Set APN ok.\r\n");

	if ( *(apn_list[my_icar.mg323.apn_index][2]) ) {	
		if( !set_gprs_user( ) )  {
			debug_gsm("Set APN user error\r\n");
			return 10 ; //set apn user error
		}
	}

	if ( *(apn_list[my_icar.mg323.apn_index][3]) ) {	
		if( !set_gprs_passwd( ) )  {
			debug_gsm("Set APN passwd error\r\n");
			return 10 ; //set apn passwd error
		}
	}

	//set connect ID
	if( !set_conn_id( ) )  {
		prompt("Set connect id error\r\n");
		return 11 ; //set connect type error
	}

	//set service type, only support socket
	if( !set_svr_type( ) )  {
		prompt("Set service type error\r\n");
		return 12 ; //set svr type error
	}

	//set destination IP and port
	if( !set_dest_ip( ) )  {
		prompt("Set dest IP and port error\r\n");
		return 13 ; //set dest IP and port error
	}

	prompt("Initiate GPRS setting ok\r\n");
	return 0 ;
}

bool gprs_disconnect( DISCONNECT_REASON reason)
{
	u16 result_temp;

	prompt("Close connection, reason: %d... ",reason);
	//close connect... AT^SISC=0
	if( close_tcp_conn( ) )  {
		prompt("ok.\r\n");
	}
	else {
		prompt("failure.\r\n");
	}

	my_icar.mg323.tcp_online = false ;
	my_icar.mg323.try_online_cnt = 1 ;

	//save to BK reg
	//BKP_DR4, GPRS disconnect time(UTC Time) high
	//BKP_DR5, GPRS disconnect time(UTC Time) low
    BKP_WriteBackupRegister(BKP_DR4, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
    BKP_WriteBackupRegister(BKP_DR5, (RTC_GetCounter( ))&0xFFFF);//low

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:reverse
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	result_temp = (BKP_ReadBackupRegister(BKP_DR1))&0xFF0F;
	result_temp = result_temp | ((reason<<4)&0xF0) ;
    BKP_WriteBackupRegister(BKP_DR1, result_temp);

	return true ;
}

bool gsm_pwr_off( POWEROFF_REASON reason)
{
	u16 result_temp;

	//close connect... AT^SISC=0
	if( !close_tcp_conn( ) )  {
		debug_gsm("Close connection error\r\n");
	}

	//Check link status... AT^SISI?
	result_temp = check_tcp_status( );
	debug_gsm("AT^SISI return %d\r\n",result_temp);

	if ( result_temp != 2 ) { //error, force shutdown
		close_tcp_conn( );
		OSTimeDlyHMSM(0, 0, 1, 0);
	}

	//shutdown module ... AT^SMSO
	if( !shutdown_mg323( ) )  {
		debug_gsm("shutdown error\r\n");
		OSTimeDlyHMSM(0, 0, 1, 0);
		shutdown_mg323( );
	}
	else {
		debug_gsm("shutdown CMD ok.\r\n");
	}

	OSTimeDlyHMSM(0, 0, 1, 0);//不能太长，否则会自动重开机

	GSM_PM_OFF;

	my_icar.mg323.try_online_cnt = 1;
	my_icar.mg323.gprs_count = 0 ;
	my_icar.mg323.gprs_ready = false;
	my_icar.mg323.tcp_online = false ;
	my_icar.mg323.power_on = false;
	memset(my_icar.mg323.ip_local, 0x0, IP_LEN-1);

	//save to BK reg
	//BKP_DR2, GSM Module power off time(UTC Time) high
	//BKP_DR3, GSM Module power off time(UTC Time) low
    BKP_WriteBackupRegister(BKP_DR2, ((RTC_GetCounter( ))>>16)&0xFFFF);//high
    BKP_WriteBackupRegister(BKP_DR3, (RTC_GetCounter( ))&0xFFFF);//low

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:reverse
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	result_temp = (BKP_ReadBackupRegister(BKP_DR1))&0xFFF0;
	result_temp = result_temp | reason ;
    BKP_WriteBackupRegister(BKP_DR1, result_temp);

	prompt("Turn off GSM power.\r\n");
	return true ;
}
