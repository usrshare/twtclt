// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <curses.h>

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

uint64_t fullid(uint8_t acct, uint64_t id);


WINDOW* titlebar;
WINDOW* statusbar;
WINDOW* inputbar;
WINDOW* colarea;

uint8_t colcount;

WINDOW* colpads;

uint8_t cur_col;
uint16_t cur_row;
uint32_t scroll_lines;

int init_ui();
int destroy_ui();

void* windowthread(void* param);

int draw_cards();
void draw_tweet(struct t_tweet* tweet);

void drawcol_cb(uint64_t id, void* ctx);
void draw_column(int column, int scrollback, struct btree* timeline);
WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected);

#endif
