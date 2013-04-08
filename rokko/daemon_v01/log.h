/**
 *      log - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: svn://svn.cn0086.info/icar/internal/rokko/log.h $
 *      $Rev: 82 $, $Date: 2013-01-17 02:07:03 +0800 (Thu, 17 Jan 2013) $
 */

#ifndef _LOG_H 
#define _LOG_H

unsigned char log_init( char * path );
unsigned char log_save( char *, unsigned char );
int log_err( char* );

#endif /* _LOG_H */
