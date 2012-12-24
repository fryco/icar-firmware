/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
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
