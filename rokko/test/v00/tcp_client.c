#include "config.h"

static const unsigned char cmd_login[ ] = 
{ 
	0xC9, 0x01, 0x53, 0x00, 0x1B, 0x00, 0x00, 0x0D, 0x99, 0x30,
	0x32, 0x50, 0x31, 0x43, 0x30, 0x44, 0x32, 0x41, 0x37, 0x31,
	0x30, 0x2E, 0x32, 0x30, 0x31, 0x2E, 0x31, 0x33, 0x37, 0x2E,
	0x32, 0x37, 0x28
};


static const unsigned char cmd_time[ ] = 
{ 
	0xC9, 0x00, 0x54, 0x00, 0x0A, 0x30, 0x32, 0x50, 0x31, 0x31,
	0x41, 0x48, 0x30, 0x30, 0x30, 0xFC
};

int	main(int argc, char	*argv[])
{
	int	client_sockfd;
	unsigned int var_int, len;
	struct sockaddr_in remote_addr;	//�������������ַ�ṹ��
	unsigned char var_char, buf[BUFSIZE];  //���ݴ��͵Ļ�����

	memset(&remote_addr,0,sizeof(remote_addr));	//���ݳ�ʼ��--����
	remote_addr.sin_family=AF_INET;	//����ΪIPͨ��
	remote_addr.sin_addr.s_addr=inet_addr( SERVER_ADDR );//������IP��ַ
	remote_addr.sin_port=htons( SERVER_PORT ); //�������˿ں�


	if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		return 1;
	}


	if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect");
		return 1;
	}
	printf("connected to server: %s:%d\n\n",SERVER_ADDR,SERVER_PORT);
	//len=recv(client_sockfd,buf,BUFSIZE,0);//���շ���������Ϣ
	len = read(client_sockfd, buf, BUFSIZE); buf[len]='\0';
	printf("%s",buf); //��ӡ����������Ϣ

	for ( var_char = 0 ; var_char < CMD_CNT ; var_char++ ) {
		write(client_sockfd,cmd_login,cmd_login[4]+6);
		printf("--> Login CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
		
		len = read(client_sockfd, buf, BUFSIZE);
		printf("<-- %d Bytes:",len);
		for ( var_int = 0 ; var_int < len ; var_int++ ) {
			printf(" %02X",buf[var_int]);
		}
		printf("\n\n");
		
	
		write(client_sockfd,cmd_time,cmd_time[4]+6);
		printf("--> Time CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
		
		len = read(client_sockfd, buf, BUFSIZE);
		printf("<-- %d Bytes:",len);
		for ( var_int = 0 ; var_int < len ; var_int++ ) {
			printf(" %02X",buf[var_int]);
		}
		printf("\n\n");
	}
	
	printf("Close socket and quit.\n");
	close(client_sockfd);//�ر��׽���

	return 0;
}
