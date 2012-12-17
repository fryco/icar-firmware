/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/database.h $ 
  * @version $Rev: 99 $
  * @author  $Author$
  * @date    $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
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
