/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://rokko-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.h $ 
  * @version $Rev: 99 $
  * @author  $Author$
  * @date    $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
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

#endif /* _COMMANDS_H */
