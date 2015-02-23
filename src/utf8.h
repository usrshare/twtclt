// vim: cin:sts=4:sw=4 

#include <mojibake.h>

#ifndef _UTF8_H_
#define _UTF8_H_

struct textsize {
    int width;
    int height;
};

int utf8_test();

int utf8char_in_set(int32_t uc, const int32_t* set);

ssize_t utf8_strnlen(const uint8_t* in, size_t maxlen);

int utf8_text_size(const char* in, int* width, int* height);

int utf8_wrap_text(const char* in, char* out, size_t maxlen, uint8_t width);

int utf8_count_chars(const char* text);

int tweet_count_chars(const char* text);

char* point_to_char_by_idx(const char* text, int idx);

int utf8_append_char(int32_t uc, char* string, size_t maxbytes);
int utf8_remove_last(char* text);

#endif
