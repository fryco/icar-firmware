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

unsigned char log_init( char * path );
unsigned char log_save( char *, unsigned char );
int log_err( char* );

#endif /* _LOG_H */
