// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <curses.h>
#include <pthread.h>

#include "btree.h"
#include "twt_struct.h"

#ifndef _UI_H_
#define _UI_H_

int load_columns(FILE* file);
int save_columns(FILE* file);

pthread_t* init_ui();
int destroy_ui();

void* windowthread(void* param);

//colors
short blackcolor; //256-color compatible black. allows for bold w/o bright.
short whitecolor; //256-color compatible black. allows for bold w/o bright.
short twtcolor; //twitter logo color
short twtcolor2; //twitter logo color, darker
short bgcolor; //background color
short selbgcolor; //selected bg color
short hdrcolor; //header bg color
short gray1; //roughly equiv to #888888
//end colors

//color pairs
#define CP_TWTBG COLOR_PAIR(1)
#define CP_BARS COLOR_PAIR(2)
#define CP_CARD COLOR_PAIR(3)
#define CP_RT COLOR_PAIR(4)
#define CP_FAV COLOR_PAIR(5)
#define CP_MENT COLOR_PAIR(6)
#define CP_GRAY COLOR_PAIR(7)
#define CP_CARDSEL COLOR_PAIR(8)
#define CP_HDR COLOR_PAIR(9)
#define CP_UNREAD COLOR_PAIR(13)
#define CP_HDRSEL COLOR_PAIR(14)
#define CP_BG COLOR_PAIR(15)
#define CP_GRAYSEL COLOR_PAIR(16)
#define CP_COMPBG1 COLOR_PAIR(17)
#define CP_COMPSEL COLOR_PAIR(18)
//end color pairs

#endif
