/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
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
