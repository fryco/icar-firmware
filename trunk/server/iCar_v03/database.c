#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "database.h"

extern int debug_flag ;

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

	//check table t_log_pid
	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(mycar->mydb.mysql),"SELECT * FROM `t_log_pid` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(mycar->mydb.mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(mycar->mydb.mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(mycar->mydb.mysql),\
			"create table t_log_pid( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
                   `IP` varchar(15) NOT NULL COMMENT 'external IP',\
                   `port` smallint unsigned NOT NULL,\
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
                   `IP` varchar(15) NOT NULL COMMENT 'external IP',\
                   `port` smallint unsigned NOT NULL,\
                   `IP_local` varchar(15) NOT NULL  COMMENT 'internal IP',\
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
                   `err_str` varchar(128) NOT NULL\
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
			"create table t_log_command_s( `ID` INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, \
					`date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
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
		//cmd_seq,cmd_pcb,buf_len);

	for ( buf_index = 5 ; buf_index < buf_len+5 ; buf_index++ ) {
		snprintf(&cmd_str[(buf_index-5)*3],8,"%02X ",*(buf+buf_index));
	}
	cmd_str[((buf_index-5)*3)-1] = 0x0 ;
	//fprintf(stderr, "cmd_str: %s\r\n",cmd_str);

	if ( buf_len*3 < 100 ) { //insert into short table
		//insert cmd detail to table: t_log_command_short
		snprintf(sql_buf,BUFSIZE-1,"insert into t_log_command_s values ( '',\
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
				fprintf(stderr, "Record CMD: 0x%02X ok.\n",cmd_pcb);
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
				fprintf(stderr, "Record CMD: 0x%02X ok.\n",cmd_pcb);
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
int record_signal(struct icar_data *mycar, unsigned char *buf)
{
	char sql_buf[BUFSIZE];
	unsigned int buf_index, buf_len;
	unsigned char gsm_ip[20];
	unsigned char cmd_seq=0, gsm_signal;
	unsigned int  record_seq,adc_voltage;

	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;
	unsigned long sqlrow_cnt = 0 ;

	//input: HEAD+SEQ+PCB+LEN+DATA+CHK
	//DATA: Record_seq(2 bytes)+GSM signal(1byte)+voltage(2 bytes)+IP
	//IP(123.123.123.123) 31 32 33 2E 31 32 33 2E 31 32 33 2E 31 32 33
	//i.e:  C9 01 52 00 14 FF FF 1C 0F FF 31 32 33 2E 31 32 33 2E 31 32 33 2E 31 32 33 73 
	cmd_seq = buf[1];
	buf_len = buf[3] << 8 | buf[4];

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		;//fprintf(stderr, "%02X ",*(buf+buf_index));
	}
	//fprintf(stderr, "\r\nLen: %d ",buf_len);

	record_seq = buf[5] << 8 | buf[6];
	//fprintf(stderr, "\trecord_seq: %d ",record_seq);

	gsm_signal = buf[7] ;
	//fprintf(stderr, "\tSingal: %d ",gsm_signal);

	adc_voltage = buf[8] << 8 | buf[9];
	//fprintf(stderr, "\tadc_voltage: %d\r\n",adc_voltage);


	for ( buf_index = 0 ; buf_index < buf_len - 5 ; buf_index++ ) {
		gsm_ip[buf_index] = *(buf+10+buf_index) ;
		//fprintf(stderr, "buf_index=%d\t%X\r\n",buf_index,gsm_ip[buf_index]);
	}
	gsm_ip[buf_index]  = 0x0; 
	gsm_ip[15] = 0x0; //ip length < 15

	if ( debug_flag ) {
		fprintf(stderr, "GSM IP: %s\r\n",gsm_ip);
	}

	//insert GSM IP and signal to table t_log_signal:
	snprintf(sql_buf,BUFSIZE-1,"insert into t_log_signal values ( '',\
								'%d',\
								'%s',\
								'%s',\
								'%d',\
								'%s',\
								'%d',\
								'%d',\
								'%d');",\
								time(NULL),\
								mycar->sn,\
								(char *)inet_ntoa(mycar->client_addr.sin_addr),\
								ntohs(mycar->client_addr.sin_port),\
								gsm_ip,\
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
		if ( debug_flag ) {
			fprintf(stderr, "insert new GSM IP&Signal ok.\n");
		}
	}

	//prevent error: 2014
	res_ptr=mysql_store_result(&(mycar->mydb.mysql));
	mysql_free_result(res_ptr);

	return 0;

}
