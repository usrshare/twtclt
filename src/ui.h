// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <curses.h>

#ifndef _UI_H_
#define _UI_H_

WINDOW* titlebar;
WINDOW* statusbar;

uint8_t columns;

WINDOW* colpads;

uint8_t cur_col;
uint16_t cur_row;
uint32_t scroll_lines;

int init_ui();
int destroy_ui();

void* windowthread(void* param);

int draw_cards();
void draw_tweet(struct t_tweet* tweet);

#endif
