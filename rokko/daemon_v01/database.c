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

	//fprintf(stderr, "DB connect %s:%d\n",__FILE__,__LINE__);
	
	if (!mysql_real_connect(&(rokko_db->mysql),\
							rokko_db->db_host,\
							rokko_db->db_user,\
							rokko_db->db_pwd,\
							rokko_db->db_name,\
							0,NULL,0))	{
		err = mysql_errno(&(rokko_db->mysql)) ;
		if ( err == 1049 ) { //Unknown database, create it
			if (!mysql_real_connect(&(rokko_db->mysql),
							rokko_db->db_host,\
							rokko_db->db_user,\
							rokko_db->db_pwd,\
							"mysql",0,NULL,0)) {
			//connect with default database:mysql still failure
				return 1 ;
			}

			if ( foreground ) {
				fprintf(stderr, "Create database icar_v03 ...\n");
			}
			mysql_query(&(rokko_db->mysql),"create database icar_v03;");
			mysql_query(&(rokko_db->mysql),"use icar_v03;");
		}
		else { //others error
			fprintf(stderr, "Error: %d, meaning:%s\r\n", err,mysql_error(&(rokko_db->mysql)));
			return 1;
		}
	}

	//本次连接使用的默认字符集为utf8
	mysql_query( &(rokko_db->mysql) , "SET NAMES 'utf8'");

	if ( foreground ) {
		;//fprintf(stderr, "Connectiong to mysql ok.\n");
	}

	return 0 ;
}
