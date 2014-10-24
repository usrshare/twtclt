// vim: cin:sts=4:sw=4 
#include <inttypes.h>
#include <string.h>
#include "stringex.h"

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
    sprintf(append,"%s%s=%s",(strchr(orig,'?') ? (addq ? "?" : "") : "&"),parname,parvalue); 
    return strrecat(orig,append);
}
char* addparam_int(char* orig, const char* parname, int64_t parvalue, int addq) {
    char pvchr[22]; //enough to store any 64bit number.
    sprintf(pvchr,"%" PRId64 "",parvalue);
    return addparam(orig,parname,pvchr,addq);
}

int countlines (char* text, int maxlen) {

    int sl = (maxlen ? maxlen : strlen(text));

    int n = 1;

    for (int i=0; i<sl; i++)
	if (text[i] == '\n') n++;

    return n;
}
