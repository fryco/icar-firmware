/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon_v01/smtp.h $ 
  * @version $Rev: 355 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2013-04-10 18:43:02 +0800 (Wed, 10 Apr 2013) $
  * @brief   This file is for send email operate
  ******************************************************************************
  */ 

#ifndef _SMTP_H
#define _SMTP_H

// Function prototype

unsigned char smtp_send(char *smtp_server, 
						unsigned int smtp_port, char *mail_to, 
						char *mail_subject, char *mail_body, char *err_str);
#endif /* _SMTP_H */
