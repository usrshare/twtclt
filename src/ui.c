// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "twitter.h"
#include "ui.h"
#include "stringex.h"

const uint8_t colwidth = 32;

int init_ui(){
	initscr();
	
	if (has_colors() == FALSE) {
		endwin();
		printf("This terminal has no color support. twtclt requires color to work.\n");
		exit(1);
	}
	
	start_color();	
	cbreak();
	noecho();

	init_pair(1,COLOR_CYAN,COLOR_BLUE); //background
	init_pair(2,COLOR_YELLOW,COLOR_BLUE); //bars
	init_pair(3,COLOR_BLACK,COLOR_WHITE); //cards

	keypad(stdscr, TRUE);
	titlebar = newwin(1,COLS,0,0);
	statusbar = newwin(1,COLS,(LINES-1),0);
	return 0;
}

int destroy_ui(){
	endwin();
	return 0;
}

char* splittext(char* origstring, char* separators) {
	char* newstring = malloc(1); newstring[0] = '\0';


	return NULL;
}

WINDOW* tweetpad(struct t_tweet* tweet) {
    if (tweet == NULL) return NULL;
    char tweettext[400];

    struct t_tweet* ot = (tweet->retweeted_status_id ? tht_search(tweet->retweeted_status_id) : tweet ); //original tweet

    char* text = parse_tweet_entities(ot);
    

    

	
	
    return NULL;
}

void* windowthread(void* param)
{
    return NULL;
}

int draw_cards() {
    return 1;
}
