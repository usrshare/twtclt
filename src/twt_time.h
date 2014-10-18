#include <time.h>

#ifndef _TWT_TIME_H_
#define _TWT_TIME_H_

time_t gettweettime(const char* created_at_str);

int reltimestr(time_t tweettime, char* outstr, size_t maxlen);
// TODO: outputs relative time values in Twitter's format "5m","11h" into a string.

#endif
