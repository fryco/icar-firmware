//$URL$ 
//$Rev$, $Date$

#include "config.h"
#include "cloud_post.h"

extern int foreground, listen_port;

//return 0: ok, others: err
int cloud_tcpclient_create(cloud_tcpclient *pclient,const char *host, int port){

	struct hostent *he;

	if(pclient == NULL) return 1;
	memset(pclient,0,sizeof(cloud_tcpclient));

	if((he = gethostbyname(host))==NULL){
		return 2;
	}

	pclient->remote_port = port;
	strcpy(pclient->remote_ip,(unsigned char * )inet_ntoa( *((struct in_addr *)he->h_addr) ));

	pclient->_addr.sin_family = AF_INET;
	pclient->_addr.sin_port = htons(pclient->remote_port);
	pclient->_addr.sin_addr = *((struct in_addr *)he->h_addr);

	if((pclient->socket = socket(AF_INET,SOCK_STREAM,0))==-1){
		return 3;
	}

	/*TODO:是否应该释放内存呢?*/

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

	len = strlen(post)+strlen(host)+strlen(header2)+strlen(content_len)+strlen(request)+1;
	lpbuf = (char*)malloc(len);
	if(lpbuf==NULL){
		return -1;
	}

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
	//fprintf(stderr,"Send req:\n%s\n",lpbuf);

	/*释放内存*/
	if(lpbuf != NULL) free(lpbuf);
	lpbuf = NULL;

	/*it's time to recv from server*/
	if(cloud_tcpclient_recv(pclient,&lpbuf,0) <= 0){
		if(lpbuf) free(lpbuf);
		return -2;
	}

	//fprintf(stderr,"Rec respond:\n%s\n",lpbuf);

	/*响应代码,|HTTP/1.0 200 OK|
	 *从第10个字符开始,第3位
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
	ptmp += 4;/*跳过\r\n*/

	len = strlen(ptmp)+1;
	*response=(char*)malloc(len);
	if(*response == NULL){
		if(lpbuf) free(lpbuf);
		return -1;
	}
	memset(*response,0,len);
	memcpy(*response,ptmp,len-1);

	/*从头域找到内容长度,如果没有找到则不处理*/
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

	char *response = NULL;

	return 0; //for test only
	
	cloud_tcpclient_create(&client,remote_host,port);

	if(http_post(&client,remote_host,"/cn0086_bbs/mach_post.php",request,&response)){
		fprintf(stderr,"Error! check %s:%d\n",__FILE__, __LINE__);
		return 1;
	}

	if ( foreground ) {
		;//fprintf(stderr,"Cloud Res: %s\n",response);
	}

	free(response);
	return 0;
}
