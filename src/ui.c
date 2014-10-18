#include <stdlib.h>
#include <pthread.h>

#include "ui.h"
#include "stringex.h"

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

}

int destroy_ui(){
	endwin();

}

char* splittext(char* origstring, char* separators) {
	char* newstring = malloc(1); newstring[0] = '\0';


	
}

WINDOW* tweetpad(struct t_tweet* tweet) {
	if (tweet == NULL) return NULL;

	
	

}

void* windowthread(void* param)
{

}

int draw_cards() {
}
