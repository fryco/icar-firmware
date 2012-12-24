//$URL$
//$Rev$, $Date$

#include "config.h"
#include "tcp_server.h"


extern int sock_server;
extern struct sockaddr_in server_addr;

//return 0: ok, others: err
unsigned char sock_init( unsigned int port )
{	
	int flag=1, err, flag_len=sizeof(int);
	
	//Create sotcket
	sock_server = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_server < 0) {
		printf("Socket error, check %s:%d\n",__FILE__, __LINE__);
		return 10;
	}

	//set server info
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	/* Set socket options: Allow local port reuse in TIME_WAIT.	 */
	if (setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR,
			&flag, flag_len) == -1) {
		printf("setsockopt SO_REUSEADDR error, check %s:%d\n",\
										__FILE__, __LINE__);
	}


	err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err < 0) {
		printf("Bind port: %d error, please try other port.\n",port);
		return 20;
	}

	err = listen(sock_server, BACKLOG);
	if (err < 0) {
		printf("Listen error, check %s:%d\n",__FILE__, __LINE__);
		return 30;
	}
	
	return 0;
}
