/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/database.h $ 
  * @version $Rev: 99 $
  * @author  $Author$
  * @date    $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
  * @brief   This file is for database operate
  ******************************************************************************
  */ 

#ifndef _DATABASE_H
#define _DATABASE_H

#include <mysql.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024*2

//log message length, same as main.c in STM32 firmware
#define MAX_LOG_LENGTH			64

#define RESULT_NO_ERR			0
#define RESULT_NO_SN			1
#define RESULT_INS_DB_FAIL		2

/* database.c */
struct icar_db {
	//database
	MYSQL mysql;
	unsigned char * db_host;
	unsigned char * db_name;
	unsigned char * db_user;
	unsigned char * db_pwd;
};

struct icar_data {
	//socket
	struct sockaddr_in client_addr;
	int client_socket ;

	struct icar_db mydb;

	unsigned char * sn;
	int  err_code;
	char err_msg[BUFSIZE];
};

struct icar_command {
	unsigned char seq;//sequence
	unsigned char pcb;//protocol control byte
	unsigned char len;//length;
	unsigned char *inf;//Information Field
	unsigned char chk;//^ result
	unsigned char pro_sn[10];//product serial number
};

//return 0:ok, 1:failure
int db_check(struct icar_data *);

//return 0:ok, 1:failure
int db_connect(struct icar_db *);

int check_sn(struct icar_data * );

int ask_instruction(struct icar_data *,unsigned char *, unsigned char *);
int record_command(struct icar_data *, unsigned char *, unsigned char *, unsigned int);
int record_signal(struct icar_data *, unsigned char *);
int record_ip(struct icar_data *, unsigned char *);
int record_error(struct icar_data *, unsigned char *, unsigned char *);

//error code:
//10:  iCar serial numble error.
//20:  GSM IP&Signal buffer length error.

#endif /* _DATABASE_H */
