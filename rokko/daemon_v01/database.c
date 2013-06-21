//$URL$ 
//$Rev$, $Date$

#include "config.h"
#include "rokkod.h"

extern unsigned int foreground;

//Connect to MYSQ
//return: 0 success
//        1 failure
int db_connect(struct db_struct *rokko_db)
{
	int err;
	char create_db[EMAIL];

	if ( foreground ) {
		fprintf(stderr, "Initializing mysql ... \n");
	}

	if ( !mysql_init(&(rokko_db->mysql)) )
	{
		if ( foreground ) {
			fprintf(stderr, "Initializing mysql failure, return\n");
		}
		return 1;
	}
	
	if (!mysql_real_connect(&(rokko_db->mysql),\
							rokko_db->db_host,\
							rokko_db->db_user,\
							rokko_db->db_pwd,\
							rokko_db->db_name,\
							0,NULL,0))	{
		err = mysql_errno(&(rokko_db->mysql)) ;
		//fprintf(stderr, "Mysql thread_id %d... \n",rokko_db->mysql.thread_id);
		if ( err == 1049 ) { //Unknown database, create it
			if (!mysql_real_connect(&(rokko_db->mysql),
							rokko_db->db_host,\
							rokko_db->db_user,\
							rokko_db->db_pwd,\
							"mysql",0,NULL,0)) {
			//connect with default database:mysql still failure
				return 1 ;
			}

			bzero( create_db, sizeof(create_db));
			snprintf(create_db,EMAIL,"create database %s;",rokko_db->db_name);
			//fprintf(stderr, "%s %s:%d\n",create_db,__FILE__,__LINE__);
			if ( foreground ) {
				fprintf(stderr, "%s\n",create_db);
			}
			mysql_query(&(rokko_db->mysql),create_db);

			bzero( create_db, sizeof(create_db));
			snprintf(create_db,EMAIL,"use %s;",rokko_db->db_name);
			mysql_query(&(rokko_db->mysql),create_db);
		}
		else { //others error
			fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(rokko_db->mysql)));
			return 1;
		}
	}

	//本次连接使用的默认字符集为utf8
	mysql_query( &(rokko_db->mysql) , "SET NAMES 'utf8'");

	if ( foreground ) {
		fprintf(stderr, "Connecte to mysql ok, Mysql thread_id %d\n",rokko_db->mysql.thread_id);
	}

	return 0 ;
}

int db_check(struct db_struct *rokko_db)
{
	int err;
	MYSQL_RES *res_ptr;

	//connect to mysql
	if(db_connect(rokko_db)){ //failure
		return 1;
	}

	//check table rokko_product
	if ( mysql_query(&(rokko_db->mysql),"SELECT * FROM `rokko_product` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(rokko_db->mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(rokko_db->mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(rokko_db->mysql),\
						"create table rokko_product( `p_id` int unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY,\
						`produce_date` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
						`IMEI` varchar(20) NOT NULL COMMENT 'Product serial numble',\
						`IMSI` varchar(20) ,\
						`intl` smallint unsigned COMMENT 'The international telephone code',\
						`phone_number` varchar(12),\
						`MCU_ID` varchar(12) NOT NULL COMMENT 'MCU ID',\
						`hw_rev` tinyint unsigned NOT NULL COMMENT 'hardware revision',\
						`fw_rev` smallint unsigned NOT NULL COMMENT 'firmware revision',\
						`update` int unsigned NOT NULL COMMENT 'update firmware date',\
						`encrypt_ver` tinyint unsigned NOT NULL COMMENT 'encryption version'\
					) ENGINE=MyISAM DEFAULT CHARSET=gbk;");
			
			//insert test data
			mysql_query(&(rokko_db->mysql),\
			"insert into `rokko_product` VALUES ('','1321609867',\
											'99775533110000',\
											'',\
											'',\
											'',\
											'E32473143B22',\
											'1',\
											'168',\
											'1339444768',\
											'0');");

		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(rokko_db->mysql));
			return 1 ;
		}
	}

	//check table rokko_gps
	//prevent error: 2014
	res_ptr=mysql_store_result(&(rokko_db->mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(rokko_db->mysql),"SELECT * FROM `rokko_gps` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(rokko_db->mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(rokko_db->mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(rokko_db->mysql),\
			"create table rokko_gps( `gps_id` int unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY,\
						`IMEI` varchar(20) NOT NULL COMMENT 'Product serial numble', \
						`updated` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
                   		`latitude` int unsigned NOT NULL,\
                   		`S_N` enum('S', 'N') NOT NULL COMMENT 'South or North',\
                   		`longitude` int unsigned NOT NULL,\
                   		`E_W` enum('E', 'W') NOT NULL COMMENT 'East or West',\
                   		`speed` tinyint unsigned NOT NULL COMMENT 'Max. 255km/h',\
						`status` smallint unsigned NOT NULL COMMENT 'High 4 bits for SAT number, low 10 bits for angle'\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");

			//insert test data
			mysql_query(&(rokko_db->mysql),\
			"insert into `rokko_gps` VALUES ('','99775533110000',\
											'1321609887',\
											X'E4322E01',\
											'N',\
											X'A4BB1201',\
											'E',\
											'22',\
											X'700C');");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(rokko_db->mysql));
			return 1 ;
		}
	}

	//check table rokko_gps2
	//prevent error: 2014
	res_ptr=mysql_store_result(&(rokko_db->mysql));
	mysql_free_result(res_ptr);
	if ( mysql_query(&(rokko_db->mysql),"SELECT * FROM `rokko_gps2` LIMIT 0 , 30;") ) {
	//error, maybe no this table, create it
		err = mysql_errno(&(rokko_db->mysql)) ;
		fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(rokko_db->mysql)));
		if ( err == 1146 ) { //Table doesn't exist
			mysql_query(&(rokko_db->mysql),\
			"create table rokko_gps2( `gps_id` int unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY,\
						`IMEI` varchar(20) NOT NULL COMMENT 'Product serial numble', \
						`updated` int unsigned NOT NULL COMMENT 'FROM_UNIXTIME(date)',\
                   		`latitude` decimal(8,4) NOT NULL,\
                   		`S_N` enum('S', 'N') NOT NULL COMMENT 'South or North',\
                   		`longitude` decimal(9,4) NOT NULL,\
                   		`E_W` enum('E', 'W') NOT NULL COMMENT 'East or West',\
                   		`speed` tinyint unsigned NOT NULL COMMENT 'Max. 255km/h',\
						`status` smallint unsigned NOT NULL COMMENT 'High 4 bits for SAT number, low 10 bits for angle'\
                   ) ENGINE=MyISAM DEFAULT CHARSET=gbk;");

			//insert test data
			mysql_query(&(rokko_db->mysql),\
			"insert into `rokko_gps2` VALUES ('','99775533110000',\
											'1321609887',\
											'2232.3837',\
											'N',\
											'11402.7712',\
											'E',\
											'22',\
											X'700C');");
		}
		else { //unknow error
			fprintf(stderr, "select table error, check %s:%d\n",__FILE__, __LINE__);
			mysql_close(&(rokko_db->mysql));
			return 1 ;
		}
	}

	//mysql_close(&(rokko_db->mysql));

	return 0 ;
}
