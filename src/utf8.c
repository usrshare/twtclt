// vim: cin:sts=4:sw=4 
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include "utf8.h"

const int32_t spaces[25] = {0x09,0x0a,0x0b,0x0c,0x0d,0x20,0x85,0xa0,0x1680,0x2000,0x2001,0x2002,0x2003,0x2004,0x2005,0x2006,0x2007,0x2008,0x2009,0x200a,0x2028,0x2029,0x202f,0x205f,0x3000};
const int32_t delimiters[4] = {32,0x2e,0x21,0x3f};
const int32_t linebreaks[2] = {10,13};

int utf8char_in_set(int32_t uc, const int32_t* set, int32_t setlen) {

    for (int i=0; i<setlen; i++)
	if (set[i] == uc) return i;

    return -1;
}

int utf8_test() {

    char text[] = "Привет! This is a test of UTF-8 text ０１２３４５６７８９ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ！゛＃＄％＆（）＊＋、ー。／：；〈＝＝〉？＠［\\］＾＿‘｛｜｝～\n";

    char test[1000];

    utf8_wrap_text(text, test, 1000, 30);	

    printf("%s\n",test);

    return 0;
}

ssize_t utf8_strnlen(const uint8_t* in, size_t maxlen) {
    //returns a number of bytes in _in_ that contain enough full characters to fit into maxlen bytes.

    int r=0,l=strlen((const char*)in); const uint8_t* iter = in; size_t n=0; int32_t uc=0;
    do {
	r = utf8proc_iterate(iter,l,&uc);

	if (n + r > maxlen) return n; else n+=r;
	iter += r;
	l -= r;

    } while (r >= 1);

    return n;
}

int utf8_text_size(const char* in, int* width, int* height) {

    int column = 0;

    int maxwidth=0, lines=1;

    const uint8_t* iter = (const uint8_t *) in;

    int r = 0, l = strlen(in); int32_t uc;

    do {
	r = utf8proc_iterate(iter,l,&uc);

	if (utf8char_in_set(uc,linebreaks,2) != -1) {
	    lines++;
	    if (column > maxwidth) maxwidth = column;
	    column = 0;
	    // is a line break.
	} else {

	    wchar_t thiswc = (wchar_t)uc;
	    int ucwidth = wcwidth(thiswc);
	    column += ucwidth;
	}

	iter+=r; l-=r;
    } while (r > 0);

    *width = maxwidth;
    *height = lines;

    return -1; 
}

int utf8_wrap_text(const char* in, char* out, size_t maxlen, uint8_t width) {

    //TODO: Writes a copy of UTF-8 text in _in_, wrapped to _width_ chars per line, into _out_.

    if (out == NULL) return -1;

    char res[maxlen];

    res[0] = '\0';

    char* strend = (char*) in + strlen(in);

    int column=0, colbyte=0; int linebroken=0;
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

	if (utf8char_in_set(uc,delimiters,4) != -1) {

	    // is a delimiter character
	    lastdelim = (char*) iter; //word_ _test
	    endline = lastdelim+r; //   word_t_est
	} else if (utf8char_in_set(uc,linebreaks,2) != -1) {
	    // is a line break.
	    column = 0; linebroken = 1;
	}

	wchar_t thiswc = (wchar_t)uc;

	int ucwidth = wcwidth(thiswc);
	//printf("Character 'U+%X' has width %d\n",thiswc,ucwidth);

	if (column + ucwidth > width) { column = width;} else {

	    if ((utf8char_in_set(uc,spaces,25) == -1) || (column != 0)) { column += ucwidth; colbyte+=r; } else if (!linebroken) lastcol = (char*)iter+r; //will not add char if line starts with a space.

	    iter+=r;
	    //colbyte+=r;
	    l = strend - (char *) iter; }

	if (column >= width) {

	    ssize_t tocopy = (lastdelim ? (endline-lastcol) : colbyte);

	    if (bytesleft < (tocopy+1)) { tocopy = bytesleft-1; r = 0; }

	    tocopy = utf8_strnlen((const uint8_t*)lastcol,tocopy);

	    strncat(res,lastcol,tocopy);
	    if (r) strncat(res,"\n",1);

	    bytesleft -= (tocopy + 1 );

	    if (lastdelim) iter = (const uint8_t *)endline;
	    lastdelim = NULL;

	    lastcol = (char* ) iter;
	    column=0;
	    linebroken=0;
	    colbyte=0;

	    if (r == 0) { strcat(res,"…"); /*3 bytes */ }
	}
    } while (r > 0);

    if (bytesleft >= colbyte+1) strncat(res,lastcol,colbyte);

    assert(strlen(res) < maxlen);

    strncpy(out,res,strlen(res)+1);

    return strlen(res)+1;
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

    return (char *)iter;

}
