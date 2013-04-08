/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/internal/rokko/smtp.h $ 
  * @version $Rev: 38 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-06 09:43:04 +0800 (Sun, 06 Jan 2013) $
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
