// vim: cin:sts=4:sw=4 
#include <stdio.h>
#include <string.h>
#include "utf8.h"

const int32_t delimiters[5] = {32,0x2c,0x2e,0x21,0x3f};
const int32_t linebreaks[2] = {10,13};

int utf8char_in_set(int32_t uc, const int32_t* set, int32_t setlen) {

    for (int i=0; i<setlen; i++)
	if (set[i] == uc) return i;

    return -1;
}

int utf8_wrap_text(const char* in, char* out, size_t maxlen, uint8_t width) {
    
    //TODO: Writes a copy of UTF-8 text in _in_, wrapped to _width_ chars per line, into _out_.

    char res[maxlen];

    res[0] = '\0';

    char* strend = (char*) in + strlen(in);

    int column=0, colbyte=0;
    char* lastcol = (char*) in;
    char* lastdelim = NULL;
    char* endline = (char*) in;
    const uint8_t* iter = (const uint8_t *) in;
    int r = 0;
    size_t l = strlen(in);
    int32_t uc=0;

    int bytesleft = (maxlen-4);

    do {
	r = utf8proc_iterate(iter,l,&uc);

	if (utf8char_in_set(uc,delimiters,5) != -1) {
	    // is a delimiter character
	    lastdelim = (char*) iter; //word_ _test
	    endline = lastdelim+r; //   word_t_est
	} else if (utf8char_in_set(uc,linebreaks,2) != -1) {
	    // is a line break.
	    column = 0;
	}
	
	column++;
	iter+=r;
	colbyte+=r;
	l = strend - (char *) iter;

	if (column == width) {

	    int tocopy = (lastdelim ? (endline-lastcol) : colbyte);

	    if (bytesleft < (tocopy+1)) { tocopy = bytesleft-1; r = 0; }
	    
	    strncat(res,lastcol,tocopy);
	    if (r) strncat(res,"\n",1);

	    bytesleft -= (tocopy + 1 );

	    if (lastdelim) iter = (const uint8_t *)endline;
	    lastdelim = NULL;

	    lastcol = (char* ) iter;
	    column=0;
	    colbyte=0;
	    
	    if (r == 0) { strcat(res,"…"); /*3 bytes */ }
	}
    } while (r > 0);
    
    if (bytesleft >= colbyte+1) strncat(res,lastcol,colbyte);
 
    printf("%s\n",res);


    return 0;
}

int utf8_count_chars(const char* text) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; const uint8_t* iter = (const uint8_t *) text; ssize_t r = 0; ssize_t l = strlen(text);

    do {
	r = utf8proc_iterate(iter,l,&unichar); if (r <= 0) return i;
	i++; iter += r; l -= r;
    } while (r > 0);

    return i;
}

int tweet_count_chars(const char* text) {

    char* new = (char*)utf8proc_NFC((const uint8_t*)text);

    int r = utf8_count_chars((const char*)new);

    free(new);

    return r;

}

char* point_to_char_by_idx(const char* text, int idx) {

    if (text == NULL) return 0;

    int i = 0; int32_t unichar = -1; const uint8_t* iter = (const uint8_t *) text; ssize_t r = 0; ssize_t l = strlen(text);

    while (i != idx) {
	r = utf8proc_iterate((const uint8_t *)iter,l,&unichar); if (r <= 0) return NULL;
	i++; iter += r; l -= r;
    }

    return (uint8_t *)iter;

}
