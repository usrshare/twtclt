// vim: cin:sts=4:sw=4 
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "twt_time.h"

time_t gettweettime(const char* created_at_str) {
    /* gets tweet's time into time_t. */
    struct tm tweettime;
    memset(&tweettime,0,sizeof tweettime);
	strptime(created_at_str,"%a %b %d %T %z %Y",&tweettime); //twitter's date format.

    time_t res = timegm(&tweettime); //thanks to this one function, this program is now BSD/GNU-only.
    return res;
};

int reltimestr(time_t tweettime, char* outstr) {

    //takes a time and outputs the difference (or day if too large) in outstr. outstr must hold at least 8 characters.

    char res[8];
    memset(&res,0,8);

    int value = 0; char* append = NULL; int norel = 0, noval = 0;

    time_t curtime = time(NULL);

    time_t timediff = curtime - tweettime;

    struct tm *noreltime = gmtime(&tweettime);

    if (timediff < 60) { value = timediff; noval = 1; append = "<1m";} else
	if (timediff < 3600) { value = timediff / 60; append = "m";} else
	    if (timediff < 3600*24) { value = timediff / 3600; append = "h";} else norel = 1;

    if (norel) { strftime(res,8,"%d %b",noreltime); } else {

	if (noval) snprintf(res,8,"%s",append); else snprintf(res,8,"%d%s",value,append);

    }

    strncpy(outstr,res,8);
    return 0;
}
