/**
 *      log - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon/v00/log.h $
 *      $Rev: 315 $, $Date: 2012-12-17 19:39:33 +0800 (Mon, 17 Dec 2012) $
 */

#ifndef _LOG_H 
#define _LOG_H

int log_init( char* path );
int log_err( char* );
int log_save( FILE *, char* );

#endif /* _LOG_H */
