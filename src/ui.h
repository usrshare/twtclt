// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <curses.h>
#include <pthread.h>

#include "btree.h"
#include "twt_struct.h"

#ifndef _UI_H_
#define _UI_H_

struct tweetbox {
    int acct_id;
    uint64_t id;
    WINDOW* window;
    int lines;
};

struct hashtable* padht; //tweet cache hash table.

int pad_insert(struct tweetbox* tbox);
int pad_delete(uint64_t id);
struct tweetbox* pad_search_acct(int acct_id, uint64_t id);
struct tweetbox* pad_search_ind(uint64_t id, int index);
struct tweetbox* pad_search(uint64_t id);

int save_columns(FILE* file);

uint64_t fullid(uint8_t acct, uint64_t id);

uint8_t cur_col;
uint64_t curtwtid;

pthread_t* init_ui();
int destroy_ui();

void* windowthread(void* param);

void drawcol_cb(uint64_t id, void* ctx);
void draw_column_limit(int column, int scrollback, struct btree* timeline, int topline, int lines);
void draw_column2(int column, int scrollback, struct btree* timeline,int do_update);
void draw_column(int column, int scrollback, struct btree* timeline);
void draw_coldesc(int column);
WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected);

void update_unread(int column);

int redraw_lines(int topline, int lines);
int draw_all_columns();

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
//end color pairs

#endif
