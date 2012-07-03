/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.h $ 
  * @version $Rev: 99 $
  * @author  $Author$
  * @date    $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
  * @brief   This file is for cloud_post
  ******************************************************************************
  */ 

#ifndef _CLOUD_TCP_CLIENT_ 
#define _CLOUD_TCP_CLIENT_

typedef struct _cloud_tcpclient{ 
    int socket;
    int remote_port;     
    char remote_ip[16];  
    struct sockaddr_in _addr; 
    int connected;       
} cloud_tcpclient;

int cloud_tcpclient_create(cloud_tcpclient *,const char *host, int port);
int cloud_tcpclient_conn(cloud_tcpclient *);
int cloud_tcpclient_recv(cloud_tcpclient *,char **lpbuff,int size);
int cloud_tcpclient_send(cloud_tcpclient *,char *buff,int size);
int cloud_tcpclient_close(cloud_tcpclient *);

#endif /* _CLOUD_POST_H */
