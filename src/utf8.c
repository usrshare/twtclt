// vim: cin:sts=4:sw=4 

#include <string.h>
#include "utf8.h"

int utf8_count_chars(const char* text) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; const uint8_t* iter = (const uint8_t*) text; ssize_t r = 0; ssize_t l = strlen(text);

    do {
	r = utf8proc_iterate(iter,l,&unichar); if (r == 0) return i;
	i++; iter += r; l -= r;
    } while (r > 0);

    return i;
}

int tweet_count_chars(const char* text) {

    uint8_t* new = utf8proc_NFC((const uint8_t*) text);

    int r = utf8_count_chars((const char*)new);

    free(new);

    return r;

}

char* point_to_char_by_idx(const char* text, int idx) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; char* iter = (char *) text; ssize_t r = 0; ssize_t l = strlen(text);

    while (i != idx) {
	r = utf8proc_iterate((const uint8_t *)iter,l,&unichar); if (r == 0) return NULL;
	i++; iter += r; l -= r;
    }

    return iter;

}
