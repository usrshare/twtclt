#ifndef _GLOBALS_H_
#define _GLOBALS_H_
#include <stdio.h>

#define s_eq(x,y) (strcmp(x,y) == 0)

FILE* logfile;

int acct_n; //known accounts
struct t_account** acctlist; //acct list.


#endif

