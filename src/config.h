#include <stdio.h>

#ifndef _CONFIG_H_
#define _CONFIG_H_

char* configdir;

FILE *cfopen(const char *path, const char *mode);
int load_config();

#endif
