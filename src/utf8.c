// vim: cin:sts=4:sw=4 

#include <string.h>
#include "utf8.h"

int utf8_count_chars(char* const text) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; char* iter = text; ssize_t r = 0; ssize_t l = strlen(text);

    do {
	r = utf8proc_iterate(iter,l,&unichar); if (r == 0) return i;
	i++; iter += r; l -= r;
    } while (r > 0);

    return i;
}

int tweet_count_chars(char* const text) {

    char* new = utf8proc_NFC(text);

    int r = utf8_count_chars(new);

    free(new);

    return r;


}
