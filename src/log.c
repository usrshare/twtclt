// vim: cin:sts=4:sw=4 

#include "globals.h"
#include "log.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

int lprintf(const char *fmt, ...)
{
    if ((logfile == NULL) && (use_curses)) {
	logfile = fopen("twtclt.log","a");
	time_t ct = time(NULL);
	fprintf(logfile,"--twtclt started on %s\n",ctime(&ct));
    }

    va_list args;
    va_start(args, fmt);

    if (use_curses) {
	fprintf(logfile,fmt,args);
	fflush(logfile);
    } else printf(fmt,args);
    return 0;
}

void lperror(const char *s) {

    //this is not a thread-safe function, I know.
	
    if ((s == NULL) || (*s == '\0'))
	lprintf("%s\n",strerror(errno));
    else
	lprintf("%s: %s\n",s,strerror(errno));
    return;
}
