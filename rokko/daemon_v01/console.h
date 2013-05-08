/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon_v01/commands.h $ 
  * @version $Rev: 351 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2013-04-08 11:58:33 +0800 (Mon, 08 Apr 2013) $
  * @brief   This file is for console
  ******************************************************************************
  */ 

#ifndef _CONSOLE_H
#define _CONSOLE_H

int console_list_all( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf, \
				struct rokko_data *rokko_all, unsigned int conn_amount );

int console_list_spe( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf, \
				struct rokko_data *rokko_all, unsigned int conn_amount );

#endif /* _CONSOLE_H */
