// vim: cin:sts=4:sw=4 

#include <string.h>
#include "utf8.h"

int utf8char_in_set(int32_t uc, int32_t* set, int32_t setlen) {

    for (int i=0; i<setlen; i++)
	if (set[i] == uc) return i;

    return -1;
}

int utf8_wrap_text(const char* in, char* out, size_t maxlen, uint8_t width, int32_t* delimiters, int delcount) {
    
    //TODO: Writes a copy of UTF-8 text in _in_, wrapped to _width_ chars per line, into _out_.

    char res[maxlen];
    int column=0, colbyte=0; char* lastdelim = (char *) in; const uint8_t* iter = (const uint8_t *) in; int r = 0; size_t l = strlen(in); int32_t uc=0;

    do {
	r = utf8proc_iterate(iter,l,&uc);

	if (utf8char_in_set(uc,delimiters,delcount) != -1) {

	    // is a delimiter character
	    lastdelim = (char*) iter;


	} else {

	    // is a regular character

	}

	column++;

	if (column == width) {

	    column=0;
	    colbyte=0;
	}

	iter+=r;
	colbyte+=r;


    } while (r != 0);
     

    return 0;
}

int utf8_count_chars(const char* text) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; const uint8_t* iter = (const uint8_t *) text; ssize_t r = 0; ssize_t l = strlen(text);

    do {
	r = utf8proc_iterate(iter,l,&unichar); if (r == 0) return i;
	i++; iter += r; l -= r;
    } while (r > 0);

    return i;
}

int tweet_count_chars(const char* text) {

    char* new = (char*)utf8proc_NFC((const uint8_t*)text);

    int r = utf8_count_chars(new);

    free(new);

    return r;

}

char* point_to_char_by_idx(const char* text, int idx) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; const uint8_t* iter = (const uint8_t *) text; ssize_t r = 0; ssize_t l = strlen(text);

    while (i != idx) {
	r = utf8proc_iterate(iter,l,&unichar); if (r == 0) return NULL;
	i++; iter += r; l -= r;
    }

    return iter;

}
