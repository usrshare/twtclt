// vim: cin:sts=4:sw=4 
#include <inttypes.h>
#include <string.h>
#include "stringex.h"

int strndcat(char* dest, char* src, size_t* left) {

    size_t ss = strlen(src);
    if (ss > ((*left)-1)) ss = ((*left)-1);

    strncat(dest,src,ss);
    *left -= ss;
    return 0;
}

char* strrecat(char* orig, const char* append) {
    char* new = realloc(orig,strlen(orig) + strlen(append) + 1);
    new = strcat(new,append);
    return new;
}

char* strnrecat(char* orig, const char* append, size_t n) {

    char* app = strndup(append,n);

    char* new = realloc(orig,strlen(orig) + n + 1);
    new = strcat(new,app);

    free(app);
    return new;
}

char* addparam(char* orig, const char* parname, const char* parvalue, int addq) {
    char* append = malloc(1 + strlen(parname) + 1 + strlen(parvalue) + 1);
    sprintf(append,"%s%s=%s",(strchr(orig,'?') ? (addq ? "?" : "") : ((strlen(orig) > 0) ? "&" : "") ),parname,parvalue); 
    return strrecat(orig,append);
}
char* addparam_int(char* orig, const char* parname, uint64_t parvalue, int addq) {
    char pvchr[22]; //enough to store any 64bit number.
    sprintf(pvchr,"%" PRIu64 "",parvalue);
    return addparam(orig,parname,pvchr,addq);
}

int countlines (char* text, size_t maxlen) {

    int sl = (maxlen ? (maxlen < strlen(text) ? maxlen : strlen(text)) : strlen(text)); //if 0, strlen, otherwise the lesser of maxlen and strlen.

    int n = 1;

    for (int i=0; i<sl; i++)
	if (text[i] == '\n') n++;

    return n;
}
