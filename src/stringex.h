// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

char* strrecat(char* orig, const char* append);
char* strnrecat(char* orig, const char* append, size_t n);
char* addparam(char* orig, const char* parname, const char* parvalue, int addq);
char* addparam_int(char* orig, const char* parname, int64_t parvalue, int addq);
int countlines (char* text, int maxlen);
