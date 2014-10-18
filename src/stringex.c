// vim: cin:sts=4:sw=4 

#include "stringex.h"

#ifndef _STRINGEX_H_
#define _STRINGEX_H_

char* strrecat(char* orig, const char* append) {
	char* new = realloc(orig,strlen(orig) + strlen(append) + 1);
	new = strcat(new,append);
	return new;
}

char* addparam(char* orig, const char* parname, const char* parvalue, int addq) {
	char* append = malloc(1 + strlen(parname) + 1 + strlen(parvalue) + 1);
	sprintf(append,"%s%s=%s",(strchr(orig,'?') ? (addq ? "?" : "") : "&"),parname,parvalue); 
	return strrecat(orig,append);
}
char* addparam_int(char* orig, const char* parname, int64_t parvalue, int addq) {
	char pvchr[22]; //enough to store any 64bit number.
	sprintf(pvchr,"%lld",parvalue);
	return addparam(orig,parname,pvchr,addq);
}

#endif
