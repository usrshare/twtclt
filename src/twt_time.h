// vim: cin:sts=4:sw=4 

#include <time.h>

#ifndef _TWT_TIME_H_
#define _TWT_TIME_H_

time_t gettweettime(const char* created_at_str);

int reltimestr(time_t tweettime, char* outstr);
// TODO: outputs relative time values in Twitter's format "5m","11h" into a string.

#endif
