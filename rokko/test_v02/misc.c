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

void ticks2time(unsigned int ostime, unsigned char *up_buf)
{
	unsigned int updays, uphours, upminutes, seconds;
	unsigned char tmp[EMAIL];
	
	bzero( up_buf, EMAIL);bzero( tmp, sizeof(tmp));
	
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
