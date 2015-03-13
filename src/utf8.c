// vim: cin:sts=4:sw=4 
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include "utf8.h"

const int32_t spaces[] = {0x20,0x85,0xa0,0x1680,0x2000,0x2001,0x2002,0x2003,0x2004,0x2005,0x2006,0x2007,0x2008,0x2009,0x200a,0x2028,0x2029,0x202f,0x205f,0x3000,0};
const int32_t delimiters[] = {32,0};
const int32_t linebreaks[] = {10,13,0};

int utf8char_in_set(int32_t uc, const int32_t* set) {

    int i=0;
    while (set[i] != 0) {
	if (set[i] == uc) return i;
	i++;
    }

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

	if ((r <= 0) || (utf8char_in_set(uc,linebreaks) != -1)) {
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
    return utf8_wrap_text2(in,out,maxlen,width,NULL);
}

int utf8_wrap_text2(const char* in, char* out, size_t maxlen, uint8_t width, int* loc_char) {

    //TODO: Writes a copy of UTF-8 text in _in_, wrapped to _width_ chars per line, into _out_.

    if (out == NULL) return -1;

    int lc=0, lcp=0;

    if (loc_char) lc=*loc_char;

    char res[maxlen];

    memset(res,0,maxlen);

    char* strend = (char*) in + strlen(in);

    int column=0, colbyte=0; int linebroken=0;
    char* lastcol = (char*) in;
    char* lastdelim = NULL;
    char* endline = (char*) in;
    const uint8_t* iter = (const uint8_t *) in;
    int r = 0;
    int32_t uc=0;
    int ind = 0;

    int bytesleft = (maxlen-4);

    do {
	r = utf8proc_iterate(iter,-1,&uc);

	if (utf8char_in_set(uc,delimiters) != -1) {

	    // is a delimiter character
	    lastdelim = (char*) iter; //word_ _test
	    endline = lastdelim+r; //   word_t_est
	} else if (utf8char_in_set(uc,linebreaks) != -1) {
	    // is a line break.
	    column = 1; linebroken = 1;
	}

	int ucwidth = utf8proc_charwidth(uc);
	//printf("Character 'U+%X' has width %d\n",thiswc,ucwidth);

	if (column + ucwidth > width) { column = width;} else {

	    if ((utf8char_in_set(uc,spaces) == -1) || (column != 0)) { column += ucwidth; colbyte+=r; if (ind < lc) lcp++; } else if (!linebroken) lastcol = (char*)iter+r; //will not add char if line starts with a space.

	    linebroken=0;
	    iter+=r;
	    ind++;
	    //colbyte+=r;
	    }

	if (column >= width) {

	    ssize_t tocopy = (lastdelim ? (endline-lastcol) : colbyte);

	    if (bytesleft < (tocopy+1)) { tocopy = bytesleft-1; r = 0; }

	    tocopy = utf8_strnlen((const uint8_t*)lastcol,tocopy);

	    strncat(res,lastcol,tocopy);
	    if (r) { strncat(res,"\n",1); if (ind < lc) lcp++;}

	    bytesleft -= (tocopy + 1 );

	    if (lastdelim) iter = (const uint8_t *)endline;
	    lastdelim = NULL;

	    lastcol = (char* ) iter;
	    column=0;
	    linebroken=0;
	    colbyte=0;

	    if (r == 0) { strcat(res,"…"); /*3 bytes */ }
	}
    } while ((r > 0) && (*iter != 0));

    if (bytesleft >= colbyte+1) { strncat(res,lastcol,colbyte); }

    assert(strlen(res) < maxlen);

    strcpy(out,res);

    if (loc_char) *loc_char = lcp;

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

int utf8_charstart(uint8_t* byte) {
    if (*byte == 0) return 0;
    if (*byte <= 0x7f) return 1; //1 byte char
    if (*byte >= 0xc0) return 1; //multibyte start char
    return 0;
}

int utf8_remove_last(char* text) {

    //removes last code point from a UTF-8 text.

    size_t len = strlen(text);

    if (len == 0) return 1;

    char* lastchar = text + (len-1);

    while (lastchar >= text) {
	if (utf8_charstart((uint8_t*)lastchar)) { *lastchar = 0; return 0;}
	lastchar--;
    }
    return 0;
}

int utf8_charcount(uint8_t* c) {
    //returns length of utf8 character, based on the first byte.
    if (*c <= 0x7f) return 1;
    if (*c >= 0xc0) return 2;
    if (*c >= 0xe0) return 3;
    if (*c >= 0xf0) return 4;
    return 0;
}
int utf8_append_char(int32_t uc, char* string, size_t maxbytes) {

    char encchar[5]; //utf8 char length max 4 bytes + 1b for 0.
    memset(encchar,0,5);

    ssize_t l = utf8proc_encode_char(uc,(uint8_t*)encchar);

    if ((strlen(string) + l + 1) > maxbytes) return 1;

    strncat(string,encchar,l);
    return 0;
}

int utf8_insert_char(uint8_t* s, size_t maxsz, int position, int32_t uc) {
    char bk[maxsz];
    strncpy(bk,(char*) s,maxsz);
    
    int up = ( point_to_char_by_idx((char*)s,position) - (char*) s);

    ssize_t l = utf8proc_encode_char(uc, s + up);

    strncpy(s+up+l,bk+up,maxsz);
    return 0;
}

int utf8_delete_char(uint8_t* s, size_t maxsz, int position) {
    
    if ( (position < 0) || (position >= utf8_count_chars((char*)s) ) ) return 1;

    uint8_t* p2 = (uint8_t*) point_to_char_by_idx(s,position);
    int pl = utf8_charcount(p2);
    memmove(p2,p2+pl,strlen(p2+pl));
    
    uint8_t* end = p2 + strlen(p2+pl);

    *end = 0;

    return 0;
}
