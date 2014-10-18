// vim: cin:sts=4:sw=4 

#include <mojibake.h>

#ifndef _UTF8_H_
#define _UTF8_H_

int utf8_count_chars(const char* text);

int tweet_count_chars(const char* text);

char* point_to_char_by_idx(const char* text, int idx);

#endif
