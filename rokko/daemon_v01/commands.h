/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/internal/rokko/commands.h $ 
  * @version $Rev: 38 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-06 09:43:04 +0800 (Sun, 06 Jan 2013) $
  * @brief   This file is for commands
  ******************************************************************************
  */ 

#ifndef _COMMANDS_H
#define _COMMANDS_H

#define MAX_FW_SIZE				61450 //60*1024+10
#define MIN_FW_SIZE				40960 //40*1024

unsigned char rec_cmd_login(struct rokko_data *, struct rokko_command *,\
				unsigned char *, unsigned char * );


unsigned char snd_cmd( struct rokko_data *, unsigned char *, unsigned char * );
unsigned char snd_cmd_upgrade( struct rokko_data *, unsigned char *, unsigned char * );
#endif /* _COMMANDS_H */
