// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "log.h"
#include "twitter.h"
#include "twt_oauth.h"
#include "twt_time.h"
#include "ui.h"
#include "stringex.h"
#include "utf8.h"

#include "ui_windows.h"

#define MAXCOLUMNS 32

#define COLHEIGHT (LINES-3)

char* okcanc[] = {"OK","Cancel"};
char* yesno[] = {"Yes","No"};

uint8_t colwidth = 40; //default width, may be larger
uint8_t visiblecolumns = 1; //how many columns are visible side by side
uint8_t leftmostcol = 0; //leftmost visible column

struct drawcol_ctx{
    int curline;
    int column;
    int row; //number of tweet currently drawn
    int scrollback;
    int topline; //if not 0, only draw lines starting from topline
    int lines; //if not 0, only draw N lines
};

struct t_timelineset {
    int enabled;
    uint64_t curtwtid; //id of selected tweet
    uint64_t lastread; //id of last read (visible on screen) tweet.
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

//--- scrollto functions. scroll to row or tweet IDs.

struct scrollto_ctx {
    int topline;
    int lines;
    int rows;
    int maxrow;
    uint64_t twtid;
    int res_tl;
    int res_ln;
};
void scrollto_btcb(uint64_t id, void* ctx) {
    struct scrollto_ctx* sts = (struct scrollto_ctx*) ctx;

    if (sts->res_tl != -1) return;

    struct tweetbox* pad = pad_search(id);

    if (pad == NULL) return;

    sts->topline += sts->lines;
    sts->lines = pad->lines;

    if ( (id <= sts->twtid) || (sts->rows == sts->maxrow) ) { 
	sts->res_tl = sts->topline;
	sts->res_ln = sts->lines;
    }

    sts->rows++;

    return;
}
int scrollto (int col, int row) {
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .maxrow = row, .twtid = 0, .res_tl = -1, .res_ln = 0};

    bt_read(columns[col],scrollto_btcb,&sts,desc);

    if (colset[col].scrollback > sts.topline) colset[col].scrollback = sts.topline;
    if (sts.topline - colset[col].scrollback + sts.lines > COLHEIGHT) colset[col].scrollback = sts.topline + sts.lines - COLHEIGHT;
    return (sts.rows - 1);
}
int scrolltotwt (int col, uint64_t twtid) {
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .maxrow = -1, .twtid = twtid, .res_tl = -1, .res_ln = 0};

    bt_read(columns[col],scrollto_btcb,&sts,desc);

    if (colset[col].scrollback > sts.topline) colset[col].scrollback = sts.topline;
    if (sts.topline - colset[col].scrollback + sts.lines > COLHEIGHT) colset[col].scrollback = sts.topline + sts.lines - COLHEIGHT;
    if (twtid > colset[col].lastread) colset[col].lastread = twtid;
    return (sts.rows - 1);
}

//---

#define min(x,y) ( (x) < (y) ? (x) : (y) )

int draw_column_headers() {
    for (int i=leftmostcol; i < min(leftmostcol + visiblecolumns,MAXCOLUMNS); i++) {

	if (colset[i].enabled) {

	    draw_column(i,colset[i].scrollback,columns[i]);
	    update_unread(i);
	}
    }

}
int draw_all_columns() {

    for (int i=leftmostcol; i < min(leftmostcol + visiblecolumns,MAXCOLUMNS); i++) {

	if (colset[i].enabled) {

	    draw_column(i,colset[i].scrollback,columns[i]);
	    update_unread(i);
	}
    }

    return 0;
}

int redraw_lines(int topline, int lines) {

    for (int i=0; i < MAXCOLUMNS; i++) {

	if (colset[i].enabled) {

	    draw_column_limit(i,colset[i].scrollback,columns[i],topline,lines);
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

struct row_tweet_ctx {
    uint64_t id;
    int cur_ind;
    int index;
    int shift;
    uint64_t cur_id;
    uint64_t res_id;
    int diff_less;
};

// row_tweet functions return a tweet ID from row number or shift from the currently selected tweet in the column.

void row_tweet_cb(uint64_t id, void* param) {
    struct row_tweet_ctx* ctx = (struct row_tweet_ctx*) param;

    if ( (ctx->index == -1) && (ctx->id = id) )
	ctx->index = ctx->cur_ind;

    if ( (ctx->id == 0) && (ctx->index == ctx->cur_ind) )
	ctx->id = id;

    ctx->cur_ind++;
}
void row_tweet_shift_cb(uint64_t id, void* param) {
    struct row_tweet_ctx* ctx = (struct row_tweet_ctx*) param;

    if ( ((ctx->diff_less) && (ctx->id < id)) || ((!(ctx->diff_less)) && (ctx->id > id)) ) ctx->index++;

    if (ctx->shift == ctx->index) ctx->res_id = id;

    ctx->cur_id = id;

}

int ui_addAccount() {

    int r = msgbox("Are you sure you want to create a new account?",msg_info,2,yesno);

    if (r == 1) return 1;

    char oauthkey[8];
    
    char baseurl[128];
    memset(baseurl,'\0',sizeof baseurl);
    struct t_account *myacct = newAccount(baseurl);

    r = request_token(myacct);

    if (r != 0) { lprintf("request_token returned %d\n",r); return 1;}
    char* url;
    r = authorize(myacct,&url);
   
    if (r != 0) { lprintf("authorize returned %d\n",r); return 1;}
    int pin = 0;
    
    char navimsg[500];

    snprintf(navimsg,499,"To authenticate your Twitter account, please navigate to the following URL and input your login details: \n%s\n",url);

    msgbox(navimsg,msg_info,0,NULL);

    free(url);
    
    inputbox("Please enter the 7-digit key you received:",msg_info,oauthkey,7);

    pin = atoi(oauthkey);

    r = oauth_verify(myacct,pin);
    if (r != 0) { lprintf("oauth_verify returned %d\n",r); return 1;}

    add_acct(myacct);

    return 0;
}

uint64_t row_tweet_shift(int column, uint64_t id, int indexdiff) {

    //returns the tweet id that's N columns above or below in the column list.2
    if (columns[column] == NULL) return 0;

    struct row_tweet_ctx ctx = {.id = id, .index = 0, .shift = abs(indexdiff), .res_id = 0, .diff_less = (indexdiff < 0) };

    if (indexdiff == 0) return id;

    if (indexdiff > 0) bt_read(columns[column], row_tweet_shift_cb, &ctx, desc); else
	bt_read(columns[column], row_tweet_shift_cb, &ctx, asc);

    uint64_t border_id = (indexdiff > 0 ? 0 : UINT64_MAX );

    uint64_t res = ( ctx.res_id ? ctx.res_id : border_id );

    lprintf("row_tweet_shift %d on %" PRIu64 " returns %" PRIu64 "\n",indexdiff,id,res);
    return res;
}
uint64_t row_tweet_index(int column, uint64_t id, int index) {

    //either returns the tweet id, if id is zero, or index, if index is -1.
    if ((index != -1) && (id != 0)) return 0;
    if (columns[column] == NULL) return 0;

    struct row_tweet_ctx ctx = {.id = id, .cur_ind = 0, .index = index};

    bt_read(columns[column], row_tweet_cb, &ctx, desc);

    if (index == -1) return ctx.index;
    if (id == 0) return ctx.id;
}

// row_tweet end

struct uicbctx {
    int colnum;
};

void uistreamcb(uint64_t id, void* cbctx) {
    struct uicbctx* ctx = (struct uicbctx *)cbctx;
    draw_column2(ctx->colnum,colset[ctx->colnum].scrollback,columns[ctx->colnum],0);
    scrolltotwt(ctx->colnum,colset[ctx->colnum].curtwtid);
    draw_column2(ctx->colnum,colset[ctx->colnum].scrollback,columns[ctx->colnum],1);
}

void* uithreadfunc(void* param) {
    // -- test.


    for (int i=0; i < MAXCOLUMNS; i++)
	if (colset[i].enabled) colset[i].curtwtid = UINT64_MAX; //first possible tweet

    cur_col = 0; int uiloop = 1;

    while(uiloop) {

	mvwprintw(statusbar,0,0,"Ready.\n"); wrefresh(statusbar);

	int k = wgetch(inputbar); 

	mvwprintw(statusbar,0,0,"Hit key %d\n",k); wrefresh(statusbar);

	switch(k) {

	    case 'J':
		// scroll down a line
		colset[cur_col].scrollback++;
		draw_all_columns();
		break;
	    case 'K':
		// scroll up a line
		if (colset[cur_col].scrollback > 0) colset[cur_col].scrollback--;
		draw_all_columns();
		break;
	    case 'l':
		// Show all links in the selected tweet
	    case KEY_LEFT:
		// Select next tweet, TODO make scrolling follow selection
		if (cur_col > 0) cur_col--;
		scrolltotwt(cur_col,colset[cur_col].curtwtid);
		draw_all_columns();
		break;
	    case KEY_RIGHT:
		// Select previous tweet, TODO make scrolling follow selection
		if (colset[cur_col+1].enabled) cur_col++;
		scrolltotwt(cur_col,colset[cur_col].curtwtid); 	
		draw_all_columns();
		break;
	    case 'j':
	    case KEY_DOWN: {
			       // Select next tweet, TODO make scrolling follow selection
			       colset[cur_col].curtwtid = row_tweet_shift(cur_col, colset[cur_col].curtwtid, 1);
			       scrolltotwt(cur_col,colset[cur_col].curtwtid);
			       draw_all_columns();
			       break; }
	    case 'k':
	    case KEY_UP: {
			     // Select previous tweet, TODO make scrolling follow selection
			     colset[cur_col].curtwtid = row_tweet_shift(cur_col, colset[cur_col].curtwtid, -1);
			     scrolltotwt(cur_col,colset[cur_col].curtwtid);
			     draw_all_columns();
			     break; }
	    case KEY_HOME:
			 // Scroll one page down
			 colset[cur_col].curtwtid = UINT64_MAX;
			 scrolltotwt(cur_col,colset[cur_col].curtwtid);
			 draw_all_columns();
			 break;
	    case KEY_END:
			 // Scroll one page down
			 colset[cur_col].curtwtid = 0;
			 scrolltotwt(cur_col,colset[cur_col].curtwtid);
			 draw_all_columns();
			 break;
	    case KEY_PPAGE:
			 // Scroll one page up
			 (colset[cur_col].scrollback)-=COLHEIGHT; if ((colset[cur_col].scrollback) < 0) (colset[cur_col].scrollback) = 0;
			 draw_all_columns();
			 break;
	    case KEY_NPAGE:
			 // Scroll one page down
			 (colset[cur_col].scrollback)+=COLHEIGHT;
			 draw_all_columns();
			 break;
	    case 'i': {
			  // Load timeline. Tweets will be added.
			  char inp[80];
			  int r = inputbox("Test?",msg_info,inp,80);
			  msgbox(inp,msg_error,0,NULL);
			  draw_all_columns();
			  break; }
	    case 'm': {
			  // Load timeline. Tweets will be added.
			  int r = msgbox("Lol.",msg_info,2,okcanc);
			  if (r) msgbox("Option 2?",msg_error,0,NULL);
			  draw_all_columns();
			  break; }
	    case 'r':
		      // Load timeline. Tweets will be added.
		      reload_all_columns();
		      draw_all_columns();
		      break;
	    case 's': {
			  int r = msgbox("This is a very experimental feature. Are you sure you want to enable streaming?",msg_warning,2,yesno);
			  struct uicbctx* sc = malloc(sizeof(struct uicbctx));
			  sc->colnum = 0;
			  if (r == 0) startstreaming(columns[0],colset[0].acct,colset[0].tt,uistreamcb,sc);
			  draw_all_columns(); }
		      break;
	    case 't':
		      msgbox("Tweeting not yet supported.",msg_error,0,NULL);
		      draw_all_columns();
		      break;
	    case 'q': {
			  int r = msgbox("Are you sure you want to exit twtclt?",msg_info,2,yesno);
			  if (r == 0) uiloop = 0;
			  draw_all_columns();
			  break; }
	}

	//draw_column(0,scrollback,acctlist[0]->timelinebt);
    }

    endwin();

    return NULL;

}

void load_columns(FILE* file) {

    int i=0;

    char colsetline[256];

    while ( (i < MAXCOLUMNS) && (!feof(file)) ) {

/*int save_accounts() {
    FILE* db = cfopen("accounts.db","w"); if ((db == NULL) && (errno != ENOENT)) { perror("fopen"); return 1;}
    for (int i=0; i < acct_n; i++)
	fprintf(db,"%" PRId64 " %s %s %s %d %d\n",acctlist[i]->userid,acctlist[i]->name,acctlist[i]->key->tkey,acctlist[i]->key->tsct,acctlist[i]->show_in_timeline,acctlist[i]->auth);

    fflush(db);
    fclose(db);
    return 0;
}*/
/*	int r = fscanf(db,"%d %16s %128s %128s %hhd %d\n",&userid,name,tkey,tsct,&show_in_timeline,&auth);
	if (r != 6) { lprintf("%d fields returned instead of 6\n",r); return 1;}
	struct t_account* na = newAccount();
	na->userid = userid; na->auth = auth;
	na->show_in_timeline = show_in_timeline;

	strncpy(na->name,name,16);
	strncpy(na->key->tkey,tkey,128);
	strncpy(na->key->tsct,tsct,128);
	add_acct(na);
 */
    fclose(file);
    return;
    }

}

void init_columns() {

    FILE* colfile = cfopen("columns.cfg","r");

    if (colfile != NULL) load_columns(colfile); else {

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

    }
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

    init_columns();

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

    colhdrs = newwin(1,COLS,1,0);
    wbkgdset(colhdrs,COLOR_PAIR(9));
    draw_column_headers();

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

    int cursel = ( (cur_col == dc->column) && (id == colset[dc->column].curtwtid) );

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

	if ( (dc->lines == 0) || (boty >= dc->topline) || (topy <= dc->topline + dc->lines)) {
	    pnoutrefresh(tp,skipy,0,topy+2,( (dc->column - leftmostcol)  * colwidth),boty+1,(dc->column - leftmostcol +1) *colwidth - 1);
	}

    }

    dc->curline += lines;
    dc->row++;

    return;

}

void draw_headers() {

    for (int i=0; i < MAXCOLUMNS; i++) {

    }	

}

// column headers draw functions

int get_username(uint64_t uid, char* out, size_t maxsz) {
    struct t_user* u = get_user(NULL,uid,NULL);
    if (u == NULL) return 0; else strncpy(out,u->screen_name,maxsz);
    userdel(u); return strlen(out);
}

struct unread_ctx {
    uint64_t min_id;
    int unread_tweets;
};
void unread_cb(uint64_t id, void* param) {
    struct unread_ctx* ctx = (struct unread_ctx*) param;
    if (id > ctx->min_id) (ctx->unread_tweets)++;
}
int get_unread_number(int column) {
    
    struct unread_ctx ctx = {.min_id = colset[column].lastread,.unread_tweets=0};
    bt_read(columns[column],unread_cb,&ctx,desc);

    return ctx.unread_tweets;
}

void draw_coldesc(int column) {

    int length = colwidth - 8;

    WINDOW* newhdr = derwin(colhdrs, 1, colwidth - 8, 0, (column - leftmostcol) * colwidth);

    werase(newhdr);
    char coldesc[length+1];

    switch(colset[column].tt) {

	case home: {
		       snprintf(coldesc,length,"@%s",colset[column].acct->name);
		       break; }
	case user: {
		       char uname[16];
		       if (colset[column].customtype != NULL) strncpy(uname,colset[column].customtype,15); else {
			   int r = get_username(colset[column].userid,uname,15);
			   snprintf(coldesc,length,"@%s's tweets",(r >= 0 ? uname : NULL));
		       }
		       break; }
	case mentions: {
			   char uname[16];
			   if (colset[column].customtype != NULL) strncpy(uname,colset[column].customtype,15); else {
			       int r= get_username(colset[column].userid,uname,15);
			       snprintf(coldesc,length,"@%s's mentions",(r >=0 ? uname : NULL));
			   }
			   break; }
	case direct_messages: {
			snprintf(coldesc,length,"@%s's DMs",colset[column].acct->name);
				  break; }
	case search: {
			 snprintf(coldesc,32,"Search: %s",colset[column].customtype);
			 break; }
    }
    
    wprintw(newhdr,"%s",coldesc);

    touchwin(colhdrs);
    wrefresh(newhdr);
    delwin(newhdr);

}
void update_unread(int column) {

    WINDOW* unwin = derwin(colhdrs, 1, 8, 0, (column - leftmostcol) * colwidth + (colwidth - 8));

    werase(unwin);
    int unread = get_unread_number(column);

    if (unread) {
	char unreadstr[7];
	snprintf(unreadstr,6,"%d",unread);

	wmove(unwin,0,8 - strlen(unreadstr)-2-1);

	wattron(unwin,COLOR_PAIR(12));
	waddstr(unwin,"▄");
	wattron(unwin,COLOR_PAIR(13));
	waddstr(unwin,unreadstr);
	wattron(unwin,COLOR_PAIR(12));
	waddstr(unwin,"▀");
	wattroff(unwin,COLOR_PAIR(12));
    }
    touchwin(colhdrs);
    wrefresh(unwin);
    delwin(unwin);
}

// draw column functions

void draw_column_limit(int column, int scrollback, struct btree* timeline, int topline, int lines) {
    //btree should contain tweet IDs.

    bt_read(timeline, drawtwt_cb, NULL, desc);

    struct drawcol_ctx dc = { .curline=0, .column=column, .row=0, .scrollback=scrollback, .topline=topline, .lines=lines};
    bt_read(timeline, drawcol_cb, &dc, desc);

    //int curcol = (dc.curline - dc.scrollback);

    doupdate();
} 
void draw_column2(int column, int scrollback, struct btree* timeline, int do_update) {
    //btree should contain tweet IDs.

    bt_read(timeline, drawtwt_cb, NULL, desc);

    struct drawcol_ctx dc = { .curline=0, .column=column, .row=0, .scrollback=scrollback};
    bt_read(timeline, drawcol_cb, &dc, desc);

    //int curcol = (dc.curline - dc.scrollback);

    if (do_update) doupdate();
} 
void draw_column(int column, int scrollback, struct btree* timeline) {
    //btree should contain tweet IDs.
    draw_column2(column,scrollback,timeline,1);
} 


WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected) {
    if (tweet == NULL) return NULL;
    char tweettext[400];

    struct t_tweet* ot = (tweet->retweeted_status_id ? tht_search(tweet->retweeted_status_id) : tweet ); //original tweet

    char* text = parse_tweet_entities(ot);

    struct t_user* rtu = uht_search(ot->user_id);

    char* usn = rtu->screen_name;
    char* uname = rtu->name;

    utf8_wrap_text(text,tweettext,400,colwidth - 2); 

    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // * | Screen name
    // -------------------------
    // Tweet content
    // RT by screen name | time

    int textlines = countlines(tweettext,400);

    int lines = (tweet->retweeted_status_id ? textlines + 5 : textlines + 3);

    WINDOW* tp = newpad(lines, colwidth);

    chtype bkgtype = ( selected ? COLOR_PAIR(8) : COLOR_PAIR(3) );

    wbkgd(tp,bkgtype);

    wattron(tp,bkgtype);

    wattron(tp,A_BOLD);

    int specialtw = (tweet->retweeted_status_id);

    if (tweet->retweeted_status_id) wattron(tp,COLOR_PAIR(4));

    for (int i=0; i < colwidth; i++) {
	waddstr(tp, (specialtw ? "█" : "▔") );
    }
    wattroff(tp,A_BOLD);

    if (tweet->retweeted_status_id) wattroff(tp,COLOR_PAIR(4));
    wattron(tp,bkgtype);
    
    //wattron(tp,A_BOLD);
    mvwchgat(tp,1,1,colwidth-1,A_BOLD,PAIR_NUMBER(bkgtype),NULL);
    if (uname) mvwprintw(tp,1,1,"%s",uname); else mvwprintw(tp,1,1,"@%s",usn);
    //wattroff(tp,A_BOLD);

    char reltime[8];
    reltimestr(ot->created_at,reltime);
    mvwaddstr(tp,1,colwidth-1-strlen(reltime),reltime);

    WINDOW* textpad = subpad(tp,textlines,colwidth-1,3,1);

    mvwaddstr(textpad,0,0,tweettext);

    touchwin(tp);

    if (tweet->retweeted_status_id) {

	struct t_user* rtu = uht_search(tweet->user_id); 

	char* rtusn = (rtu ? rtu->screen_name : "(unknown)");
	char* rtuname = (rtu ? rtu->name : "NULL");

	if (rtuname) mvwprintw(tp,lines-1,1,"RT by %s",rtuname); else mvwprintw(tp,lines-1,1,"RT by @%s",rtusn);

	userdel(rtu);

	char rttime[8];
	reltimestr(tweet->created_at,rttime);
	mvwaddstr(tp,lines-1,colwidth-1-strlen(rttime),rttime);




    }

    //wbkgdset(tp,COLOR_PAIR(7));
    if (linecount != NULL) *linecount = lines;

    delwin(textpad);

    return tp;
}

