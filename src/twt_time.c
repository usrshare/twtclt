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

    //takes a time and outputs the difference (or day if too large) in outstr. outstr must hold at least 12 characters.

    char res[12];
    memset(&res,0,8);

    int value = 0; char* append = NULL; int norel = 0; /* int r = 0; */

    time_t curtime = time(NULL);

    time_t timediff = curtime - tweettime;

    struct tm *noreltime = gmtime(&tweettime);

    if (timediff < 60) { value = timediff; append = "s";} else
	if (timediff < 3600) { value = timediff / 60; append = "m";} else
	    if (timediff < 3600*24) { value = timediff / 3600; append = "h";} else
		if (timediff < 3600*24*7) { value = timediff / 3600 / 24; append = "d";} else norel=1;

    if (norel) { strftime(res,12,"%d %b %Y",noreltime); } else {
	snprintf(res,12,"%d%s",value,append);
    }

    strncpy(outstr,res,12);
    return 0;
}
