// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "log.h"
#include "twitter.h"
#include "twt_time.h"
#include "ui.h"
#include "stringex.h"
#include "utf8.h"

#define MAXCOLUMNS 32

#define COLHEIGHT (LINES-3)

char* okcanc[] = {"OK","Cancel"};
char* yesno[] = {"Yes","No"};


uint8_t colwidth = 40; //default width, may be larger
uint8_t visiblecolumns = 1; //how many columns are visible side by side

struct drawcol_ctx{
    int curline;
    int column;
    int row; //number of tweet currently drawn
    int scrollback;
};

struct t_timelineset {
    int enabled;
    struct t_account* acct;
    enum timelinetype tt;
    uint64_t userid;
    int scrollback;
    char* customtype;
}; //column settings

struct btree* columns[MAXCOLUMNS]; //maximum 32 columns
struct t_timelineset colset[MAXCOLUMNS];

int pad_insert(struct tweetbox* pad){

    return ht_insert(padht,pad->id,pad);

}
int pad_delete(uint64_t id){
    struct tweetbox* old = (struct tweetbox*)ht_search(padht,id);
    int r = ht_delete(padht,id);
    if (old != NULL) { delwin(old->window); free(old); }
    return r;

}

int findcolwidth(int minwidth) {

    int c = (COLS / minwidth);

    if (c > 0) visiblecolumns = c;

    if (c == 0) return COLS;

    return (COLS / c);
}

struct tweetbox* pad_search_acct(int acct_id, uint64_t id){

    int n=0; struct tweetbox* pad = NULL;
    do {
	pad = pad_search_ind(id,n);
	if (pad->acct_id == acct_id) return pad; else n++;}
    while (pad != NULL);

    return NULL;
}
struct tweetbox* pad_search_ind(uint64_t id, int index){
    return (struct tweetbox*)ht_search_ind(padht,id,index);
}
struct tweetbox* pad_search(uint64_t id){
    return pad_search_ind(id,0);
}

//---

struct scrollto_ctx {
    int topline;
    int lines;
    int rows;
};

void scrollto_btcb(uint64_t id, void* ctx) {
    struct scrollto_ctx* sts = (struct scrollto_ctx*) ctx;

    struct tweetbox* pad = pad_search(id);

    if (pad == NULL) return;

    sts->topline += sts->lines;
    sts->lines = pad->lines;
    sts->rows++;

    return;
}

int scrollto (int col, int row) {
    struct scrollto_ctx sts = {0,0,0};

    int curel = 0;

    bt_read2(columns[col],scrollto_btcb,&sts,desc,row+1,&curel);

    //lprintf("COLS-2 = %d, topline = %d, lines = %d\n",(LINES-2),sts.topline,sts.lines);

    if (colset[col].scrollback > sts.topline) colset[col].scrollback = sts.topline;

    if (sts.topline - colset[col].scrollback + sts.lines > COLHEIGHT) colset[col].scrollback = sts.topline + sts.lines - COLHEIGHT;

    //TODO
    return (sts.rows - 1);
}

//---

int draw_all_columns() {

    for (int i=0; i < MAXCOLUMNS; i++) {

	if (colset[i].enabled) {

	    draw_column(i,colset[i].scrollback,columns[i]);
	}
    }

    return 0;
}


int reload_all_columns() {

    for (int i=0; i < MAXCOLUMNS; i++) {

	if (colset[i].enabled) {

	    load_timeline(columns[i],colset[i].acct,colset[i].tt,colset[i].userid,colset[i].customtype);	

	}
    }

    return 0;
}

enum msgboxclass {
    msg_info, //blue borders
    msg_warning, //yellow borders
    msg_error, //red borders
    msg_critical, //red borders, darken background?
};

int msgbox(char* message, enum msgboxclass class, int buttons_n, char** btntext) {

    //TODO show and handle buttons.

    int textwidth, textheight, btnwidth = (buttons_n - 1);

    utf8_text_size(message,&textwidth,&textheight);

    char* cutmsg = NULL;

    if ((textwidth) > (COLS - 8)) {

	cutmsg = malloc(strlen(message) + 128);

	int r = utf8_wrap_text(message,cutmsg,strlen(message) + 128,(COLS-8));

	utf8_text_size(cutmsg,&textwidth,&textheight);

    }

    char* dispmsg = (cutmsg ? cutmsg : message);


    if (buttons_n > 0)
	for (int i=0; i<buttons_n; i++) btnwidth += 1 + strlen(btntext[i]) + 1; //[button]

    int maxwidth = textwidth+4;

    int maxheight = textheight+4;

    if (btnwidth >= textwidth) maxwidth = btnwidth+8;

    WINDOW* msgwindow = newwin(maxheight,maxwidth,(LINES-maxheight)/2, (COLS-maxwidth)/2);

    switch (class) {
	case msg_info:
	    wattron(msgwindow,COLOR_PAIR(10));
	    box(msgwindow,0,0);
	    wattroff(msgwindow,COLOR_PAIR(10));
	    break;
	case msg_warning:
	    wattron(msgwindow,COLOR_PAIR(11));
	    box(msgwindow,0,0);
	    wattroff(msgwindow,COLOR_PAIR(11));
	    break;
	case msg_error:
	    wattron(msgwindow,COLOR_PAIR(12));
	    box(msgwindow,0,0);
	    wattroff(msgwindow,COLOR_PAIR(12));
	    break;
	case msg_critical:
	    wattron(msgwindow,COLOR_PAIR(13));
	    box(msgwindow,0,0);
	    wattroff(msgwindow,COLOR_PAIR(13));
	    break;
    }

    WINDOW* textwin = derwin(msgwindow,maxheight-4,maxwidth-4,2,2);

    waddstr(textwin,message);

    touchwin(msgwindow);

    WINDOW* buttons[buttons_n]; //C99 VLAs FTW!

    if (buttons_n != 0) {

	int btnwidth = (buttons_n - 1); //1 space between buttons

	for (int i=0; i<buttons_n; i++) {
	    btnwidth += 1 + strlen(btntext[i]) + 1; //[button]
	}

	int btnleft = (maxwidth - btnwidth) / 2;

	int curbutleft = btnleft;

	for (int i=0; i<buttons_n; i++) {

	    buttons[i] = derwin(msgwindow,1,strlen(btntext[i])+2, maxheight-2, curbutleft);
	    curbutleft+=(strlen(btntext[i])+3);
	}
    }

    int selectloop = 1;

    int selbtn = 0;

    while (selectloop) {

	for (int i=0; i<buttons_n; i++) {
	    if (selbtn == i) wattron(buttons[i],A_REVERSE);
	    mvwprintw(buttons[i],0,0,"[%s]",btntext[i]);
	    if (selbtn == i) wattroff(buttons[i],A_REVERSE);
	    touchwin(msgwindow);
	}

	wrefresh(msgwindow);

	wmove(inputbar,0,0);
	int k = wgetch(inputbar); 

	switch(k) {
	    case 'h':
	    case KEY_LEFT:
		selbtn = (selbtn -1) % buttons_n; if (selbtn < 0) selbtn = (buttons_n -1);
		break;
	    case 'l':
	    case KEY_STAB:
	    case KEY_RIGHT:
		selbtn = (selbtn +1) % buttons_n;
		break;
	    case 32:
	    case KEY_ENTER:
		selectloop=0;
		break;
	}

    }

    for (int i=0; i<buttons_n; i++) {

	delwin(buttons[i]);

    }

    delwin(textwin);
    delwin(msgwindow);

    touchwin(colarea);
    wrefresh(colarea);

    if (cutmsg) free (cutmsg);
    return selbtn;
}

void* uithreadfunc(void* param) {
    // -- test.

    cur_col = 0; cur_row = 0; int uiloop = 1;

    while(uiloop) {

	mvwprintw(statusbar,0,0,"Ready.\n"); wrefresh(statusbar);

	int k = wgetch(inputbar); 

	mvwprintw(statusbar,0,0,"Hit key %d\n",k); wrefresh(statusbar);

	switch(k) {

	    case 'J':
		// scroll down a line
		colset[cur_col].scrollback++; break;
	    case 'K':
		// scroll up a line
		if (colset[cur_col].scrollback > 0) colset[cur_col].scrollback--; break;
	    case 'l':
		// Show all links in the selected tweet
	    case KEY_LEFT:
		// Select next tweet, TODO make scrolling follow selection
		if (cur_col > 0) cur_col--;
		cur_row = scrollto(cur_col,cur_row); 	
		break;
	    case KEY_RIGHT:
		// Select previous tweet, TODO make scrolling follow selection
		if (colset[cur_col+1].enabled) cur_col++;
		cur_row = scrollto(cur_col,cur_row); 	
		break;
	    case 'j':
	    case KEY_DOWN:
		// Select next tweet, TODO make scrolling follow selection
		cur_row++; cur_row = scrollto(cur_col,cur_row); break;
	    case 'k':
	    case KEY_UP:
		// Select previous tweet, TODO make scrolling follow selection
		if (cur_row >= 1) cur_row--; else cur_row = 0; cur_row = scrollto(cur_col,cur_row); break;
	    case KEY_NPAGE:
		// Scroll one page down
		(colset[cur_col].scrollback)+=COLHEIGHT; break;
	    case KEY_PPAGE:
		// Scroll one page up
		(colset[cur_col].scrollback)-=COLHEIGHT; if ((colset[cur_col].scrollback) < 0) (colset[cur_col].scrollback) = 0; break;
	    case 'm': {
		// Load timeline. Tweets will be added.
		int r = msgbox("Lol.",msg_info,2,okcanc);
		if (r) msgbox("Option 2?",msg_error,0,NULL);
		break; }
	    case 'r':
		// Load timeline. Tweets will be added.
		reload_all_columns(); break;
	    case 'q':
		uiloop = 0;
		break;
	}

	draw_all_columns();
	//draw_column(0,scrollback,acctlist[0]->timelinebt);
    }

    endwin();

    return NULL;

}

pthread_t* init_ui(){
    cursesused = 1;
    initscr();

    if (has_colors() == FALSE) {
	endwin();
	printf("This terminal has no color support. twtclt requires color to work.\n");
	exit(1);
    }

    colwidth = findcolwidth(40);

    for (int i=0; i<MAXCOLUMNS; i++) {
	columns[i] = NULL;
	colset[i].enabled = 0;
	colset[i].acct = NULL;
	colset[i].tt = 0;
	colset[i].customtype = NULL;
    }

    columns[0] = bt_create();
    colset[0].enabled = 1;
    colset[0].acct = acctlist[0];
    colset[0].tt = home;
    colset[0].customtype = NULL;

    columns[1] = bt_create();
    colset[1].enabled = 1;
    colset[1].acct = acctlist[0];
    colset[1].tt = mentions;
    colset[1].customtype = NULL;

    start_color();	
    cbreak();

    padht = ht_create2(1024,0); //this hash table allows for non-unique items. 

    noecho();

    init_pair(1,COLOR_CYAN,COLOR_BLUE); //background
    init_pair(2,COLOR_YELLOW,COLOR_BLUE); //bars
    init_pair(3,COLOR_BLACK,COLOR_WHITE); //cards
    init_pair(4,COLOR_GREEN,COLOR_WHITE); //retweet indication
    init_pair(5,COLOR_YELLOW,COLOR_WHITE); //fave indication
    init_pair(6,COLOR_BLUE,COLOR_WHITE); //mention indication
    init_pair(7,COLOR_WHITE,COLOR_BLUE); //mention indication

    if (COLORS >= 16) 
	init_pair(8,COLOR_BLACK,COLOR_WHITE + 8); //selected bg
    else
	init_pair(8,COLOR_BLACK,COLOR_YELLOW);

    init_pair(9,COLOR_BLACK,COLOR_CYAN); //background

    init_pair(10,COLOR_BLUE,COLOR_BLACK); //bars
    init_pair(11,COLOR_YELLOW,COLOR_BLACK); //bars
    init_pair(12,COLOR_RED,COLOR_BLACK); //bars
    init_pair(13,COLOR_WHITE,COLOR_RED); //bars

    keypad(stdscr, TRUE);

    titlebar = newwin(1,COLS,0,0);
    wbkgd(titlebar,COLOR_PAIR(1));
    wprintw(titlebar,"twtclt");
    wrefresh(titlebar);

    statusbar = newwin(1,(COLS-1),(LINES-1),0);
    wbkgd(statusbar,COLOR_PAIR(1));

    inputbar = newwin(1,1,(LINES-1),(COLS-1));
    keypad(inputbar, TRUE);
    wbkgd(inputbar,COLOR_PAIR(1));

    wrefresh(titlebar);

    colhdrs = newwin(1,COLS,2,0);
    wbkgd(colarea,COLOR_PAIR(9));

    colarea = newwin(COLHEIGHT,COLS,2,0);
    wbkgdset(colarea,ACS_CKBOARD | COLOR_PAIR(1));
    wbkgd(colarea,ACS_CKBOARD | COLOR_PAIR(1));
    wrefresh(colarea);
    wrefresh(statusbar);
    wrefresh(inputbar);

    pthread_t* keythread = malloc(sizeof(pthread_t));
    int r = pthread_create(keythread,NULL,uithreadfunc,NULL);

    if (r != 0) lprintf("pthread_create returned %d\n",r);

    return keythread;
}

void drawtwt_cb(uint64_t id, void* ctx) {

    //draws an individual tweet into its respective tweetbox, then stores the tweetbox in the pad hashtable.

    struct tweetbox* pad = pad_search(id); 

    if (pad == NULL) {

	WINDOW* tp; int lines;

	struct tweetbox* newpad = malloc(sizeof(struct tweetbox));

	newpad->acct_id = 0;
	newpad->id = id;

	struct t_tweet* tt = tht_search(id);
	if (tt == NULL) return;

	tp = tweetpad(tt,&lines,0);
	if (tp == NULL) {free(newpad); return;}

	newpad->window = tp;
	newpad->lines = lines;

	tweetdel(tt);

	int r = pad_insert(newpad);
	if (r != 0) lprintf("pad_insert returned %d\n",r);
    }
    return;
}


void drawcol_cb(uint64_t id, void* ctx) {

    struct drawcol_ctx *dc = (struct drawcol_ctx *) ctx;

    WINDOW* tp;

    int lines;

    struct tweetbox* pad = pad_search(id); 

    int cursel = ((dc->row == cur_row) && (dc->column == cur_col));

    if (pad == NULL) return;

    if (cursel) {

	struct t_tweet* tt = tht_search(id);
	if (tt == NULL) return;

	tp = tweetpad(tt,&lines,1);
	if (tp == NULL) return;

	tweetdel(tt);

    } else { tp = pad->window; lines = pad->lines; }

    if ((dc->curline + lines >= dc->scrollback) && (dc->curline - dc->scrollback <= COLHEIGHT -1 )) {

	int skipy = -(dc->curline - dc->scrollback);
	int topy = ( (dc->curline - dc->scrollback > 0) ? (dc->curline - dc->scrollback) : 0);
	int boty = ( (dc->curline - dc->scrollback + lines <= COLHEIGHT+1) ? (dc->curline - dc->scrollback + lines) : COLHEIGHT);

	pnoutrefresh(tp,skipy,0,topy+2,(dc->column * colwidth),boty+1,(dc->column +1) *colwidth - 1);

    }

    dc->curline += lines;
    dc->row++;

    return;

}

void draw_headers() {

    for (int i=0; i < MAXCOLUMNS; i++) {

    }	

}

void draw_column(int column, int scrollback, struct btree* timeline) {
    //btree should contain tweet IDs.

    bt_read(timeline, drawtwt_cb, NULL, desc);

    struct drawcol_ctx dc = { .curline=0, .column=column, .row=0, .scrollback=scrollback};
    bt_read(timeline, drawcol_cb, &dc, desc);

    //int curcol = (dc.curline - dc.scrollback);

    doupdate();
} 

WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected) {
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

    int textlines = countlines(tweettext,400);

    int lines = (tweet->retweeted_status_id ? textlines + 6 : textlines + 4);

    WINDOW* tp = newpad(lines, colwidth);

    wbkgd(tp,(selected ? COLOR_PAIR(8) : COLOR_PAIR(3)));

    if (selected) wattron(tp,COLOR_PAIR(8));

    wattron(tp,A_BOLD);
    if (tweet->retweeted_status_id) wattron(tp,COLOR_PAIR(4)); else wattron(tp,COLOR_PAIR(3));
    for (int i=0; i < colwidth; i++) {
	waddstr(tp,"▀");
    }
    wattroff(tp,A_BOLD);
    if (tweet->retweeted_status_id) wattroff(tp,COLOR_PAIR(4)); else wattroff(tp,COLOR_PAIR(3));
    wattron(tp,(selected ? COLOR_PAIR(8) : COLOR_PAIR(3)));
    mvwaddch(tp,1,1,'@');
    mvwaddstr(tp,1,2,usn);

    char reltime[8];
    reltimestr(ot->created_at,reltime);
    mvwaddstr(tp,1,colwidth-1-strlen(reltime),reltime);

    WINDOW* textpad = subpad(tp,textlines,colwidth-1,3,1);

    mvwaddstr(textpad,0,0,tweettext);

    touchwin(tp);

    if (tweet->retweeted_status_id) {

	struct t_user* rtu = uht_search(tweet->user_id); 

	char* rtusn = (rtu ? rtu->screen_name : "(unknown)");

	mvwprintw(tp,lines-2,1,"RT by @%s",rtusn);

	userdel(rtu);

	char rttime[8];
	reltimestr(tweet->created_at,rttime);
	mvwaddstr(tp,lines-2,colwidth-1-strlen(rttime),rttime);




    }

    wbkgdset(tp,COLOR_PAIR(7));

    if (selected) wattroff(tp,COLOR_PAIR(8));

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

    WINDOW* tp = tweetpad(tweet,&lines,0);

    prefresh(tp,0,0,0,0,lines,colwidth);

    wclear(tp);

    delwin(tp);
}

void* windowthread(void* param){
    printf("Called windowthread with param %p\n",param);
    return NULL;
}

int draw_cards() {
    return 1;
}
