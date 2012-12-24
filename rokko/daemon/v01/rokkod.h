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

struct rokko_data {
	//socket
	struct sockaddr_in client_addr;
	int client_socket ;

	struct rokko_db mydb;

	unsigned char * sn;
	unsigned char hw_rev;
	unsigned int fw_rev;
	int  err_code;
	char err_msg[BUFSIZE+1];
};

struct rokko_command {
	unsigned char seq;//sequence
	unsigned char pcb;//protocol control byte
	unsigned char len;//length;
	unsigned char *inf;//Information Field
	unsigned char chk;//^ result
	unsigned char pro_sn[10];//product serial number
};

int main(int, char *[]);
void bg(void);
void scan_args(int, char *[]);
void print_help(char *[]);
void print_version(void);
void period_check( FILE * );
unsigned char sock_init( unsigned int );
void daemon_server(struct rokko_data *);

#endif /* _ROKKOD_H */
