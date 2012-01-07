#include "me3000.h"

#ifdef DEBUG_GSM
#define debug_gsm(x, args...)  prompt(x, ##args);
#else
#define debug_gsm(x, args...)  ;
#endif

extern u8 *u2_tx_buf;//set to tx buffer before enable interrupt
extern u8 *u2_rx_buf;//set to rx buffer before enable interrupt
extern u8 u2_rec_binary ; //0:receive bin, 1:char, default is char
extern u32 u2_tx_cnt , u2_rx_cnt ;

u8 g_trn_sbuf[AT_CMD_LENGTH]= {0};

//AT回显命令；
uc8 at_echo_false[] = "ATE0\r";
uc8 at_echo_true[] = "ATE1\r";

//设置9600波特率
uc8 at_baudrate_9600[] = "AT+IPR=9600\r";
//设置115200波特率
uc8 at_baudrate_115200[] = "AT+IPR=115200\r";

//检查SIM卡当前状态
uc8 at_check_sim[] = "AT+CPIN?\r";

//关闭流控制
uc8 at_close_ifc[] = "AT+IFC=0,0\r";

//查询GSM网络登记
uc8 at_check_gsm_reg[] = "AT+CREG?\r";

//查询GPRS网络登记
uc8 at_check_gprs_reg[] = "AT+CGREG?\r";

//查询信号强度
uc8 at_check_signal[] = "AT+CSQ\r";

//配置APN 定义PDP上下文,参数需要改变
uc8 at_set_apn[] = "AT+CGDCONT=1,\"IP\",\"CMNET\"\r";
uc8 at_set_apnzte[] = "AT+ZPNUM=\"CMNET\",\"USER\",\"PWD\"\r";
uc8 at_get_apnzte[] = "AT+ZPNUM?\r"; //返回CMNET USER PWD

//打开GPRS 数据连接，PPP 拨号
uc8 at_open_ppp[] = "AT+ZPPPOPEN\r";

//查询 TCP 连接状态，
//ESTABLISHED 表示TCP已经连接
//DISCONNECTED 表示TCP已经断开
uc8 at_ask_iplink[] = "AT+ZIPSTATUS=1\r";

//查询 GPRS 连接状态，
//ESTABLISHED 表示PPP已经连接
//DISCONNECTED 表示PPP已经断开
uc8 at_ask_ppplink[] = "AT+ZPPPSTATUS\r";

//查询模块IP
uc8 at_ask_ip[] = "AT+ZIPGETIP\r";

//通过域名查IP
uc8 at_check_ip[] = "AT+ZDNSGETIP=\"cqt.8866.org\"\r";

//设置ip地址和端口号
u8 at_set_ipport[35] = "AT+ZIPSETUP=1,\r";
u8 dest_port[] = {",23\r"};        // port

//断开socket
uc8 at_ip_close[] = "AT+ZIPCLOSE=1\r";
//断开PPP连接
uc8 at_ppp_close[] = "AT+ZPPPCLOSE\r";

//Shutdown ME3000 , AT+ZPWROFF
uc8 at_gsm_off[] = "AT+ZPWROFF\r";

void gsm_bus_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART2_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 , ENABLE);

	/* TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART2_InitStructure.USART_BaudRate = 115200;
	USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART2_InitStructure.USART_StopBits = USART_StopBits_1;
	USART2_InitStructure.USART_Parity = USART_Parity_No ;
	USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART2_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART2_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	USART_Cmd(USART2, ENABLE);
}

static void uart2_send_byte(u8 ch)
{
	USART_SendData(USART2, ch);
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

static void uart2_send_str(uc8  *str)
{
	while(*str) {
			uart2_send_byte(*str++);
	}
}

static unsigned int  uart2_receive_str(u8* f_str)
{
	u32  i;
	u32  uart2_time;

	i = 0;
	uart2_time = OSTime ;
	while( OSTime - uart2_time < 600)
	{
		if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
		{
			*(f_str + i++) = USART_ReceiveData(USART2);
			if ( i > AT_CMD_LENGTH ) { //overflow
				i = 0 ; //prevent overflow, but lost data
			}
		}
	}
	return i;
}

//me3000发送命令
void send_at(uc8* send_str, u8* rev_str)
{
	ITStatus u2_status = RESET;

	u2_status = USART_GetITStatus(USART2, USART_IT_RXNE);
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);

	OSTimeDlyHMSM(0, 0, 0, 1);

	uart2_send_str(send_str);  //发送AT命令

	memset(rev_str, 0x0, AT_CMD_LENGTH);

	uart2_receive_str(rev_str);

	if ( u2_status == RESET ) {
		USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	}
	else {
		USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	}
	OSTimeDlyHMSM(0, 0, 0, 1);
}

bool test_at_uart(void)
{
	u8 i = 0 ;
	u8 result_temp1;
	u8 result_temp2;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区
	uc8 at_test[] = "AT\r";    //AT测试命令；

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		i++;
		send_at(at_test, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 1, 0);
		debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		result_temp2 = strncmp((char *)rev_at_sbuf, "\r\nOK\r\n",6);
		result_temp1 = strncmp((char *)rev_at_sbuf, "AT\r\r\nOK\r\n",9);
	}while((!(result_temp1 || result_temp2))&&(i <= 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		return true ;
	}
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
	u8 i = 0 ;
	u8 result_temp1,  result_temp2;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	if (f_mod)  // 1 回显  不需要
	{
			send_at(at_echo_true, rev_at_sbuf);
	}
	else  // 0 禁止回显
	{
		do{
		i++;
		send_at(at_echo_false, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 0, 50);
		//debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		result_temp1 = strncmp((char *)rev_at_sbuf,"ATE0\r\n\r\nOK\r\n",12); //解析me3000返回AT命令
		result_temp2 = strncmp((char *)rev_at_sbuf,"\r\nOK\r\n",6);
		}while((!(result_temp1||result_temp2))&&(i < 10));
	}
}

//------------------------------------------------------------------------------
//原型：
//功能：设置9600bps波特率
//参数：
//返回：
//备注：
//------------------------------------------------------------------------------
bool set_baud_rate(void)
{
	u8 i = 0 ;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		 i++;
		 send_at(at_baudrate_115200, rev_at_sbuf);
		 OSTimeDlyHMSM(0, 0, 0, 50);
		 debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		 result_temp = strncmp((char *)rev_at_sbuf,"\r\nOK\r\n",6);
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//------------------------------------------------------------------------------
//原型：
//功能：检查SIM状态
//参数：
//返回：
//备注：
//------------------------------------------------------------------------------
bool check_sim_state(void)
{
	u8 i = 0 ;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
	i++;
		send_at(at_check_sim, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 0, 200);
		debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
	result_temp = strncmp((char *)rev_at_sbuf,"\r\n+CPIN: READY",14);  //解析GTM900B返回AT命?
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		return true ;
	}
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
	u8 i = 0 ;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		 i++;
		 send_at(at_close_ifc, rev_at_sbuf);
		 OSTimeDlyHMSM(0, 0, 0, 10);
	 debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		 result_temp = strncmp((char *)rev_at_sbuf,"\r\nOK\r\n",6);  //解析GTM900B返回AT命令
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		return true ;
	}
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

	u8  result_temp;
	u8   i = 0;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	do{
		set_at_echo(0);//设置禁止回显
		i++;
		memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);
		send_at(at_check_gsm_reg, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 1, 0);
	 	result_temp = strncmp((char *)rev_at_sbuf, "\r\n+CREG: 0, 1", 13);
		result_temp |= strncmp((char *)rev_at_sbuf, "\r\n+CREG: 0, 5", 13);
	 	debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
	}while((!result_temp)&&(i < 120));

	if(i >= 120)  {
		return false ;      
	}
	else {
		return true ;
	}
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
	u8   result_temp;
	u8   i = 0;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do
	{
		i++;
		send_at(at_check_gprs_reg, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 1, 0);
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
		result_temp = strncmp((char *)rev_at_sbuf, "\r\n+CGREG: 0, 1", 14);
	}while((!result_temp) && (i < 120));

	if(i >= 120)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：连接GPRS的时候检测_CSQ信号值
//参数：
//返回：
//备注：
//---------------------------------------------------------------------------------------------//
u8 me3000_check_CSQ(void)
{
	u8  result_temp;
	u8  i;
	u8  l_tmp = 0;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	for(i = 0; i < 3; i++)
	{
		send_at(at_check_signal, rev_at_sbuf);

		result_temp = strncmp((char *)rev_at_sbuf,"\r\n+CSQ: ",8);

		if(result_temp)
		{
			if(rev_at_sbuf[9] == 0x2c)//信号为个位数
			{
				l_tmp = rev_at_sbuf[8] - 0x30;
			}
			else
			{
				rev_at_sbuf[8] = rev_at_sbuf[8] - 0x30;
				rev_at_sbuf[9] = rev_at_sbuf[9] - 0x30;
				l_tmp = rev_at_sbuf[8]*10 + rev_at_sbuf[9];
			}
			return  l_tmp;
		}
	}
	return  0;//返回信号错误
}

bool set_apn_zte(void)
{
	u8 i = 0;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do
	{
		i++;
		send_at(at_set_apnzte, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 0, 100);
		debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		result_temp = strncmp((char *)rev_at_sbuf,"\r\nOK\r\n",6);  //解析GTM900B返回AT命令
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		return true ;
	}
}

bool open_ppp_link(void)
{
	u8 i = 0;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		set_at_echo(0);//设置禁止回显
		OSTimeDlyHMSM(0, 0, 1, 0);
		i++;
		send_at(at_open_ppp, rev_at_sbuf);
		debug_gsm("%s try: %d, return %s\r\n",__FUNCTION__,i,rev_at_sbuf);
		result_temp = strncmp((char *)rev_at_sbuf,"\r\n+ZPPPOPEN:CONNECTED",21);  //解析GTM900B返回AT命令
		result_temp |= strncmp((char *)rev_at_sbuf,"\r\n+ZPPPOPEN:ESTABLISHED",23);  //解析GTM900B返回AT命令
	}while((!result_temp)&&(i < 20));

	if(i >= 20)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//查询PPP连接状态
//return  true if ESTABLISHED
//       false if DISCONNECTED
bool check_ppp_link(void)
{
	u8 i = 0;
	char  *result_temp=NULL;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		i++;
		send_at(at_ask_ppplink, rev_at_sbuf);
		result_temp = strstr((char *)rev_at_sbuf,"ZPPPSTATUS"); //解析GR64返回AT命令
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 0, 200 );
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		result_temp = strstr((char *)rev_at_sbuf,"ESTABLISHED"); //解析GR64返回AT命令
		if ( result_temp ) {
			return true ;
		}
		else {
			return false ;
		}
	}
}

//查询模块IP
bool me3000_check_ip( u8 *rev_at_sbuf )
{
	u8 i = 0;
	u8 result_temp;

	do
	{
		OSTimeDlyHMSM(0, 0, 0, 200);
		i++;
		send_at(at_ask_ip, rev_at_sbuf);
		result_temp = 1;
	}while((!result_temp)&&(i < 5));

	if(i >= 5)  {
		return false ;      
	}
	else {
		return true ;
	}
}

// 通过域名得到IP
u8 domain2ip(u8 *destip)
{
	u8  i = 0 , ip_len = 0 ;
	char result_temp ;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		send_at(at_check_ip, rev_at_sbuf);
		debug_gsm("%s: try %d, return %s\r\n",__FUNCTION__,i,rev_at_sbuf);
		result_temp = strncmp((char *)rev_at_sbuf,"\r\n+ZDNSGETIP:",13);
		OSTimeDlyHMSM(0, 0, 0, 500);
		i++;
	}while((!result_temp) && (i < 20));

	if(i >= 20)  {
		return 0 ;      
	}
	else {
		while(*(rev_at_sbuf + 13 + ip_len) != 0x0d && i < 10)
		{
			ip_len++;
		}
	
		memcpy(destip,rev_at_sbuf + 13,ip_len);
	
		return ip_len;
	}
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：设置IP、PORT参数
//参数：uchar * pIP_Port 存放地址
//返回：
//备注：
//---------------------------------------------------------------------------------------------//
bool set_ip_port(u8 *pIP, u8 *pPort,u8 ip_len)
{
	char  result_temp;
	u8  i = 0 ;
	u8  rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	memcpy((u8 *)(at_set_ipport + 14),pIP,ip_len);
	memcpy((u8 *)(at_set_ipport + 14 + ip_len),dest_port,5);
	debug_gsm("Set ip port: %s\r\n", at_set_ipport);

	do{
		send_at(at_set_ipport, rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 1, 0);
		result_temp = strncmp((char *)rev_at_sbuf,"\r\n+ZIPSETUP:CONNECTED",21);
		result_temp |= strncmp((char *)rev_at_sbuf,"\r\n+ZIPSETUP:ESTABLISHED",23);
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
		i++;
	}while((!result_temp)&&(i < 12));

	if(i >= 12)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//查询ip连接状态
//return  true if ESTABLISHED
//       false if DISCONNECTED
bool check_ip_link(void)
{
	u8 i = 0;
	char  *result_temp=NULL;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		i++;
		send_at(at_ask_iplink, rev_at_sbuf);
		result_temp = strstr((char *)rev_at_sbuf,"ZIPSTATUS"); //解析GR64返回AT命令
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
		OSTimeDlyHMSM(0, 0, 0, 100 );
	}while((!result_temp)&&(i < 10));

	if(i >= 10)  {
		return false ;      
	}
	else {
		result_temp = strstr((char *)rev_at_sbuf,"ESTABLISHED"); //解析GR64返回AT命令
		if ( result_temp ) {
			return true ;
		}
		else {
			return false ;
		}
	}
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：断开socket
//参数：
//返回：
//备注：
//---------------------------------------------------------------------------------------------//
bool close_ip(void)
{
	char  *result_temp=NULL;
	u8  i = 0;
	u8  rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	set_at_echo(0);//设置禁止回显
	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		i++;
		OSTimeDlyHMSM(0, 0, 0, 500);
		send_at(at_ip_close, rev_at_sbuf);

		result_temp = strstr((char *)rev_at_sbuf,"ZIPCLOSE:OK"); //解析GR64返回AT命令
		if ( !result_temp ) { //no found
			result_temp = strstr((char *)rev_at_sbuf,"ZIPCLOSE:DISCONNECTED"); //解析GR64返回AT命令
		}
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
	}while((!result_temp) && (i < 20));

	if(i >= 20)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//----------------------------------------------------------------------------------------------//
//原型：
//功能：断开ppp
//参数：
//返回：
//备注：
//---------------------------------------------------------------------------------------------//
bool  close_ppp(void)
{
	char  *result_temp=NULL;
	u8  i = 0;
	u8  rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	set_at_echo(0);//设置禁止回显
	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		i++;
		OSTimeDlyHMSM(0, 0, 1, 0);
		send_at(at_ppp_close, rev_at_sbuf);
		result_temp = strstr((char *)rev_at_sbuf,"ZPPPCLOSE:OK"); //解析GR64返回AT命令
		if ( !result_temp ) { //no found
			result_temp = strstr((char *)rev_at_sbuf,"ZPPPCLOSE:DISCONNECTED"); //解析GR64返回AT命令
		}
		debug_gsm("%s try: %d, result_temp = %d, return %s\r\n",__FUNCTION__,i,result_temp,rev_at_sbuf);
	}while((!result_temp) && (i < 20));

	if(i >= 20)  {
		return false ;      
	}
	else {
		return true ;
	}
}

//Function: connect to server by tcp
//return: 0 success
//        4  failure, GSM module: ME3000 no respond
//        8  failure, SIM card error
//        10 failure, no gsm network
//        20 failure, no gprs network
//        30 failure, gsm signal too week
//        35 failure, set APN error
//        40 failure, open ppp error
//        50 failure, ppp status is DISCONNECTED
//        60 failure, get IP error
//        70 failure, get server IP error
//        80 failure, open socket error
//        90 failure, connect to server error, check network or server
u8 gsm_connect_tcp(void)
{
	u8 s_flag=0, ip_len=0;
	u8 my_ip[AT_CMD_LENGTH]; //接收AT命令返回缓冲区
	u8 dest_ip[15];

	debug_gsm("\r\nTurn on GSM power...\r\n");
	GSM_PM_ON;

	//check module: me3000
	OSTimeDlyHMSM(0, 0, 2, 0);
	if( !test_at_uart() )  {
		//module no respond, need to reset
		debug_gsm("\r\nGSM module no respond, force shutdown...\r\n");
		GSM_PM_OFF;
		OSTimeDlyHMSM(0, 0, 30, 0);
		return 4 ; //me3000 no respond
	}

	OSTimeDlyHMSM(0, 0, 0, 100);
	set_at_echo(0);//设置禁止回显
	OSTimeDlyHMSM(0, 0, 0, 100);
	//set_baud_rate();//设置固定波特率为 115200bps

	//check SIM card status
	OSTimeDlyHMSM(0, 0, 0, 100);
	if( !check_sim_state() )  {
		return 8 ; //SIM Card error
	}

	close_ifc();//关闭硬件流控制

	//check gsm network
	set_at_echo(0);//设置禁止回显
	if( !check_gsm_net() )  {
		return 10 ; //no gsm network
	}

	//check gprs
	set_at_echo(0);//设置禁止回显
	if( !check_gprs_net() )  {
		return 20 ; //no gprs network
	}

	//检测网络信号强度 <8 不登陆
	s_flag = me3000_check_CSQ();
	debug_gsm("Current singal is: %d\r\n",s_flag);
	if(s_flag < 8) {
		debug_gsm("Signal too weak.\r\n");
		return 30 ;
	}

	//set APN
	set_at_echo(0);//设置禁止回显
	if ( !set_apn_zte() ) { //failure
		return 35 ; //set apn error
	}

	//ask ppp by gprs
	OSTimeDlyHMSM(0, 0, 0, 500);
	if ( !open_ppp_link() ) { //failure
		return 40 ; //open ppp error
	}

	if ( !check_ppp_link() ) { //failure
		return 50 ; //ppp status is DISCONNECTED
	}

	//Check my IP
	OSTimeDlyHMSM(0, 0, 0, 500);
	if ( !me3000_check_ip(my_ip) ) { //failure
		return 60 ; //get my IP error
	}
	else {
		debug_gsm("my IP is: %s\r\n",my_ip);
	}

	//get server IP
	ip_len = domain2ip(dest_ip);
	if ( ip_len < 7 ) { //failure
		return 70 ; //get server IP error
	}
	else {
		debug_gsm("Dest IP is: %s\r\n",dest_ip);
	}

	//建立TCP 服务器连接，开启一个SOCKET
	if ( !set_ip_port(dest_ip, dest_port, ip_len) ) { //failure
		debug_gsm("IPSETUP with port: %s failure!\r\n",dest_port);
		return 80 ; //open socket error
	}

	//wait, maybe server send something
	OSTimeDlyHMSM(0, 0, 1, 0);

	//check tcp status
	if ( !check_ip_link() ) { //failure
		return 90 ; //connect to server error
	}

	debug_gsm("gsm_connect_tcp ok. %s :%d\r\n",__FILE__, __LINE__);
	return 0 ;
}

//Function: disconnect server by tcp
//return: 0  success
//        10 close TCP socket error
//        15 disconnect TCP socket error
//        20 close PPP error
u8 gsm_disconn_tcp(void)
{
	//check tcp status
	if ( !close_ip() ) { //failure
		return 10 ; //close TCP error
	}

	//check tcp status
	if ( !check_ip_link() ) { //DISCONNECTED
		debug_gsm("Disconnect ok.\r\n");
	}
	else {
		debug_gsm("Disconnect failure.\r\n");
		return 15 ; //DISCONNECTED to server failure
	}

	//check tcp status
	if ( !close_ppp() ) { //failure
		return 20 ; //close IP error
	}

	OSTimeDlyHMSM(0, 0, 1, 0);
	return 0 ;
}

/* 关机：
 * 若已经PPP 拨号成功了并连接到Internet 后，等待5s 连续输入+++（不带回车符）、
 * 等待5s 输入回车符(CR)、等待5s 输入ATH(CR)、等待5s 输入AT+CGATT=0(CR)、等待5s
 * 输入AT+ZPWROFF(CR)，完成以上操作后，再等待10s 即可执行断电操作，直接关闭模块
 */

bool gsm_pwr_off(void)
{
	u8 i = 0;
	u8 result_temp;
	u8 rev_at_sbuf[AT_CMD_LENGTH]; //接收AT命令返回缓冲区

	if ( check_ppp_link() ) { //ppp status is ESTABLISHED
		//at send +++
		uart2_send_str("+++");
	
		OSTimeDlyHMSM(0, 0, 5, 0);
		uart2_send_str("\r");
	
		OSTimeDlyHMSM(0, 0, 5, 0);
		uart2_send_str("ATH\r");
	
		OSTimeDlyHMSM(0, 0, 5, 0);
		uart2_send_str("AT+CGATT=0\r");
	
		OSTimeDlyHMSM(0, 0, 5, 0);
	}

	memset(rev_at_sbuf, 0x0, AT_CMD_LENGTH);

	do{
		set_at_echo(0);//设置禁止回显
		OSTimeDlyHMSM(0, 0, 1, 0);
		i++;
		send_at(at_gsm_off, rev_at_sbuf);
		debug_gsm("%s return %s\r\n",__FUNCTION__,rev_at_sbuf);
		result_temp = strncmp((char *)rev_at_sbuf,"\r\nOK\r\n",6);  //解析GTM900B返回AT命令
	}while((!result_temp)&&(i < 2));

	if(i >= 2)  {
		GSM_PM_OFF; //force power shutdown
		OSTimeDlyHMSM(0, 0, 5, 0);
		return false ;      
	}
	else {
		OSTimeDlyHMSM(0, 0, 1, 0);//不能太长，否则会自动重开机
		debug_gsm("Turn off GSM power.\r\n");
		GSM_PM_OFF;
		return true ;
	}
}
