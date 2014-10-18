// vim: cin:sts=4:sw=4 

#include <string.h>

#include "twt_time.h"

time_t gettweettime(const char* created_at_str) {
	/* gets tweet's time into time_t. */
	struct tm tweettime;
	memset(&tweettime,0,sizeof tweettime);
	strptime(created_at_str,"%a %b %d %T %z %Y",&tweettime); //twitter's date format.
	time_t res = mktime(&tweettime);
	return res;
};
