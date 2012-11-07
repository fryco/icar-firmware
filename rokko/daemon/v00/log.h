/**
 *      log - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */

#ifndef _LOG_H 
#define _LOG_H

int log_init( char* path );
int log_err( char* );
int log_save1( FILE *, char* );

#endif /* _LOG_H */
