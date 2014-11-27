#ifndef _LOG_H_
#define _LOG_H_

int cursesused;
int log_enabled;

int lprintf(const char *format, ...);
void lperror(const char *s);

#endif
