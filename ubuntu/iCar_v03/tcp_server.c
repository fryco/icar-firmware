//$URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/tcp_server.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>  
#include <linux/in.h>
#include <signal.h>  
#include <time.h> 
#include "database.h"
#include "commands.h"
#include "misc.h"

#define BACKLOG 5        //Maximum queue of pending connections  

#define iCar_RELEASE "\niCar server v03, built by cn0086@139.com at " __DATE__" "__TIME__ "\n" 

/* Flag indicating whether debug mode is on.  */
int debug_flag = 0;

unsigned char icar_db_host[]="localhost";
unsigned char icar_db_user[]="root";
unsigned char icar_db_pwd[]="cn0086";
unsigned char icar_db_name[]="icar_v03";

static int sock_server = -1;

void sig_proccess_server(int signo)  
{  
	close(sock_server);
	fprintf(stderr, "\nClose socket and exit.\n");  
	exit(1);  
}  

static void usage(void)
{
	fprintf(stderr, "%s\n", iCar_RELEASE );
	fprintf(stderr,
"usage: server [-h] [-d] \n"
"              [-p port] \n"
"              default port is 23\n\n"
	);
	exit(1);
}


void process_conn_server(struct icar_data *mycar)
{  
	ssize_t size = 0;
	unsigned char recv_buf[BUFSIZE];
	unsigned char send_buf[BUFSIZE];
	unsigned int buf_index ;
	unsigned int chk_count ;
	time_t ticks=time(NULL); 
	struct tm *tblock;
	int cmd_err_cnt = 0 ;//command error count
	struct icar_command cmd ;

	unsigned char var_u8 ;

	memset(cmd.pro_sn, 0x0, 10);
	mycar->sn = cmd.pro_sn ;

	if ( debug_flag ) {
		fprintf(stderr, "\n%sConnect: %s:%d\n",\
				(char *)ctime(&ticks),(char *)inet_ntoa(mycar->client_addr.sin_addr),\
				ntohs(mycar->client_addr.sin_port));
	}

	//connect mysql
	if(db_connect(&(mycar->mydb)))
	{//failure
		syslog(LOG_INFO, "Database no ready, exit.");
		fprintf(stderr,  "Database no ready, exit.\n");
		fprintf(stderr,  "Check: 1, Have install mysql?\n");
		fprintf(stderr,  "       2, host, user, password, database are correct?\n");
		return ;
	}

		//while ( 1 ) 
		{//for test only, send current time continue
			memset(send_buf, '\0', BUFSIZE);  
			snprintf(send_buf,100,"%.24s Welcome to CQT server\r\n",(char *)ctime(&ticks));
			write(mycar->client_socket,send_buf,strlen(send_buf));
		}

	while ( 1 ) {

		//read the content from remote site
		memset(recv_buf, '\0', BUFSIZE);

		//以下基于假设：每次读取1个或更多包，不会收到不完整包
		//HEAD+SEQ+PCB+Length, please refer to: iCar protocol_通讯协议
		size = read(mycar->client_socket, recv_buf, BUFSIZE); 
		if (size == 0 || size > BUFSIZE) { //no data or overflow
			return;
		}

		ticks=time(NULL);
		fprintf(stderr, "\r\n%.24s  Rec:%02d Bytes\r\n",(char *)ctime(&ticks),size-1);

		//find the HEAD flag: 0xC9
		for ( buf_index = 0 ; buf_index < size ; buf_index++ ) {
			//fprintf(stderr, "Index  %d = %d \r\n",buf_index,recv_buf[buf_index]);
			if ( recv_buf[buf_index] == GSM_HEAD ) { //found first HEAD : 0xC9
				fprintf(stderr, "\r\n");
				cmd.len = recv_buf[buf_index+3] << 8 | recv_buf[buf_index+4];

				//calc the chk byte:
				cmd.chk = GSM_HEAD ;
				for ( chk_count = 1 ; chk_count < cmd.len+5 ; chk_count++) {
					cmd.chk ^= recv_buf[buf_index+chk_count] ;
				}

				if ( (buf_index + cmd.len) > (size-6) \
					|| cmd.chk != recv_buf[buf_index+cmd.len+5] ) { //illegal package
					fprintf(stderr, "\r\nIllegal package: ");
					for ( chk_count = 0 ; chk_count < cmd.len+5 ; chk_count++) {
						fprintf(stderr, "%02X ",recv_buf[buf_index+chk_count]);
					}
					fprintf(stderr, "\r\nLen= %d\tRec chk= 0x%02X\tCal chk= 0x%02X\r\n",\
							cmd.len,recv_buf[buf_index+cmd.len+5],cmd.chk);
					fprintf(stderr, "Check %s, line: %d\r\n",__FILE__, __LINE__);
				}
				else {
					cmd.seq = recv_buf[buf_index+1];
					cmd.pcb = recv_buf[buf_index+2];
					fprintf(stderr, "cmd seq=0x%X pcb=0x%X len=%d\t",\
									cmd.seq,cmd.pcb,cmd.len);

					if ( debug_flag ) {
						fprintf(stderr, "At %d CMD: %c Len:%d\r\n",buf_index,cmd.pcb,cmd.len);
					}
					//handle the input cmd
					switch (cmd.pcb) {

					case GSM_ASK_IST://0x3F, '?':
						cmd_ask_ist( mycar,&cmd,\
							&recv_buf[buf_index], send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_ERROR: //0x45,'E' Error, upload error log
						cmd_err_log( mycar,&cmd,\
							&recv_buf[buf_index], send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					
					case 0x51://'Q' Quit
						if ( debug_flag ) {
							fprintf(stderr, "CMD is Quit, disconnect...\n");
						}
						goto exit_process_conn_server;

					case GSM_CMD_RECORD: //0x52, 'R' Record GSM signal,adc ...
						cmd_rec_signal( mycar,&cmd,\
							&recv_buf[buf_index], send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_SN: //0x53, 'S', upload SN to server
						if ( cmd_sn_upload( mycar,&cmd,\
							&recv_buf[buf_index], send_buf ) == 1 ) {

							goto exit_process_conn_server;
						}
						buf_index = buf_index + cmd.len ;//update index
						break;

					
					case GSM_CMD_TIME: //0x54, 'T', Get server Time
						cmd_get_time( mycar,&cmd,\
							&recv_buf[buf_index], send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware
						cmd_upgrade_fw( mycar,&cmd,\
							&recv_buf[buf_index], send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					default:
						fprintf(stderr, "Unknow command: 0x%X\r\n",cmd.pcb);
			
						cmd_err_cnt++;
						break;
					}//end of handle the input cmd
					
					if ( cmd_err_cnt > 3 ) {
						goto exit_process_conn_server;
					}
				}//end of if (i + cmd.len) > (size-4)

				buf_index = buf_index + 5 ;//take HEAD(1),SEQ(1),PCB(1),LEN(2)...+CHK
			}//end of if ( recv_buf[buf_index] == 0xC9 )
		}
	}

exit_process_conn_server:

	mysql_close(&(mycar->mydb.mysql));
}

/*
 * Main program for the daemon.
 */
int main(int ac, char **av)
{
	extern char *optarg;
	extern int optind;
	int opt, err;
	
	const char *remote_ip;
	char *test_user = NULL, *test_host = NULL, *test_addr = NULL;
	int remote_port;
	int listen_port = 23;  
	struct sockaddr_in server_addr;
	int flag=1,flag_len=sizeof(int);
	pid_t pid;
	struct icar_data mycar ;

	signal(SIGINT, sig_proccess_server);

	/* Parse command-line arguments. */
	while ((opt = getopt(ac, av, "hdp:")) != -1) {
		switch (opt) {
		case 'p':
			listen_port = a2port(optarg);
			if (listen_port <= 0) {
				fprintf(stderr, "Bad port number, the port must: 1 ~ 65535\n");
				exit(1);
			}
			break;

		case 'd':
				debug_flag = 1;
				fprintf(stderr, "Debug mode is on.\n");
			break;

		case 'h':
			usage();
			break;

		case '?':
			usage();

		default:
			break;
		}
	}

	openlog(av[0], LOG_PID | LOG_NDELAY, LOG_DAEMON);
	syslog(LOG_INFO, "iCar server start, listen port is: %d",listen_port);
	fprintf(stderr,  "iCar server start, listen port is: %d\r\n",listen_port);

	//Check database
	mycar.mydb.db_host=icar_db_host;
	mycar.mydb.db_user=icar_db_user;
	mycar.mydb.db_pwd =icar_db_pwd;
	mycar.mydb.db_name=icar_db_name;
	if ( db_check(&mycar) ) {//no ready
		syslog(LOG_INFO, "Database no ready, exit.");
		fprintf(stderr,  "Database no ready, exit.\n");
		fprintf(stderr,  "Check: 1, Have install mysql?\n");
		fprintf(stderr,  "       2, host, user, password, database are correct?\n");
		return -1;
	}

	sock_server = socket(AF_INET, SOCK_STREAM, 0);  
	if (sock_server < 0) {
		syslog(LOG_ERR, "Socket error, check %s:%d",__FILE__, __LINE__);
		fprintf(stderr, "Socket error, check %s:%d\n",__FILE__, __LINE__);
		return -1;  
	}

	//set server info
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(listen_port);

	/*
	 * Set socket options.
	 * Allow local port reuse in TIME_WAIT.
	 */
	if (setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR,
			&flag, flag_len) == -1) {
		syslog(LOG_ERR, "setsockopt SO_REUSEADDR error, check %s:%d",\
										__FILE__, __LINE__);
		fprintf(stderr, "setsockopt SO_REUSEADDR error, check %s:%d\n",\
										__FILE__, __LINE__);
	}

	err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
	if (err < 0) {
		syslog(LOG_ERR, "bind error, check %s:%d",__FILE__, __LINE__);
		fprintf(stderr, "bind port: %d error, please try other port.\n",listen_port);
		return -1;  
	}

	err = listen(sock_server, BACKLOG);
	if (err < 0) {
		syslog(LOG_ERR, "listen error, check %s:%d",__FILE__, __LINE__);
		fprintf(stderr, "Listen error, check %s:%d\n",__FILE__, __LINE__);
		return -1;  
	}

	for ( ;; ) {
		int addrlen = sizeof(struct sockaddr);
		
		//Accept new connection
		mycar.client_socket = accept(sock_server, (struct sockaddr *)&mycar.client_addr, &addrlen);
		if (mycar.client_socket < 0) {
			syslog(LOG_ERR, "accept new connection error, check %s:%d",\
											__FILE__, __LINE__);
			continue; //error, stop this connection
		}

		//Record this connection
		if ( debug_flag ) {
			syslog(LOG_INFO, "New connection, from %s:%d",\
							(char *)inet_ntoa(mycar.client_addr.sin_addr),\
							ntohs(mycar.client_addr.sin_port));
			fprintf(stderr, "PID:%ld accept new connection from: %s:%d\n",\
							(long) getpid(),(char *)inet_ntoa(mycar.client_addr.sin_addr),\
							ntohs(mycar.client_addr.sin_port));
		}

		//Create new process for this connection
		pid = fork();
		if (pid == 0) { //In new process
			close(sock_server);
			if ( debug_flag ) {
				syslog(LOG_INFO, "Create new process:%d for new connection",getpid());
				//fprintf(stderr, "New process:%ld sock_client=%d, %s:%d\n",\
				//								(long) getpid(),sock_client,__FILE__, __LINE__);
			}
			process_conn_server(&mycar);
			if ( debug_flag ) {
				syslog(LOG_INFO, "Process:%d complete, disconnect: %s:%d",\
								getpid(),(char *)inet_ntoa(mycar.client_addr.sin_addr),\
								ntohs(mycar.client_addr.sin_port));
				fprintf(stderr, "Process:%d complete, disconnect: %s:%d\n",\
								getpid(),(char *)inet_ntoa(mycar.client_addr.sin_addr),\
								ntohs(mycar.client_addr.sin_port));
			}
			close(mycar.client_socket);
			break;
		}
		else {
			syslog(LOG_WARNING, "accept new connection, but create new process error, check %s:%d",\
													__FILE__, __LINE__);
			if ( debug_flag ) {
				fprintf(stderr, "fork failure. check %s:%d\n",__FILE__, __LINE__);
			}
			close(mycar.client_socket);
		}
	}

	if ( debug_flag ) {
		fprintf(stderr, "PID:%ld exit.\n",(long) getpid());
	}
  return 0 ;
}


