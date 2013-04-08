/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/cloud_post.h $ 
  * @version $Rev: 302 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-12-11 13:31:08 +0800 (Tue, 11 Dec 2012) $
  * @brief   This file is for cloud_post
  ******************************************************************************
  */ 

#ifndef _CLOUD_POST_H 
#define _CLOUD_POST_H

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
