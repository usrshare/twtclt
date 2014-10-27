#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "log.h"
#include "config.h"
#include "stringex.h"

FILE *cfopen(const char *path, const char *mode) {
    char* cfname = strdup(configdir);
    cfname = strrecat(cfname,path);
    FILE* r = fopen(cfname,mode);
    free(cfname);
    return r;
}

int load_config() {

    char* cdir = strdup("");

    char* xdgdir = getenv("XDG_CONFIG_HOME");
    if (xdgdir != NULL) configdir = strrecat(configdir,xdgdir); else {

	char* homedir = getenv("HOME");
	if (homedir == NULL) homedir = getenv("USERPROFILE"); //Windows

	if (homedir != NULL) { cdir = strrecat(cdir,homedir); cdir = strrecat(cdir,"/.twtclt/"); }

    }

    int r = mkdir(cdir,0700);

    if ((r == -1) && (errno != EEXIST)) lperror("Error while creating config dir"); else configdir = strdup(cdir);

    free(cdir);

    return 0;
}
