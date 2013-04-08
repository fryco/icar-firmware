/**
 *      rokkod - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */

#ifndef _ROKKOD_H 
#define _ROKKOD_H

struct rokko_db {
	//database
	MYSQL mysql;
	unsigned char * db_host;
	unsigned char * db_name;
	unsigned char * db_user;
	unsigned char * db_pwd;
};

struct rokko_command {
	unsigned char seq;//sequence
	unsigned char pcb;//protocol control byte
	unsigned short len;//length;
	unsigned char *inf;//Information Field
	unsigned short crc16;//crc16 result
};


//记录已发命令，如果收到响应，就清除记录
//否则超时(CMD_TIMEOUT), 重发，详见config.h
struct rokko_send_queue {
	unsigned int timer;//cancel CMD if time > 5*CMD_TIMEOUT
	unsigned char confirm; 
	unsigned char seq;
	unsigned char pcb;
};

struct rokko_data { //as minimum as possible
	//socket
	struct sockaddr_in client_addr;
	int client_socket ;

	unsigned char active_time; //close socket if timeout
	unsigned char cmd_err_cnt;//command error count
	
	unsigned char pro_sn[12];//product serial number, Max. char(10)
	unsigned short hw_rev;
	unsigned short fw_rev;
	unsigned int rx_cnt;//receive total bytes
	unsigned int tx_cnt;//transmit total bytes
	unsigned int con_time;//connect time
	//int  err_code;
	//char err_msg[BUFSIZE+1];
	
	struct rokko_send_queue send_q[MAX_CMD_QUEUE];
};


//记录已发命令，如果收到响应，就清除记录
//否则超时(CMD_TIMEOUT), 重发，详见config.h
struct SENT_QUEUE {
	unsigned int send_timer;//cancel CMD if time > 5*CMD_TIMEOUT, 
							//can be improved to char, reduce size, TBD
	unsigned char confirm; 
	unsigned char send_seq;
	unsigned char send_pcb;
};

int main(int, char *[]);
void bg(void);
void scan_args(int, char *[]);
void print_help(char *[]);
void print_version(void);
void period_check( struct rokko_data *, unsigned int);
unsigned char sock_init( unsigned int );
unsigned char daemon_server(struct rokko_data *, unsigned char *, unsigned short);

#endif /* _ROKKOD_H */
