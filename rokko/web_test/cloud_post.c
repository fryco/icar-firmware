//$URL$ 
//$Rev$, $Date$

#include "config.h"
#include "cloud_post.h"

int foreground = 1;

//return 0: ok, others: err
int cloud_tcpclient_create(cloud_tcpclient *pclient,const char *host, int port){

	struct hostent *he;
	char **pptr;

	if(pclient == NULL) return 1;
	memset(pclient,0,sizeof(cloud_tcpclient));

/*
	if((he = gethostbyname(host))==NULL){
		fprintf(stderr,"gethostbyname error for host:%s\n", host);
		return 2;
	}

	// show all the alias name
	for(pptr = he->h_aliases; *pptr != NULL; pptr++) {
		fprintf(stderr, "Alias:%s\n",*pptr);
	}
*/
	pclient->remote_port = port;
	pclient->_addr.sin_family = AF_INET;
	pclient->_addr.sin_port = htons(pclient->remote_port);
	//pclient->_addr.sin_addr = *((struct in_addr *)he->h_addr);
	pclient->_addr.sin_addr.s_addr = inet_addr( "42.121.6.226" );

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
			fprintf(stderr,"Send buf failure! return: %d @ %d\n",tmpres,__LINE__);
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
		fprintf(stderr,"Apply %d buf @ %d Failure!\n",len,__LINE__);
		return -1;
	}
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
		fprintf(stderr,"Send %d buf @ %d Failure!\n",len,__LINE__);
		return -1;
	}
	fprintf(stderr,"Send req:\n%s\n",lpbuf);

	/*释放内存*/
	if(lpbuf != NULL) free(lpbuf);
	lpbuf = NULL;

	/*it's time to recv from server*/
	if(cloud_tcpclient_recv(pclient,&lpbuf,0) <= 0){
		if(lpbuf) free(lpbuf);
		fprintf(stderr,"Rec buf @ %d Failure!\n",__LINE__);
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
		fprintf(stderr,"Free buf @ %d Failure!\n",len,__LINE__);
		return -3;
	}
	ptmp += 4;/*跳过\r\n*/

	len = strlen(ptmp)+2;
	*response=(char*)malloc(len);
	if(*response == NULL){
		if(lpbuf) free(lpbuf);
		return -1;
	}
	memset(*response,0,len);
	memcpy(*response,ptmp,len);

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
	int ret;
	char *response = NULL;	

	cloud_tcpclient_create(&client,remote_host,port);
	
	//http://yun.test.33xuexi.com/service/state/?
	if(http_post(&client,remote_host,"/service/state/?",request,&response)){
		fprintf(stderr,"Error! check %s:%d\n",__FILE__, __LINE__);
		return 1;
	}

/*
	if(http_post(&client,remote_host,"/service/equipment/?",request,&response)){
		fprintf(stderr,"Error! check %s:%d\n",__FILE__, __LINE__);
		return 1;
	}
*/
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
	unsigned int i, lat, lon;
	unsigned int lat_ori=0, lon_ori=0, lat_offset=0, lon_offset=0 ;
	unsigned int lat_degree, lat_minute, lat_second;
	unsigned int lon_degree, lon_minute, lon_second;

	struct timeval log_tv;
	unsigned int usecs=50000;

	bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));

	bzero(logtime, sizeof(logtime));
	gettimeofday(&log_tv,NULL);
	strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));

	lat_ori = (24*60+((getpid())&0x3F))*30000;
	lon_ori = (114*60+((getpid())&0x3F))*30000;

	while ( 1 ) {//for test upload data
		lat = lat_ori + lat_offset ;	lat_offset = lat_offset + 33*((getpid())&0x3F) ;
		lon = lon_ori + lon_offset ;	lon_offset = lon_offset + 47*((getpid())&0x7F) ;

		lat_degree = lat/(60*30000);
		lat_minute = (lat - lat_degree*60*30000)/30000;
		lat_second = (lat - lat_degree*60*30000 - lat_minute*30000)/3;

		lon_degree = lon/(60*30000);
		lon_minute = (lon - lon_degree*60*30000)/30000;
		lon_second = (lon - lon_degree*60*30000 - lon_minute*30000)/3;

		snprintf(post_buf,BUFSIZE,"sn=997755331160687&longitude=%03d%02d.%04d&latitude=%02d%02d.%04d&voltage=12.4",\
									lon_degree,lon_minute,lon_second,\
									lat_degree,lat_minute,lat_second);

		//post to cloud
		if ( cloud_post( CLOUD_HOST, post_buf, 80 ) != 0 ) {//error
			fprintf(stderr,"%s\nErr, exit.\n",post_buf);
			//exit( 0 );
			usleep(usecs*60);
		}
		usleep(usecs);	
	}
	
	//For test initial SN
	//模拟终端序列号：9977553311 (10位固定前续) + xxxxx (5位随机数)
	//470 ==> 10726
	//10726 ==> 15722
	//15722 ==> 20718
	//20718 ==> 25714
	//25714 ==> 30710
	//30710 ==> 35706
	//40702 ==> 45698
	//45698 ==> 50694
	//50694 ==> 55690
	for ( i = 55691 ; i < 100000 ; i++ ) {
		snprintf(post_buf,BUFSIZE,"sn=9977553311%05u&password=123456\r\n",i);
		//snprintf(post_buf,BUFSIZE,"sn=259&password=123456");
		//post to cloud
		if ( cloud_post( CLOUD_HOST, post_buf, 80 ) != 0 ) {//error
			fprintf(stderr,"%s\nErr, exit.\n",post_buf);
			//exit( 0 );
			usleep(usecs*60);
		}
		usleep(usecs);
	}

	exit( 0 );
}
