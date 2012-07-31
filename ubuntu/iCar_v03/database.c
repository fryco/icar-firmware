//$URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/database.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "database.h"
#include "commands.h"

extern int debug_flag ;

const unsigned char *ERR_GSM[][1]= {\
//	SHUTDOWN_NO_ERR	=	0,	//normal, no error
	"SHUTDOWN NO ERR",\
//	NO RESPOND		=	1, 	//if ( OSTime - mg323 status.at timer > 10*AT_TIMEOUT )
	"GSM Module no respond",\
//	SIM_CARD_ERR	=	2,	//Pin? no SIM?
	"SIM CARD ERR",\
//	NO_GSM_NET		=	3,	//no GSM net or can't register
	"NO GSM NET",\
//	NO_CARRIER_INFO	=	4,	//Get GSM carrier info failure
	"Get GSM carrier info failure",\
//	SIGNAL_WEAK		=	5,	//gsm signal < MIN_GSM_SIGNAL
	"Signal weak",\
//	NO_GPRS			=	6,	//mg323_status.gprs_count > 60
	"NO GPRS",\
//	RSV				=	7,	//Reserve
	"RSV",\
//	RSV				=	8,	//Reserve
	"RSV",\
//	RSV				=	9,	//Reserve
	"RSV",\
//	RSV				=	10,	//Reserve
	"RSV",\
//	RSV				=	11,	//Reserve
	"RSV",\
//	RSV				=	12,	//Reserve
	"RSV",\
//	TRY_ONLINE		=	13,	//if ( my_icar.mg323.try_online > MAX_ONLINE_TRY )
	"Try online > MAX_ONLINE_TRY",\
//	RSV				=	14,	//Reserve
	"RSV",\
//	MODULE_REBOOT	=	15	//if receive: SYSSTART
	"GSM module reboot"\
};

const unsigned char *ERR_UPGRADE[][1]= {\
	"UPGRADE_NO_ERR",\
	"firmware upgrade successful",\
	"STM32 has latest firmware, no need upgrade",\
	"firmware size too large > 60KB",\
	"Upgrading a newer firmware",\
	"No upgrade info in flash",\
	"Block CRC error",\
	"Upgrading FW Rev no match",\
	"Prog flash failure",\
	"Receive upgrade data length un-correct",\
	"Firmware CRC error",\
	"un-expect firmware ready flag",\
	"Receive update parameter length un-correct",\
	"Update parameter rev no match",\
	"RSV",\
	"Parameters update successful"\
};

const unsigned char *ERR_GPRS[][1]= {\
//	DISCONNECT_NO_ERR = 0,	//normal, no error
	"Disconnect NO ERR",\
//	CONNECTION_DOWN	=	1,	//^SICI: 0,2,0
	"Connection Down",\
//	PEER_CLOSED 	= 	2,	//^SIS: 0, 0, 48, Remote Peer has closed the connection
	"Remote Peer closed the connection",\
//	PROFILE_NO_UP 	=	3,	//AT^SISI return is not 4: up
	"Profile NO UP",\
//	RX_TIMEOUT		=	4,	//should rx result after tx data
	"RX timeout after TX",\
//	RSV				=	5,	//Reserve
	"RSV",\
//	NO_GPRS_IN_INIT	=	6,	//no gprs network
	"NO GPRS IN INIT",\
//	GPRS_ATT_ERR	=	7,	//gprs attach failure
	"GPRS attach failure",\
//	CONN_TYPE_ERR	=	8,	//set connect type error
	"Set connection type error",\
//	GET_APN_ERR		=	9,	//get APN error
	"GET APN ERR",\
//	SET_APN_ERR		=	10,	//set APN error
	"SET APN ERR",\
//	SET_CONN_ERR	=	11,	//set conID error
	"Set conID error",\
//	SVR_TYPE_ERR	=	12,	//set svr type error
	"Set svr type error",\
//	DEST_IP_ERR		=	13	//set dest IP and port error
	"Set dest IP and port error",\
//	RSV				=	14,	//Reserve
	"RSV",\
//	RSV				=	15,	//Reserve
	"RSV"\
};

const unsigned char *ERR_MCU_RST[][1]= {\
//	MCU_RST_NO_ERR	=	0,	//normal, no error
	"MCU RST NO ERR",\
//	External Reset	=	1
	"External Reset",\
//	Power On Reset	=	2
	"Power On Reset",\
//	Software reset	=	3
	"Software reset",\
//	Reset by IWDG	=	4
	"Independent watchdog reset",\
//	Reset by WWDG	=	5
	"Window watchdog reset",\
//	Low-power reset	=	6
	"Low-power reset",\
//	RSV				=	7,	//Reserve
	"RSV",\
//	RSV				=	8,	//Reserve
	"RSV",\
//	RSV				=	9,	//Reserve
	"RSV",\
//	RSV				=	10,	//Reserve
	"RSV",\
//	RSV				=	11,	//Reserve
	"RSV",\
//	RSV				=	12,	//Reserve
	"RSV",\
//	RSV				=	13,	//Reserve
	"RSV",\
//	RSV				=	14,	//Reserve
	"RSV",\
//	RSV				=	15,	//Reserve
	"RSV"\
};

//Connect to MYSQ
//return: 0 success
//        1 failure
int db_connect(struct icar_db *my_icar_db)
{
	int err;

	if ( debug_flag ) {
		;//fprintf(stderr, "Initializing mysql ... \n");
	}

	if ( !mysql_init(&(my_icar_db->mysql)) )
	{
		if ( debug_flag ) {
			fprintf(stderr, "Initializing mysql failure, return\n");
		}
		return 1;
	}

	if (!mysql_real_connect(&(my_icar_db->mysql),\
							my_icar_db->db_host,\
							my_icar_db->db_user,\
							my_icar_db->db_pwd,\
							my_icar_db->db_name,\
							0,NULL,0))	{
		err = mysql_errno(&(my_icar_db->mysql)) ;
		if ( err == 1049 ) { //Unknown database, create it
			if (!mysql_real_connect(&(my_icar_db->mysql),
							my_icar_db->db_host,\
							my_icar_db->db_user,\
							my_icar_db->db_pwd,\
							"mysql",0,NULL,0)) {
			//connect with default database:mysql still failure
				return 1 ;
			}

			if ( debug_flag ) {
				fprintf(stderr, "Create database icar_v03 ...\n");
			}
			mysql_query(&(my_icar_db->mysql),"create database icar_v03;");
			mysql_query(&(my_icar_db->mysql),"use icar_v03;");
		}
		else { //others error
			fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(my_icar_db->mysql)));
			return 1;
		}
	}

	//本次连接使用的默认字符集为utf8
	mysql_query( &(my_icar_db->mysql) , "SET NAMES 'utf8'");

	if ( debug_flag ) {
		;//fprintf(stderr, "Connectiong to mysql ok.\n");
	}

	return 0 ;
}

int db_check(struct icar_data *mycar)
{
	int err;
	MYSQL_RES *res_ptr;

	//connect to mysql
	if(db_connect(&(mycar->mydb)))
	{//failure
		return 1;
	}

	//check table t_product
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_product` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_product( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
							`SN` char(10) NOT NULL COMMENT 'Product serial numble',\
							`fw_ver` decimal(2) NOT NULL COMMENT 'firmware version',\
							`encrypt_ver` decimal(3) NOT NULL COMMENT 'encryption version',\
							`dev_Serial` char(24) NOT NULL COMMENT 'MCU SN',\
							`IMEI` decimal(15) NOT NULL,\
							`IMSI` decimal(15) NOT NULL,\
							`mobile` decimal(15) NOT NULL,\
							`produce_date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
							`reg_date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
							`status` enum('un-active', 'active', 'repaired', 'EOL') NOT NULL\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");

			//insert test data
			mysql_query(&(mycar->mydb.mysql),\
			"insert into `t_product` VALUES ('','02P11AH003',\
											'01',\
											'001',\
											'D335FF333732473143022475',\
											'732473143022475',\
											'732473143022475',\
											'13800138000',\
											'1319444768',\
											'1309444768',\
											'1');");

		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	//check table t_log
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
                   `SN` char(10) NOT NULL COMMENT 'Product serial numble',\
                   `IP` varchar(15) NOT NULL COMMENT 'external IP',\
                   `port` smallint unsigned NOT NULL,\
                   `IP_local` varchar(15) NOT NULL  COMMENT 'internal IP',\
                   `action` enum('login', 'logout', 'upload', 'download', 'net_err') NOT NULL,\
                   `status` enum('first_login', 'normal', 'pwd_error') NOT NULL\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	//check table t_log_ip
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_ip` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_ip( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
					`SN` char(10) NOT NULL COMMENT 'Product serial numble',\
					`IP` varchar(15) NOT NULL COMMENT 'external IP',\
					`port` smallint unsigned NOT NULL,\
					`IP_local` varchar(15) NOT NULL  COMMENT 'internal IP',\
					`OSTime` int unsigned NOT NULL COMMENT '100Hz Tick',\
					`PID` int unsigned NOT NULL\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	//check table t_log_signal
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_signal` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_signal( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
                   `SN` char(10) NOT NULL COMMENT 'Product serial numble',\
                   `cmd_seq` tinyint unsigned NOT NULL,\
                   `gsm_signal` tinyint unsigned NOT NULL,\
                   `signal_seq` smallint unsigned NOT NULL\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	//check table t_log_error
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_error` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_error( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
			`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
			`SN` char(10) NOT NULL COMMENT 'Product serial numble',\
			`err_time` int unsigned NOT NULL,\
			`err_code` tinyint unsigned NOT NULL,\
			`err_str` varchar(64) NOT NULL\
			) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	//check table t_log_command_s
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_command_s` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_command_s(`ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,\
					`date` int unsigned NOT NULL  COMMENT 'FROM_UNIXTIME(date)',\
					`SN` char(10) NOT NULL COMMENT 'Product serial numble',\
					`cmd_seq` tinyint unsigned NOT NULL,\
					`cmd_pcb` tinyint unsigned NOT NULL,\
					`cmd_str` varchar(100) NOT NULL,\
					`rx_len` smallint unsigned NOT NULL,\
					`return` enum('NO_ERR', 'NEED_SN', 'DB_ERR') NOT NULL,\
					`tx_len` smallint unsigned NOT NULL \
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	
	//check table t_log_command_l
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_command_l` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_command_l( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
					`SN` char(10) NOT NULL COMMENT 'Product serial numble',\
					`cmd_seq` tinyint unsigned NOT NULL,\
					`cmd_pcb` tinyint unsigned NOT NULL,\
					`cmd_str` varchar(4096) NOT NULL,\
					`rx_len` smallint unsigned NOT NULL,\
					`return` enum('NO_ERR', 'NEED_SN', 'DB_ERR') NOT NULL,\
					`tx_len` smallint unsigned NOT NULL \
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(mycar->mydb.mysql));
			return 1 ;
		}
	}

	mysql_close(&(mycar->mydb.mysql));

	return 0 ;
}

//0: ok
//others: error, check err_code and err_msg
int check_sn(struct icar_data *mycar)
{
	char sql_buf[BUFSIZE];
	unsigned int sn_len;
	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//sn example: 02P11AH000
	//1, must 10 char
	sn_len = strlen(mycar->sn) ;
	if ( sn_len == 10 ) {//correct
		if ( debug_flag ) {
			fprintf(stderr, "iCar serial numble correct.\n");
		}
	}
	else { //illegal
		fprintf(stderr, "iCar serial numble error, length:%d.\n",sn_len);
		mycar->err_code = 10 ;
		snprintf(mycar->err_msg,BUFSIZE-1,"SN length error\n");
		return 1 ;
	}
	
	//remove 0x0D + 0x0A
	*((mycar->sn)+10) =0x0, *((mycar->sn)+11) =0x0;

	//check sn in db?
	snprintf(sql_buf,BUFSIZE-1,"SELECT 1 FROM `t_product` WHERE `SN` = '%s' limit 1;",mycar->sn);

	//fprintf(stderr, "%s\n",sql_buf);

	if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}
	else { //ok
		if ( debug_flag ) {
			fprintf(stderr, "mysql_query ok.\n");
		}
	}

	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	if(!res_ptr) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}

	sqlrow_cnt = mysql_num_rows(res_ptr) ;
	mysql_free_result(res_ptr);

	if ( sqlrow_cnt > 0 ) {//exist
		return 0 ;
	}

	//no exist, but SN correct, add to db
	snprintf(sql_buf,BUFSIZE-1,	"insert into `t_product` VALUES ('','%s',\
								'01',\
								'001',\
								'D335FF333732473143022475',\
								'732473143022475',\
								'732473143022475',\
								'13800138000',\
								'%d',\
								'%d',\
								'2');",mycar->sn,time(NULL),time(NULL));
								//enum('un-active','active','repaired','EOL') 

	//insert data
	if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}
	else { //ok
		if ( debug_flag ) {
			fprintf(stderr, "insert new SN ok.\n");
		}
	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	fprintf(stderr, "\nBuf:%s\n",mycar->sn);
	
	return 0;
}

//0: ok
//others: error, check err_code and err_msg
int record_command(struct icar_data *mycar, unsigned char *buf, unsigned char * tx_return,unsigned int tx_len)
{
	char sql_buf[BUFSIZE*2];
	unsigned int buf_index, buf_len;
	unsigned char cmd_str[BUFSIZE];
	unsigned char cmd_seq=0,cmd_pcb=0;

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//input: HEAD+SEQ+PCB+LEN+DATA+CHK
	//DATA: command 
	//i.e:  C9 00 54 00 0A 30 32 50 31 31 41 48 30 30 30 FC

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		;//fprintf(stderr, "%02X ",*(buf+buf_index));
	}

	cmd_seq = buf[1];
	cmd_pcb = buf[2];
	buf_len = buf[3] << 8 | buf[4];
	//fprintf(stderr, "\r\nSEQ: 0x%02X, PCB: 0x%02X, Len: %d\r\n",\
		cmd_seq,cmd_pcb,buf_len);

	if ( buf_len ) {
		for ( buf_index = 5 ; buf_index < buf_len+5 ; buf_index++ ) {
			snprintf(&cmd_str[(buf_index-5)*3],8,"%02X ",*(buf+buf_index));
			//fprintf(stderr, "%02X ",*(buf+buf_index));
		}
		cmd_str[((buf_index-5)*3)-1] = 0x0 ;
		//fprintf(stderr, "cmd_str: %s\r\n",cmd_str);
	}
	else {
		cmd_str[0] = 0x0 ;
	}

	memset(sql_buf, '\0', BUFSIZE*2);
	if ( buf_len*3 < 100 ) { //insert into short table
		//insert cmd detail to table: t_log_command_short
		snprintf(sql_buf,BUFSIZE-1,"insert into t_log_command_s values ('',\
									'%d',\
									'%s',\
									'%d',\
									'%d',\
									'%s',\
									'%d',\
									'%s',\
									'%d'\
									);",\
									time(NULL),\
									mycar->sn,\
									cmd_seq,\
									cmd_pcb,\
									cmd_str,\
									buf_len+6,\
									tx_return,\
									tx_len \
									);

		if ( debug_flag ) {
			;//fprintf(stderr, "%s\r\n",sql_buf);
		}

		if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
			fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
			return 1;
		}
		else { //ok
			if ( debug_flag ) {
				;//fprintf(stderr, "Record CMD: 0x%02X ok.\n",cmd_pcb);
			}
		}

	}
	else { //insert into long table
	
		//insert cmd detail to table: t_log_command_long
		snprintf(sql_buf,BUFSIZE-1,"insert into t_log_command_l values ( '',\
									'%d',\
									'%s',\
									'%d',\
									'%d',\
									'%s',\
									'%d',\
									'%s',\
									'%d'\
									);",\
									time(NULL),\
									mycar->sn,\
									cmd_seq,\
									cmd_pcb,\
									cmd_str,\
									buf_len+6,\
									tx_return,\
									tx_len \
									);

		if ( debug_flag ) {
			;//fprintf(stderr, "%s\r\n",sql_buf);
		}

		if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
			fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
			return 1;
		}
		else { //ok
			if ( debug_flag ) {
				;//fprintf(stderr, "Record CMD: 0x%02X ok.\n",cmd_pcb);
			}
		}

	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	return 0;
}

//0: ok
//others: error, check err_code and err_msg
int record_signal(struct icar_data *mycar, unsigned char *buf, unsigned char *p_buf)
{
	char sql_buf[BUFSIZE];
	unsigned int buf_index, buf_len;
	unsigned char cmd_seq=0, gsm_signal;
	unsigned int  record_seq,adc_voltage;

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//input: HEAD+SEQ+PCB+LEN+DATA+CHK
	//DATA: Record_seq(2 bytes)+GSM signal(1byte)+voltage(2 bytes)
	//i.e:  C9 01 52 00 14 FF FF 1C 0F FF 73 
	cmd_seq = buf[1];
	buf_len = buf[3] << 8 | buf[4];

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		;//fprintf(stderr, "%02X ",*(buf+buf_index));
	}
	//fprintf(stderr, "\r\nLen: %d ",buf_len);

	record_seq = buf[5] << 8 | buf[6];
	fprintf(stderr, "Record_seq: %d ",record_seq);

	gsm_signal = buf[7] ;
	fprintf(stderr, "Singal: %d ",gsm_signal);

	adc_voltage = buf[8] << 8 | buf[9];
	fprintf(stderr, "adc_voltage: %d\r\n",adc_voltage);

	//prepare cloud post string
	snprintf(p_buf,BUFSIZE-1,"ip=%s&fid=38&subject=%s => GSM signal: %d&message=Record sequence: %d\r\n\r\nip: %s",\
						(char *)inet_ntoa(mycar->client_addr.sin_addr),\
						mycar->sn,gsm_signal,record_seq,\
						(char *)inet_ntoa(mycar->client_addr.sin_addr));

	//insert GSM IP and signal to table t_log_signal:
	snprintf(sql_buf,BUFSIZE-1,"insert into t_log_signal values ( '',\
								'%d',\
								'%s',\
								'%d',\
								'%d',\
								'%d');",\
								time(NULL),\
								mycar->sn,\
								cmd_seq,\
								gsm_signal,\
								record_seq);

	if ( debug_flag ) {
		;//fprintf(stderr, "%s\r\n",sql_buf);
	}

	if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}
	else { //ok
		;
	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	return 0;

}


//0: ok
//others: error, check err_code and err_msg
int record_ip(struct icar_data *mycar, unsigned char *buf, unsigned char *p_buf)
{
	char sql_buf[BUFSIZE];
	unsigned int buf_index, buf_len, ostime=0;
	unsigned char gsm_ip[20];

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//input: HEAD+SEQ+PCB+LEN+OSTime+SN+IP+CHK
	//IP(123.123.123.123) 31 32 33 2E 31 32 33 2E 31 32 33 2E 31 32 33
	//i.e: C9 01 53 00 1B 00 00 0D 99 
	//     30 32 50 31 43 30 44 32 41 37 
	//     31 30 2E 32 30 31 2E 31 33 37 2E 32 37 28 
	buf_len = buf[3] << 8 | buf[4];

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		;//fprintf(stderr, "%02X ",*(buf+buf_index));
	}
	//fprintf(stderr, "\r\nLen: %d ",buf_len);

	ostime = buf[5] << 24 | buf[6] << 16 | buf[7] << 8 | buf[9];
	fprintf(stderr, "\r\nOSTime: %d ",ostime);
	
	for ( buf_index = 0 ; buf_index < buf_len - 14 ; buf_index++ ) {
		gsm_ip[buf_index] = *(buf+19+buf_index) ;
		;//fprintf(stderr, "buf_index=%d\t%X\r\n",buf_index,gsm_ip[buf_index]);
	}
	gsm_ip[buf_index]  = 0x0; 
	gsm_ip[15] = 0x0; //ip length < 15

	if ( debug_flag ) {
		fprintf(stderr, "GSM IP: %s\r\n",gsm_ip);
	}

	snprintf(p_buf,BUFSIZE-1,"ip=%s&fid=40&subject=%s => login&message=uptime(H:M:S): %d:%02d:%02d\r\n\
				\r\nLAN ip:%s\r\n\r\nip: %s",\
				(char *)inet_ntoa(mycar->client_addr.sin_addr),mycar->sn,\
				ostime/360000,((ostime/100)%3600)/60,((ostime/100)%3600)%60,\
				gsm_ip,(char *)inet_ntoa(mycar->client_addr.sin_addr));

	//insert GSM IP and signal to table t_log_ip:
	snprintf(sql_buf,BUFSIZE-1,"insert into t_log_ip values ( '',\
								'%d',\
								'%s',\
								'%s',\
								'%d',\
								'%s',\
								'%d',\
								'%d');",\
								time(NULL),\
								mycar->sn,\
								(char *)inet_ntoa(mycar->client_addr.sin_addr),\
								ntohs(mycar->client_addr.sin_port),\
								gsm_ip,\
								ostime,\
								getpid());

	if ( debug_flag ) {
		;//fprintf(stderr, "%s\r\n",sql_buf);
	}

	if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}
	else { //ok
		;//
	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	return 0;

}

//0: ok
//others: error, check err_code and err_msg
int record_error(struct icar_data *mycar, unsigned char *buf, unsigned char *part, \
				unsigned char *p_buf)
{
	char sql_buf[BUFSIZE];
	unsigned int buf_index, buf_len, err_time=0, err_len, err_code;
	unsigned char err_str[MAX_LOG_LENGTH];

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//input: HEAD+SEQ+PCB+LEN+err_time+err_code+CHK
	//i.e: C9 09 45 00 2A 00 00 10 4D 01 
	//     47 53 4D 20 6D 6F 64 75 6C 65 20 70 6F 77 65 72 20 6F 
	//	   66 66 2C 20 63 68 65 63 6B 20 61 70 70 5F 67 73 6D 2E 68 
	buf_len = buf[3] << 8 | buf[4];

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		;//fprintf(stderr, "%02X ",*(buf+buf_index));
	}
	//fprintf(stderr, "\r\nLen: %d ",buf_len);

	err_time = buf[5] << 24 | buf[6] << 16 | buf[7] << 8 | buf[8];

	//fprintf(stderr, "\r\nerr_time: %08X ",err_time);

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:upgrade fw success flag
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason

	//err_code, 2 byte
	err_code = buf[9] << 8 | buf[10];
	if ( err_code & 0xF000 ) { //highest priority, MCU reset
		err_code = (err_code & 0xF000) >> 12 ;
		*part = 4 ;
		snprintf(err_str, MAX_LOG_LENGTH-1, \
			"MCU reset: %s",*ERR_MCU_RST[err_code]);
	}
	else {
		if ( err_code & 0x0F00 ) { //higher priority
			err_code = (err_code & 0x0F00) >> 8 ;
			*part = 3 ;
			snprintf(err_str, MAX_LOG_LENGTH-1, \
				"STM32 fw: %s",*ERR_UPGRADE[err_code]);
		}
		else {	
			if ( err_code & 0x00F0 ) { //GPRS disconnect err
				err_code = (err_code & 0x00F0) >> 4 ;
				*part = 2 ;
				snprintf(err_str, MAX_LOG_LENGTH-1, \
					"GPRS disconnect: %s",*ERR_GPRS[err_code]);
		 	}
			else {		
				if ( err_code & 0x000F ) { //GSM module poweroff err
					err_code = (err_code & 0x000F) ;
					*part = 1 ;
					snprintf(err_str, MAX_LOG_LENGTH-1, \
						"GSM module power off: %s",*ERR_GSM[err_code]);
				}
				else {//program logic error
					fprintf(stderr, "firmware logic error, chk: %s: %d\r\n",\
						__FILE__,__LINE__);	
				}//end (i & 0x000F)
			}//end (i & 0x00F0)
		}//end (i & 0x0F00)
	}//end (i & 0xF000)

	//fprintf(stderr, "\terr_code: %04X ",err_code);

	//snprintf(err_str, 127, "Different SN: %s and ",mycar->sn);
	//err_len = strlen( err_str );

	err_str[MAX_LOG_LENGTH-1] = 0x0; //prevent overflow

	//prepare cloud post string

	snprintf(p_buf,BUFSIZE-1,"ip=%s&fid=41&subject=%s =>ERR: %s&message=Client time: %s\r\n\r\nip: %s",\
						(char *)inet_ntoa(mycar->client_addr.sin_addr),\
						mycar->sn,err_str,(char *)ctime(&err_time),\
						(char *)inet_ntoa(mycar->client_addr.sin_addr));


	if ( debug_flag ) {
		fprintf(stderr, "err_str: %s\r\n",err_str);
	}

	//insert err to table t_log_error:
	snprintf(sql_buf,BUFSIZE-1,"insert into t_log_error values ( '',\
								'%d',\
								'%s',\
								'%d',\
								'%d',\
								'%s'\
								);",\
								time(NULL),\
								mycar->sn,\
								err_time,\
								err_code,\
								err_str\
								);

	if ( debug_flag ) {
		;//fprintf(stderr, "%s\r\n",sql_buf);
	}

	if ( mysql_query(&(mycar->mydb.mysql),sql_buf)) {//error
		fprintf(stderr, "mysql_query error: %d, meaning:%s\r\n",\
						mysql_errno(&(mycar->mydb.mysql)),mysql_error(&(mycar->mydb.mysql)));
		return 1;
	}
	else { //ok
		;//
	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	return 0;

}

//0: ok
//others: error, check err_code and err_msg
int ask_instruction(struct icar_data *mycar, unsigned char *buf, unsigned char *ist)
{
	char sql_buf[BUFSIZE];

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	// *ist = 0x55 ;//Upgrade firmware
	// *ist = 0x75 ;//Update parameter

	 *ist = 0 ;//No new instruction

	//input: HEAD+SEQ+PCB+LEN+CHK

//Will inquire DB for instruction

	return 0;

}
