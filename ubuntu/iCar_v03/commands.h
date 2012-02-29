/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
  * @brief   This file is for commands
  ******************************************************************************
  */ 

#ifndef _COMMANDS_H
#define _COMMANDS_H

//For GSM <==> Server protocol, need to same as STM32 firmware define
#define GSM_HEAD				0xC9
#define GSM_ASK_IST				0x3F //'?', Ask instruction from server
#define GSM_CMD_ERROR			0x45 //'E', upload error log to server
#define GSM_CMD_RECORD			0x52 //'R', record gsm/adc data
#define GSM_CMD_SN				0x53 //'S', upload SN
#define GSM_CMD_TIME			0x54 //'T', time
#define GSM_CMD_UPGRADE			0x55 //'U', Upgrade firmware

int cmd_ask_ist(struct icar_data *, struct icar_command *,\
				unsigned char *, unsigned char * );

int cmd_err_log(struct icar_data *, struct icar_command *,\
				unsigned char *, unsigned char * );

int cmd_err_signal(struct icar_data *, struct icar_command *,\
				unsigned char *, unsigned char * );

#endif /* _COMMANDS_H */
