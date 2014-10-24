// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "twitter.h"
#include "ui.h"
#include "stringex.h"
#include "utf8.h"

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
    init_pair(4,COLOR_GREEN,COLOR_WHITE); //retweet indication
    init_pair(5,COLOR_YELLOW,COLOR_WHITE); //fave indication
    init_pair(6,COLOR_BLUE,COLOR_WHITE); //mention indication
    init_pair(7,COLOR_WHITE,COLOR_BLUE); //mention indication

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

WINDOW* tweetpad(struct t_tweet* tweet, int* linecount) {
    if (tweet == NULL) return NULL;
    char tweettext[400];

    struct t_tweet* ot = (tweet->retweeted_status_id ? tht_search(tweet->retweeted_status_id) : tweet ); //original tweet

    char* text = parse_tweet_entities(ot);
    
    struct t_user* rtu = uht_search(ot->user_id);

    char* usn = rtu->screen_name;

    utf8_wrap_text(text,tweettext,400,colwidth - 2); 

    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // * | Screen name
    // -------------------------
    // Tweet content
    // RT by screen name | time
    // ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

    int lines = countlines(tweettext,400)+6;

    WINDOW* tp = newpad(lines, colwidth);

    wbkgd(tp,COLOR_PAIR(3));
    
    wattron(tp,A_BOLD);
    if (tweet->retweeted_status_id) wattron(tp,COLOR_PAIR(4)); else wattron(tp,COLOR_PAIR(3));
        for (int i=0; i < colwidth; i++) {
	waddstr(tp,"▀");
    }
    wattroff(tp,A_BOLD);
    if (tweet->retweeted_status_id) wattroff(tp,COLOR_PAIR(4)); else wattroff(tp,COLOR_PAIR(3));
    wattron(tp,COLOR_PAIR(3));
    mvwaddch(tp,1,1,'@');
    mvwaddstr(tp,1,2,usn);

    char reltime[8];
    reltimestr(ot->created_at,reltime);
    mvwaddstr(tp,1,colwidth-1-strlen(reltime),reltime);

    WINDOW* textpad = subpad(tp,lines-6,colwidth-1,3,1);

    mvwaddstr(textpad,0,0,tweettext);

    touchwin(tp);

        if (tweet->retweeted_status_id) {
	mvwprintw(tp,lines-2,1,"RT by somebody");
    }
    
    wbkgdset(tp,COLOR_PAIR(7));

    wattroff(tp,COLOR_PAIR(7));
    wmove(tp,lines-1,0);
    for (int i=0; i < colwidth; i++) {

	waddstr(tp,"▀");
    }
    wattroff(tp,COLOR_PAIR(7));

    if (linecount != NULL) *linecount = lines;

    delwin(textpad);

    return tp;
}

void draw_tweet(struct t_tweet* tweet) {

    int lines;

    WINDOW* tp = tweetpad(tweet,&lines);

    prefresh(tp,0,0,0,0,lines,colwidth);

    getch();

    wclear(tp);

    delwin(tp);
}

void* windowthread(void* param)
{
    return NULL;
}

int draw_cards() {
    return 1;
}
