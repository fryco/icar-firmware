//$URL: https://icar-firmware.googlecode.com/svn/rokko/daemon_v01/cloud_post.c $ 
//$Rev: 355 $, $Date: 2013-04-10 18:43:02 +0800 (Wed, 10 Apr 2013) $

#include "config.h"
#include "cloud_post.h"

int foreground = 1;

//return 0: ok, others: err
int cloud_tcpclient_create(cloud_tcpclient *pclient,const char *host, int port){

	struct hostent *he;
	char **pptr;

	if(pclient == NULL) return 1;
	memset(pclient,0,sizeof(cloud_tcpclient));

	if((he = gethostbyname(host))==NULL){
		fprintf(stderr,"gethostbyname error for host:%s\n", host);
		return 2;
	}

	// show all the alias name
	for(pptr = he->h_aliases; *pptr != NULL; pptr++) {
		fprintf(stderr, "Alias:%s\n",*pptr);
	}

	pclient->remote_port = port;
	pclient->_addr.sin_family = AF_INET;
	pclient->_addr.sin_port = htons(pclient->remote_port);
	pclient->_addr.sin_addr = *((struct in_addr *)he->h_addr);

	if((pclient->socket = socket(AF_INET,SOCK_STREAM,0))==-1){
		return 3;
	}

	/*TODO:�Ƿ�Ӧ���ͷ��ڴ���?*/

	return 0;
}

int cloud_tcpclient_conn(cloud_tcpclient *pclient){
	if(pclient->connected)
		return 1;

	if(connect(pclient->socket, (struct sockaddr *)&pclient->_addr,sizeof(struct sockaddr))==-1){
		return -1;
	}

	pclient->connected = 1;

	return 0;
}

int cloud_tcpclient_recv(cloud_tcpclient *pclient,char **lpbuff,int size){
	int recvnum=0,tmpres=0;
	char buff[BUFSIZE];

	*lpbuff = NULL;

	while(recvnum < size || size==0){
		tmpres = recv(pclient->socket, buff,BUFSIZE,0);
		if(tmpres <= 0)
			break;
		recvnum += tmpres;

		if(*lpbuff == NULL){
			*lpbuff = (char*)malloc(recvnum);
			if(*lpbuff == NULL)
				return -2;
		}else{
			*lpbuff = (char*)realloc(*lpbuff,recvnum);
			if(*lpbuff == NULL)
				return -2;
		}

		memcpy(*lpbuff+recvnum-tmpres,buff,tmpres);
	}

	return recvnum;
}

int cloud_tcpclient_send(cloud_tcpclient *pclient,char *buff,int size){
	int sent=0,tmpres=0;

	while(sent < size){
		tmpres = send(pclient->socket,buff+sent,size-sent,0);
		if(tmpres == -1){
			return -1;
		}
		sent += tmpres;
	}
	return sent;
}

int cloud_tcpclient_close(cloud_tcpclient *pclient){
	close(pclient->socket);
	pclient->connected = 0;
}

int http_post(cloud_tcpclient *pclient,const char *remote_host, char *page,char *request,char **response){

	char post[300],host[100],content_len[100];
	char *lpbuf,*ptmp;
	int len=0;

	lpbuf = NULL;
	const char *header2="User-Agent: Mozilla/4.0 Http 0.1\r\nCache-Control: no-cache\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept: */*\r\n";

	sprintf(post,"POST %s HTTP/1.0\r\n",page);
	sprintf(host,"HOST: %s:%d\r\n",remote_host,pclient->remote_port);
	sprintf(content_len,"Content-Length: %d\r\n\r\n",strlen(request));

	len = strlen(post)+strlen(host)+strlen(header2)+strlen(content_len)+strlen(request)+2;
	lpbuf = (char*)malloc(len);
	if(lpbuf==NULL){
		return -1;
	}
	//fprintf(stderr,"Apply %d buf ok.\n",len);
	bzero( lpbuf, len);
	
	strcpy(lpbuf,post);
	strcat(lpbuf,host);
	strcat(lpbuf,header2);
	strcat(lpbuf,content_len);
	strcat(lpbuf,request);

	if(!pclient->connected){
		cloud_tcpclient_conn(pclient);
	}

	if(cloud_tcpclient_send(pclient,lpbuf,len)<0){
		return -1;
	}
	fprintf(stderr,"Send req:\n%s\n",lpbuf);

	/*�ͷ��ڴ�*/
	if(lpbuf != NULL) free(lpbuf);
	lpbuf = NULL;

	/*it's time to recv from server*/
	if(cloud_tcpclient_recv(pclient,&lpbuf,0) <= 0){
		if(lpbuf) free(lpbuf);
		return -2;
	}

	//fprintf(stderr,"Rec respond:\n%s\n",lpbuf);

	/*��Ӧ����,|HTTP/1.0 200 OK|
	 *�ӵ�10���ַ���ʼ,��3λ
	 * */
	memset(post,0,sizeof(post));
	strncpy(post,lpbuf+9,3);
	if(atoi(post)!=200){
		if(lpbuf) free(lpbuf);
		return atoi(post);
	}


	ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
	if(ptmp == NULL){
		free(lpbuf);
		return -3;
	}
	ptmp += 4;/*����\r\n*/

	len = strlen(ptmp)+2;
	*response=(char*)malloc(len);
	if(*response == NULL){
		if(lpbuf) free(lpbuf);
		return -1;
	}
	memset(*response,0,len);
	memcpy(*response,ptmp,len);

	/*��ͷ���ҵ����ݳ���,���û���ҵ��򲻴���*/
	ptmp = (char*)strstr(lpbuf,"Content-Length:");
	if(ptmp != NULL){
		char *ptmp2;
		ptmp += 15;
		ptmp2 = (char*)strstr(ptmp,"\r\n");
		if(ptmp2 != NULL){
			memset(post,0,sizeof(post));
			strncpy(post,ptmp,ptmp2-ptmp);
			if(atoi(post)<len)
				(*response)[atoi(post)] = '\0';
		}
	}

	if(lpbuf) free(lpbuf);

	return 0;
}

int cloud_post( char *remote_host, char *request, int port )
{  
	cloud_tcpclient client;
	int ret;
	char *response = NULL;	

	cloud_tcpclient_create(&client,remote_host,port);
	
	if(http_post(&client,remote_host,"/service/equipment/?",request,&response)){
		fprintf(stderr,"Error! check %s:%d\n",__FILE__, __LINE__);
		return 1;
	}

	ret = strncmp(response,"1000",4);
	if ( foreground ) {
		fprintf(stderr,"\nReturn: %s\t%d\n",response,ret);
	}
	
	free(response);
	return ret;
}

//http://yun.test.33xuexi.com/service/equipment/?sn=1111&password=123456&..
	
int main(int argc, char *argv[]) {
	
	unsigned char post_buf[BUFSIZE], host_info[BUFSIZE], logtime[EMAIL];
	unsigned int i;
	struct timeval log_tv;
	unsigned int usecs=100000;

	bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));

	bzero(logtime, sizeof(logtime));
	gettimeofday(&log_tv,NULL);
	strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));

	//ģ���ն����кţ�9977553311 (10λ�̶�ǰ��) + xxxxx (5λ�����)
	for ( i = 740 ; i < 100000 ; i++ ) {
		snprintf(post_buf,BUFSIZE,"sn=9977553311%05u&password=123456\r\n",i);
		//snprintf(post_buf,BUFSIZE,"sn=259&password=123456");
		//post to cloud
		if ( cloud_post( CLOUD_HOST, post_buf, 80 ) != 0 ) {//error
			fprintf(stderr,"%s\nErr, exit.\n",post_buf);
			//exit( 0 );
			usleep(usecs*3);
		}
		usleep(usecs);
	}

	exit( 0 );
}
