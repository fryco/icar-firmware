/* misc.c */

#include "config.h"
#include "misc.h"

#define INVALID 	1
#define TOOSMALL 	2
#define TOOLARGE 	3

#ifndef LLONG_MIN
	#define LLONG_MIN     (-LLONG_MAX-1)
#endif

#ifndef LLONG_MAX
	#define LLONG_MAX    9223372036854775807LL
#endif

static const int FSHIFT = 16;           /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

extern unsigned int daemon_time;

long long
strtonum(const char *numstr, long long minval, long long maxval,
    const char **errstrp)
{
	long long ll = 0;
	char *ep;
	int error = 0;
	struct errval {
		const char *errstr;
		int err;
	} ev[4] = {
		{ NULL,		0 },
		{ "invalid",	EINVAL },
		{ "too small",	ERANGE },
		{ "too large",	ERANGE },
	};

	ev[0].err = errno;
	errno = 0;
	if (minval > maxval)
		error = INVALID;
	else {
		ll = strtoll(numstr, &ep, 10);
		if (numstr == ep || *ep != '\0')
			error = INVALID;
		else if ((ll == LLONG_MIN && errno == ERANGE) || ll < minval)
			error = TOOSMALL;
		else if ((ll == LLONG_MAX && errno == ERANGE) || ll > maxval)
			error = TOOLARGE;
	}
	if (errstrp != NULL)
		*errstrp = ev[error].errstr;
	errno = ev[error].err;
	if (error)
		ll = 0;

	return (ll);
}

/*
 * Convert ASCII string to TCP/IP port number.
 * Port must be >=0 and <=65535.
 * Return -1 if invalid.
 */
int
a2port(const char *s)
{
	long long port;
	const char *errstr;

	port = strtonum(s, 0, 65535, &errstr);
	if (errstr != NULL)
		return -1;
	return (int)port;
}

void ticks2time(unsigned int ostime, unsigned char *up_buf, unsigned int buf_len)
{
	unsigned int updays, uphours, upminutes, seconds;
	unsigned char tmp[EMAIL];
	
	bzero( up_buf, buf_len);bzero( tmp, sizeof(tmp));
	
	updays = ostime / (60*60*24*100);
	if (updays) {
		sprintf(tmp,"%d day%s, ", updays, (updays != 1) ? "s" : "");
		strcat(up_buf, tmp);
	}
	upminutes = ostime / (60*100);
	uphours = (upminutes / (60*100)) % 24;
	upminutes %= 60;
	if(uphours) {
		sprintf(tmp,"%2d:%02d ", uphours, upminutes);
		strcat(up_buf, tmp);
	}
	else {
		sprintf(tmp,"%d min ", upminutes);
		strcat(up_buf, tmp);
	}
	seconds = ostime - (updays*60*60*24*100) - uphours*60*60*100 - upminutes*60*100;
	sprintf(tmp,"%d.%d sec\n", seconds/100,seconds%100);
	strcat(up_buf, tmp);

	return ;
}

void get_sysinfo( char *buf, int buf_len )
{
	/* 用于进制转换的常量。*/
	const double megabyte = 1024 * 1024;
	
	char tmp[BUFSIZE] ;
	int user_cnt = 0;
	struct utmp* entry;
 
	unsigned int updays, uphours, upminutes, seconds, up_time;
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;

	time(&current_secs);
	current_time = localtime(&current_secs);

	//bzero( info, sizeof(info));
	info.freeram = 0, info.totalram = 0 ;
	sysinfo(&info);

	sprintf(buf, "%2d:%02d, Server up ", current_time->tm_hour, current_time->tm_min);
	
	bzero( tmp, sizeof(tmp));
	updays = (int) info.uptime / (60*60*24);
	if (updays) {
		sprintf(tmp,"%d day%s, ", updays, (updays != 1) ? "s" : "");
		strcat(buf, tmp);
	}
	upminutes = (int) info.uptime / 60;
	uphours = (upminutes / 60) % 24;
	upminutes %= 60;
	seconds = (int) info.uptime - (updays*60*60*24) - uphours*60*60 - upminutes*60;
	
	if(uphours) {
		sprintf(tmp,"%2d:%02d:%02d, ", uphours, upminutes,seconds);
		strcat(buf, tmp);
	}
	else {
		sprintf(tmp,"%02d:%02d, ", upminutes, seconds);
		strcat(buf, tmp);
	}
		
	sprintf(tmp,"load avg: %ld.%02ld, %ld.%02ld, %ld.%02ld. ", 
			LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]), 
			LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]), 
			LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));
	strcat(buf, tmp);
	
	sprintf (tmp,"Total RAM: %5.1f MB, free RAM: %5.1f MB, ",\
		info.totalram*info.mem_unit / megabyte,\
		info.freeram*info.mem_unit / megabyte);
	
	strcat(buf, tmp);
	sprintf (tmp,"process cnt: %d\n\n", info.procs);
	strcat(buf, tmp);

	if ( strlen(buf) > (buf_len<<1) ) {//buf too long
		sprintf (buf,"Buf too long, check %s:%d\n", __FILE__,__LINE__);
	}

	setutent();
    while ((entry = getutent()) != NULL)
    {
        if ( (entry->ut_type == USER_PROCESS) && (entry->ut_name[0] != '\0') ) {
        	user_cnt++;
        }
    }
    sprintf(tmp,"%d user%s\n", user_cnt, (user_cnt > 1 ? "s" : ""));
	strcat(buf, tmp);
	endutent();
		
	setutent();
	user_cnt = 0;
    while ((entry = getutent()) != NULL)
    {

        if ( (entry->ut_type == USER_PROCESS) && (entry->ut_name[0] != '\0') ) {
        	user_cnt++;
	        sprintf(tmp,"%d: %s, from: %s, time: %s", user_cnt, entry->ut_user,
               entry->ut_host, ctime((const time_t *)&entry->ut_tv));
			strcat(buf, tmp);
        }
    }
	endutent();

	if ( strlen(buf) > (buf_len - 50) ) {//buf too long
		sprintf (buf,"Buf too long, check %s:%d\n", __FILE__,__LINE__);
	}

	up_time = (time(NULL) - daemon_time);
	strcat(buf, "\nDaemon up: ");
	updays = up_time / (60*60*24);
	if (updays) {
		sprintf(tmp,"%d day%s, ", updays, (updays != 1) ? "s" : "");
		strcat(buf, tmp);
	}
	upminutes = up_time / 60;
	uphours = (upminutes / 60) % 24;
	upminutes %= 60;
	seconds = up_time - (updays*60*60*24) - uphours*60*60 - upminutes*60;
	
	sprintf(tmp,"%2d:%02d:%02d\n", uphours, upminutes,seconds);
	strcat(buf, tmp);

	if ( strlen(buf) > (buf_len - 2) ) {//buf too long
		sprintf (buf,"Buf too long, check %s:%d\n", __FILE__,__LINE__);
	}
	
	strcat(buf, "\n");
}
