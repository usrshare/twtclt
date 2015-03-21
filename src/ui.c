// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <wchar.h>

#include "config.h"
#include "log.h"
#include "twitter.h"
#include "twt_oauth.h"
#include "twt_time.h"
#include "ui.h"
#include "stringex.h"
#include "utf8.h"

#include "ui_windows.h"


#define COLHEIGHT (LINES-3)

typedef uint32_t colpad_id;

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

struct t_ui_timeline {
    int enabled;
    struct t_account* acct;
    struct timeline_params param;
    struct btree* bt; //timeline btree
    streamhnd stream;
}; //column settings

#define MAXTIMELINES 64
#define MAXCOLUMNS 32

struct t_ui_timeline tls[MAXTIMELINES];

enum colpadtype {
    CP_NONE,
    CP_TWEET,
    CP_USER,
    CP_SEARCHBOX,
    CP_REFRESHBTN,
    CP_LOADMOREBTN,
    CP_COMPOSE,
};

struct t_colpad { //column pad
    enum colpadtype pt; //type
    int read; //has the user seen this pad?
    uint32_t pad_id; //pad ID, used for hashtable & sorting.
    uint64_t cnt_id; //content (tweet / user / etc. id)
    struct t_ui_timeline* tl; //timeline used
    struct t_account* acct;
    //int acct_id; //account id
    WINDOW* window; //pad's associated window
    int lines; //pad's height
    time_t mtime; //last update time
};

struct t_column {
    int enabled;
    int tl_c; //timeline count
    struct t_ui_timeline** tl; //timelines

    struct btree* padbt; //column pad btree

    struct t_column* parent; //parent column

    int scrollback;

    char title[128]; //up to 32 UTF-8 chars.

    WINDOW* header; //column header pad
    WINDOW* cnt; //column content pad

    pthread_mutex_t colmut; //column mutex

    pthread_cond_t colupd; //column updated conditional
    pthread_mutex_t colupd_mut; //column updated conditional mutex
};

struct t_column* cols[MAXCOLUMNS]; //current columns
struct t_column _cols[MAXCOLUMNS]; //permanent columns

uint8_t cur_col;
uint64_t curtwtid;

pthread_mutex_t screenmut = PTHREAD_MUTEX_INITIALIZER; 

WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected);
void draw_coldesc(int column);
int update_unread(int column, int colsleft);
void draw_column_limit(int column, int scrollback, int topline, int lines);
void draw_column2(int column, int scrollback, int do_update);
void draw_column(int column, int scrollback);

int col_id (struct t_column* col);

void render_timeline(struct btree* timeline, struct btree* padbt);
WINDOW* render_compose_pad(char* text, struct t_tweet* respond_to, struct t_account* acct, int* linecount, int selected, int cursor);
void render_column(struct t_column* column);

//----------------------------------------------------------- CODE

int findcolwidth(int minwidth) {

    int c = (COLS / minwidth);

    if (c > 0) visiblecolumns = c;

    if (c == 0) return COLS;

    return (COLS / c);
}

struct scrollto_ctx {
    int topline;
    int lines;
    int rows;
    int maxrow;
    int rline;
    uint64_t twtid;
    uint64_t lastid;
    int res_tl;
    int res_ln;
};
void scrollto_btcb(uint64_t id, void* data, void* ctx) {
    struct scrollto_ctx* sts = (struct scrollto_ctx*) ctx;

    if (sts->res_tl != -1) return;

    struct t_colpad* pad = (struct t_colpad*)data;

    if (pad == NULL) return;

    sts->topline += sts->lines;
    sts->lines = pad->lines;

    if (sts->rline != -1) {

	if ( (sts->topline <= sts->rline) && ((sts->topline + sts->lines) >= (sts->rline)) ) {
	    sts->res_tl = sts->topline;
	    sts->res_ln = sts->lines;
	    sts->twtid = pad->cnt_id;
	}
    }


    if ( (id <= sts->twtid) || (sts->rows == sts->maxrow) ) { 
	sts->res_tl = sts->topline;
	sts->res_ln = sts->lines;
    }

    sts->lastid = pad->cnt_id;

    sts->rows++;

    return;
}

int adjustscrollback(struct t_column* col, struct scrollto_ctx sts) {
    if (col->scrollback > sts.topline) col->scrollback = sts.topline;
    if (sts.topline - col->scrollback + sts.lines > COLHEIGHT) col->scrollback = sts.topline + sts.lines - COLHEIGHT;
    return 0;
}

int scrollto (struct t_column* col, int row) {
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .rline = -1, .maxrow = row, .twtid = 0, .res_tl = -1, .res_ln = 0};

    bt_read(col->padbt,scrollto_btcb,&sts,desc);
    if (sts.twtid == 0) sts.twtid = sts.lastid;

    adjustscrollback(col,sts);
    return (sts.rows - 1);
}
int scrolltotwt (struct t_column* col, uint64_t twtid) {
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .rline = -1, .maxrow = -1, .twtid = twtid, .res_tl = -1, .res_ln = 0};

    bt_read(col->padbt,scrollto_btcb,&sts,desc);
    if (sts.twtid == 0) sts.twtid = sts.lastid;

    adjustscrollback(col,sts);
    return (sts.rows - 1);
}

uint64_t scrolltoline (struct t_column* col, int line) {
    //in this case, line is the line number in the column, not on the screen. to get screen line number, remove scrollback for the column.
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .rline = line, .maxrow = -1, .twtid = 0, .res_tl = -1, .res_ln = 0};

    bt_read(col->padbt,scrollto_btcb,&sts,desc);
    if (sts.twtid == 0) sts.twtid = sts.lastid;

    adjustscrollback(col,sts);
    return sts.twtid;
}

int get_tweet_line(struct t_column* col, uint64_t twtid) {
    struct scrollto_ctx sts = {.topline = 0, .lines = 0, .rows = 0, .rline = -1, .maxrow = -1, .twtid = twtid, .res_tl = -1, .res_ln = 0};
    bt_read(col->padbt,scrollto_btcb,&sts,desc);

    int r = ((sts.res_tl) + (sts.res_ln / 2) + 1);

    lprintf("get_tweet_line returned %d\n",r);
    return r;
}

struct t_account* get_account(struct t_column* col) {
    //returns account associated with column's timeline(s).
    //if multiple accounts are, returns NULL.

    struct t_account* r = NULL;

    for (int i = 0; i < col->tl_c; i++) {
	if (col->tl[i]->acct != r) {
	    if (r == NULL) r = (col->tl[i]->acct); else return NULL;
	}
    }

    return r;
}

//---

#define min(x,y) ( (x) < (y) ? (x) : (y) )

struct t_account* accounts_menu(int allow_cancel, char* custom_title) {

    int items = (allow_cancel ? acct_n + 1 : acct_n);

    struct menuitem acctmenu[items];

    for (int i=0; i < acct_n; i++) 
	acctmenu[i] = (struct menuitem){.id = i, .name = acctlist[i]->name};

    if (allow_cancel) acctmenu[acct_n] = (struct menuitem){.id = UINT64_MAX, .name = "Cancel", .desc = NULL};

    uint64_t r = menu(custom_title ? custom_title : "Please select an account.",msg_info,items,acctmenu); 

    if (r != UINT64_MAX) return acctlist[r]; else return NULL;
}

int describe_column (struct t_column* col, char* out, size_t maxsize) {

    return 0;

} 

int describe_timeline (struct t_ui_timeline* tl, char* out, size_t maxsize) {

    switch(tl->param.tt) {
	case home:
	    snprintf(out,maxsize,"@%s: Home",tl->acct->name); break;
	case user:
	    snprintf(out,maxsize,"@%s: Tweets from @???",tl->acct->name); break; //TODO user name
	case mentions:
	    snprintf(out,maxsize,"@%s: Mentions",tl->acct->name); break;
	case direct_messages:
	    snprintf(out,maxsize,"@%s: Direct messages",tl->acct->name); break;
	case search:
	    snprintf(out,maxsize,"@%s: Search for %s",tl->acct->name,tl->param.query); break;
    }

    if (tl->stream) strcat(out, " (Streaming)");

    return 0;
}

struct t_ui_timeline* timelines_menu (struct t_column* col, int allow_cancel, char* custom_title) {

    int items = (allow_cancel ? col->tl_c + 1 : col->tl_c);

    struct menuitem tlmenu[items];

    char descriptions[128 * items];

    for (int i=0; i < col->tl_c; i++) {
	describe_timeline(col->tl[i],descriptions+(128*i),127);

	tlmenu[i] = (struct menuitem){.id = i, .name = descriptions+(128*i), .desc = NULL};
    }

    if (allow_cancel) tlmenu[col->tl_c] = (struct menuitem){.id = UINT64_MAX, .name = "Cancel"};

    uint64_t r = menu(custom_title ? custom_title : "Please select a timeline.",msg_info,items,tlmenu); 

    if (r != UINT64_MAX) return col->tl[r]; else return NULL;
}

int draw_column_headers() {
    for (int i=leftmostcol; i < min(leftmostcol + visiblecolumns,MAXCOLUMNS); i++) {

	if (cols[i]->enabled) {

	    //draw_column(i,colset[i].scrollback,columns[i]);

	    int colsleft = colwidth - 1;

	    colsleft -= update_unread(i, colsleft);

	    draw_coldesc(i);
	}
    }
    return 0;
}
int draw_all_columns() {

    for (int i=leftmostcol; i < min(leftmostcol + visiblecolumns,MAXCOLUMNS); i++) {

	if (cols[i]->enabled) {

	    draw_column(i,cols[i]->scrollback);
	}
    }

    draw_column_headers();

    return 0;
}

int redraw_lines(int topline, int lines) {

    for (int i=0; i < MAXCOLUMNS; i++) {

	if (cols[i]->enabled) {

	    draw_column_limit(i,cols[i]->scrollback,topline,lines);
	}
    }

    return 0;
}

struct loadedcb_ctx {
    struct t_ui_timeline* tl;
    struct t_column* col;
};

void loaded_cb(int op_id, int op_type, void* param) {
    struct loadedcb_ctx* ctx = (struct loadedcb_ctx*)param;
    render_timeline(ctx->tl->bt, ctx->col->padbt);
    draw_column(col_id(ctx->col),ctx->col->scrollback);

    free(ctx);

}

int reload_column(int col) {

    for (int i=0; i < cols[col]->tl_c; i++) {

	struct t_ui_timeline* tl = cols[col]->tl[i];

	struct loadedcb_ctx* ctx = malloc(sizeof(struct loadedcb_ctx));
	ctx->tl = tl; ctx->col = cols[col];

	load_timeline2(tl->bt, tl->acct, tl->param, loaded_cb, ctx);
    }
    return 0;
}

int reload_all_columns() {

    for (int i=0; i < MAXCOLUMNS; i++) {

	if (cols[i]->enabled) {

	    reload_column(i);
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

void row_tweet_cb(uint64_t id, void* data, void* param) {
    struct row_tweet_ctx* ctx = (struct row_tweet_ctx*) param;

    if ( (ctx->index == -1) && (ctx->id = id) )
	ctx->index = ctx->cur_ind;

    if ( (ctx->id == 0) && (ctx->index == ctx->cur_ind) )
	ctx->id = id;

    ctx->cur_ind++;
}
void row_tweet_shift_cb(uint64_t id, void* data, void* param) {
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

uint64_t row_tweet_shift(struct t_column* col, uint64_t id, int indexdiff) {

    //returns the tweet id that's N columns above or below in the column list.2
    if (col->enabled == 0) return 0;

    struct row_tweet_ctx ctx = {.id = id, .index = 0, .shift = abs(indexdiff), .res_id = 0, .diff_less = (indexdiff < 0) };

    if (indexdiff == 0) return id;

    bt_read(col->padbt, row_tweet_shift_cb, &ctx, ((indexdiff > 0) ? desc : asc));

    uint64_t border_id = (indexdiff > 0 ? 0 : UINT64_MAX );

    uint64_t res = ( ctx.res_id ? ctx.res_id : border_id );

    lprintf("row_tweet_shift %d on %" PRIu64 " returns %" PRIu64 "\n",indexdiff,id,res);
    return res;
}
uint64_t row_tweet_index(struct t_column* col, uint64_t id, int index) {

    //either returns the tweet id, if id is zero, or index, if index is -1.
    if ((index != -1) && (id != 0)) return 0;
    if (!(col->enabled)) return 0;

    struct row_tweet_ctx ctx = {.id = id, .cur_ind = 0, .index = index};

    bt_read(col->padbt, row_tweet_cb, &ctx, desc);

    if (index == -1) return ctx.index;
    if (id == 0) return ctx.id;
    return 0;
}

// row_tweet end

struct uicbctx {
    struct t_column* col;
    struct t_ui_timeline* tl;
};

void uistreamcb(uint64_t id, void* cbctx) {

    struct uicbctx* ctx = (struct uicbctx *)cbctx;
    struct t_column* col = ctx->col;

    render_timeline(ctx->tl->bt, ctx->col->padbt);

    struct t_colpad* twt = bt_data(ctx->col->padbt,id);
    if (col->scrollback != 0) col->scrollback += (twt->lines);

    draw_column(col_id(ctx->col),ctx->col->scrollback);
    //draw_column2(ctx->colnum,cols[ctx->colnum].scrollback,1);

}


struct menuitem optionItems[] = {

    {   1,"Accounts...","Add, delete and manage your Twitter accounts.",0},
    {   2,"Columns...","Add, delete and manage your columns and timelines.",0},
    {   0,"---","",1},
    {   7,"About","Information about twtclt.",0},

};

char* twtclt_about = \

    "twtclt - a curses-based Twitter client\n"
    "(c) 2014-2015 usr_share and twtclt contributors\n"
    "(http://github.com/usrshare/twtclt/)\n"
    "\n";


    int ui_optionsScreen() { 
	uint64_t r = menu("twtclt Option Menu",msg_info,sizeof(optionItems) / sizeof(*optionItems),optionItems);

	switch(r) {
	    case 1:
	    case 2:
		msgbox("This menu item has not been implemented yet.",msg_warning,0,0);
		break;

	    case 7:
		msgbox(twtclt_about,msg_info,0,0);
		break;
	}
	return 0;
    }

void* uiupdfunc(void* param) {

    // This thread updates the screen.

}

void* uithreadfunc(void* param) {

    // This thread processes user input.

    cur_col = 0;
    curtwtid = UINT64_MAX; //first possible tweet

    int uiloop = 1;

    while(uiloop) {

	mvwprintw(statusbar,0,0,"Ready.\n"); wrefresh(statusbar);

	int k = wgetch(inputbar); 

	lprintf("Hit key %d\n",k);

	switch(k) {

	    case '\x1B': //escape key
		msgbox("Escape key hit!\n",msg_warning,0,NULL);

	    case 'J':
		// scroll down a line
		cols[cur_col]->scrollback++;
		draw_all_columns();
		break;
	    case 'K':
		// scroll up a line
		if (cols[cur_col]->scrollback > 0) cols[cur_col]->scrollback--;
		draw_all_columns();
		break;
	    case 'l':
		// Show all links in the selected tweet
		break;
	    case KEY_LEFT: {
			       // Select next tweet, TODO make scrolling follow selection
			       int old_col = cur_col;
			       if (cur_col > 0) cur_col--;
			       if (cur_col < leftmostcol) leftmostcol = cur_col;
			       curtwtid = scrolltoline(cols[cur_col],get_tweet_line(cols[old_col],curtwtid) - cols[old_col]->scrollback + cols[cur_col]->scrollback);
			       draw_column_headers();
			       draw_all_columns();
			       break; }
	    case KEY_RIGHT: {
				// Select previous tweet, TODO make scrolling follow selection
				int old_col = cur_col;
				if (cols[cur_col+1]->enabled) cur_col++;
				if (cur_col >= (visiblecolumns + leftmostcol)) leftmostcol = (cur_col - visiblecolumns + 1);
				curtwtid = scrolltoline(cols[cur_col],get_tweet_line(cols[old_col],curtwtid) - cols[old_col]->scrollback + cols[cur_col]->scrollback);
				draw_column_headers();
				draw_all_columns();
				break; }
	    case 'j':
	    case KEY_DOWN: {
			       // Select next tweet, TODO make scrolling follow selection
			       curtwtid = row_tweet_shift(cols[cur_col], curtwtid, 1);
			       scrolltotwt(cols[cur_col],curtwtid);
			       draw_all_columns();
			       break; }
	    case 'k':
	    case KEY_UP: {
			     // Select previous tweet, TODO make scrolling follow selection
			     curtwtid = row_tweet_shift(cols[cur_col], curtwtid, -1);
			     scrolltotwt(cols[cur_col],curtwtid);
			     draw_all_columns();
			     break; }
	    case KEY_HOME:
			 // Scroll to the top
			 curtwtid = UINT64_MAX;
			 scrolltotwt(cols[cur_col],curtwtid);
			 draw_all_columns();
			 break;
	    case KEY_END:
			 // Scroll to the bottom
			 curtwtid = 0;
			 scrolltotwt(cols[cur_col],curtwtid);
			 draw_all_columns();
			 break;
	    case KEY_PPAGE:
			 // Scroll one page up
			 if (curtwtid != UINT64_MAX) {

			     int l = get_tweet_line(cols[cur_col],curtwtid) - COLHEIGHT; if (l < 0) l = 0;
			     curtwtid = scrolltoline(cols[cur_col],l);
			     scrolltotwt(cols[cur_col],curtwtid);
			     draw_all_columns(); }
			 break;
	    case KEY_NPAGE:
			 // Scroll one page down
			 if (curtwtid != 0) {
			     curtwtid = scrolltoline(cols[cur_col],get_tweet_line(cols[cur_col],curtwtid) + COLHEIGHT);
			     scrolltotwt(cols[cur_col],curtwtid);
			     draw_all_columns(); }
			 break;
	    case 'o': {
			  ui_optionsScreen();
			  break; }
	    case 'a': {
			  ui_addAccount();
			  break; }
	    case 'z': {
			  // Load timeline. Tweets will be added.
			  char test[11];
			  int r = inputbox_utf8("Test?",msg_info,test,8,10);
			  if (r == 0) msgbox(test,msg_error,0,NULL);
			  draw_all_columns();
			  break; }
	    case 'b': {
			  // Load timeline. Tweets will be added.
			  int r = msgbox("Lol.",msg_info,2,okcanc);
			  if (r) msgbox("Option 2?",msg_error,0,NULL);
			  draw_all_columns();
			  break; }
	    case 'm': {

			  break; }
	    case 'r':
		      // Load timeline. Tweets will be added.
		      reload_all_columns();
		      draw_all_columns();
		      break;

	    case 's': {
			      struct t_ui_timeline* tl = timelines_menu(cols[cur_col],1,"Please select a timeline to enable/disable streaming for.");
			      if (tl != NULL) {

				  //enable streaming on this timeline

				  if (tl->stream == NULL) {
				      struct uicbctx* sc = malloc(sizeof(struct uicbctx));
				      sc->col = cols[cur_col];
				      sc->tl = tl;

				      tl->stream = startstreaming(tl->bt,tl->acct,tl->param.tt,uistreamcb,sc); } else {
					  stopstreaming(tl->stream);
					  tl->stream = 0;

				      }
			      }
			  draw_all_columns();
			  break; }
	    case 'i':
	    case 't': {
			  char newtweet[640];
			  memset(newtweet,0,640);
			  compose(cur_col,newtweet,140,640);
			  draw_all_columns();
			  break; }
	    case 'q': {
			  int r = msgbox("Are you sure you want to exit twtclt?",msg_info,2,yesno);
			  if (r == 0) uiloop = 0;
			  draw_all_columns();
			  break; }
	}

	//draw_column(0,scrollback,acctlist[0]->timelinebt);
    }

    FILE* colfile = cfopen("columns.cfg","w");
    if (colfile != NULL) save_columns(colfile); 

    endwin();

    return NULL;

}

int save_columns(FILE* file) {

    for (int i=0; i < MAXCOLUMNS; i++) {
	fprintf(file,"%d %d\n",_cols[i].enabled,_cols[i].tl_c);

	for (int j=0; j < _cols[i].tl_c; j++) {
	    struct t_ui_timeline* ctl = _cols[i].tl[j];
	    fprintf(file,"* %d %d %d %" PRIu64 "\n",ctl->enabled,acct_id(ctl->acct),ctl->param.tt,ctl->param.user_id); }
    }

    fflush(file);
    fclose(file);
    return 0;
}


int load_columns(FILE* file) {

    int i=0, tli=0;

    for (int i=0; i < MAXCOLUMNS; i++) {
	cols[i] = &(_cols[i]); _cols[i].enabled = 0; }

    while ( (i < MAXCOLUMNS) && (!feof(file)) ) {

	int c_enabled, tl_c;

	int t_enabled, acct_id; enum timelinetype tt; uint64_t userid; //char custom[64];

	int r = fscanf(file,"%d %d\n",&c_enabled,&tl_c);
	if (r != 2) { lprintf("C: %d fields returned instead of 2\n",r); return 1;}

	_cols[i].tl = malloc(sizeof(struct t_ui_timeline*) * tl_c);

	for (int t=0; t<tl_c; t++) {

	    if (tli < MAXTIMELINES) {
		_cols[i].tl[t] = &(tls[tli]);

		int r = fscanf(file,"* %d %d %d %" SCNu64 "\n",&t_enabled,&acct_id,(int *)&tt,&userid);
		if (r != 4) { lprintf("T: %d fields returned instead of 4\n",r); return 1;}

		tls[tli].enabled = t_enabled;
		tls[tli].acct = (acct_id != -1 ? acctlist[acct_id] : NULL);
		tls[tli].param.tt = tt;
		tls[tli].param.user_id = userid;
		tls[tli].bt = bt_create();

		tli++;
	    }
	}
	_cols[i].tl_c = tl_c;

	_cols[i].enabled = c_enabled;
	_cols[i].padbt = bt_create();

	cols[i] = &(_cols[i]);
	i++;
    }

    fclose(file);
    return 0;
}

void init_columns() {

    FILE* colfile = cfopen("columns.cfg","r");

    if (colfile != NULL) load_columns(colfile); else {

	_cols[0].enabled = 1;
	_cols[0].tl_c = 1; _cols[0].tl = malloc(sizeof(struct t_ui_timeline) * 1); _cols[0].tl[0] = &tls[0];
	_cols[0].padbt = bt_create();
	cols[0] = &(_cols[0]);

	tls[0].enabled = 1;
	tls[0].acct = acctlist[0];
	tls[0].param = (struct timeline_params){.tt = home};
	tls[0].bt = bt_create();
	tls[0].stream = NULL;

	_cols[1].enabled = 1;
	_cols[1].tl_c = 1; _cols[1].tl = malloc(sizeof(struct t_ui_timeline) * 1); _cols[1].tl[0] = &tls[1];
	_cols[1].padbt = bt_create();
	cols[1] = &(_cols[1]);

	tls[1].enabled = 1;
	tls[1].acct = acctlist[0];
	tls[1].param = (struct timeline_params){.tt = mentions};
	tls[1].bt = bt_create();
	tls[1].stream = NULL;

    }
    return;
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
	_cols[i].enabled = 0;
	_cols[i].tl_c = 0;

	cols[i] = NULL;
    }

    init_columns();

    start_color();	
    cbreak();

    noecho();

    blackcolor = (COLORS > 16) ? 16 : COLOR_BLACK; //256col black, for a better bold.
    whitecolor = (COLORS > 16) ? 231 : COLOR_WHITE; //256col white.
    twtcolor = (COLORS > 16) ? 111 : COLOR_CYAN; //twitter logo color
    twtcolor2 = (COLORS > 16) ? 68 : COLOR_BLUE; //twitter logo color, darker
    bgcolor = (COLORS > 16) ? 231 : COLOR_YELLOW; //background color
    selbgcolor = (COLORS > 16) ? 253 : COLOR_WHITE; //selected bg color
    hdrcolor = (COLORS > 16) ? 254 : COLOR_WHITE; //header bg color
    gray1 = (COLORS > 16) ? 245 : COLOR_BLACK + 8; //roughly equiv to #888888

    init_pair(15,whitecolor,blackcolor); //background
    bkgdset(CP_BG); 

    init_pair(1,twtcolor, twtcolor2); //background
    init_pair(2,COLOR_YELLOW,COLOR_BLUE); //bars
    init_pair(3,blackcolor,bgcolor); //cards
    init_pair(4,COLOR_GREEN,bgcolor); //retweet indication
    init_pair(5,COLOR_YELLOW,bgcolor); //fave indication
    init_pair(6,COLOR_BLUE,bgcolor); //mention indication
    init_pair(7,gray1,bgcolor); //grayed out text

    init_pair(8,blackcolor,selbgcolor); //selected bg
    init_pair(9,gray1,hdrcolor); //headers

    init_pair(10,COLOR_BLUE,blackcolor); //bars
    init_pair(11,COLOR_YELLOW,blackcolor); //bars
    init_pair(12,COLOR_RED,blackcolor); //bars
    init_pair(13,COLOR_WHITE,COLOR_RED); //bars

    init_pair(14,gray1,selbgcolor); //headers selected bg
    init_pair(16,gray1,selbgcolor); //grayed out selected

    init_pair(17,whitecolor, twtcolor2); //compose bg 1
    init_pair(18,whitecolor, twtcolor); //compose bg 2
    
    init_pair(19,COLOR_GREEN,bgcolor);    //retweet unselected
    init_pair(20,COLOR_GREEN,selbgcolor); //retweet selected

    init_pair(21,COLOR_YELLOW,bgcolor);    //fave unselected
    init_pair(22,COLOR_YELLOW,selbgcolor); //fave selected


    keypad(stdscr, TRUE);
    timeout(100);

    titlebar = newwin(1,COLS,0,0);
    wbkgd(titlebar,CP_TWTBG);
    wprintw(titlebar,"twtclt");
    wrefresh(titlebar);

    statusbar = newwin(1,(COLS-1),(LINES-1),0);
    wbkgd(statusbar,CP_TWTBG);

    inputbar = newwin(1,1,(LINES-1),(COLS-1));
    keypad(inputbar, TRUE);
    wbkgd(inputbar,CP_TWTBG);

    wrefresh(titlebar);

    colhdrs = newwin(1,COLS,1,0);
    wbkgdset(colhdrs,CP_HDR);
    draw_column_headers();

    colarea = newwin(COLHEIGHT,COLS,2,0);
    wbkgdset(colarea,ACS_CKBOARD | CP_TWTBG);
    wbkgd(colarea,ACS_CKBOARD | CP_TWTBG);
    wrefresh(colarea);
    wrefresh(statusbar);
    wrefresh(inputbar);

    pthread_t* keythread = malloc(sizeof(pthread_t));
    int r = pthread_create(keythread,NULL,uithreadfunc,NULL);

    if (r != 0) lprintf("pthread_create returned %d\n",r);

    return keythread;
}

void rendertwt_cb(uint64_t id, void* data, void* ctx) {

    //draws an individual tweet into its respective pad, then stores the tweetbox in the respective columns' btree.

    if (ctx == NULL) return;
    struct btree* colbt = (struct btree*) ctx;

    struct t_colpad* pad = (struct t_colpad*) bt_data(colbt,id); 

    if (pad == NULL) {

	int lines;

	struct t_colpad* newpad = malloc(sizeof(struct t_colpad));

	newpad->acct = NULL;
	newpad->pt = CP_TWEET;
	newpad->cnt_id = id;
	newpad->read = 0;
	newpad->mtime = time(NULL);

	struct t_tweet* tt = tht_search(id);
	if (tt == NULL) return;

	WINDOW* tp = tweetpad(tt,&lines,0);
	if (tp == NULL) {free(newpad); return;}

	newpad->window = tp;
	newpad->lines = lines;

	tweetdel(tt);

	int r = bt_insert(colbt,id,newpad);
	if (r != 0) lprintf("pad_insert returned %d\n",r);
    }
    return;
}

int compose(int column, char* textbox, size_t maxchars, size_t maxbytes) {

    struct t_colpad* cpad = malloc(sizeof(struct t_colpad));

    cpad->pt = CP_COMPOSE; cpad->read = 1; cpad->pad_id = 0xFFFFFFFF; cpad->cnt_id = UINT64_MAX; cpad->tl = NULL; cpad->acct = get_account(cols[column]); cpad->mtime = time(NULL);
    int composing = 1, cwlines = 0, ocwlines = 0; //compose window lines

    WINDOW* composepad = NULL, *ocp = NULL;

    bt_insert(cols[column]->padbt,UINT64_MAX,cpad);

    int curpos = 0;

    wint_t wch;

    composepad = render_compose_pad(textbox, NULL, cpad->acct, &cwlines, 1, curpos);
    cpad->lines = cwlines;
    cpad->window = composepad;
    draw_column(column,0);

    while (composing) {

	keypad(composepad, TRUE);
	int ctype = wget_wch(composepad, &wch);

	int utf8_count_chars(const char* text);

	if (ctype == OK) {
	    lprintf("got wide character %d\n",wch);

	    //received a character

	    switch(wch) {
		case 1:	
		case 21: { //CTRL+a, CTRL+u
			     struct t_account* acct = accounts_menu(1,"Please select an account you would like to send this tweet from.");
			     if (acct) cpad->acct = acct;
			     break; }
		case 20:   //CTRL+t
		case 23: { //CTRL+w, write tweet
			     uint64_t tid = update_status(cpad->acct, textbox, 0);
			     if (tid) composing = 0; else msgbox ("Some error happened while sending this tweet.\n",msg_error,0,NULL);
			     break; }
		case 27: //escape
			 composing = 0;
			 break;
		case 127: { //backspace
			      int r = utf8_delete_char((uint8_t*)textbox,maxchars,curpos-1);
			      curpos--; if (curpos <0) {curpos = 0; beep();}
			      if (r == 1) beep();	
			      break; }
		default: {
			     int r = utf8_insert_char((uint8_t*)textbox,maxbytes,curpos,wch);
			     curpos++; if (curpos > utf8_count_chars(textbox)) {curpos = utf8_count_chars(textbox); beep();}
			     if (r == 1) beep();
			     break; }
	    }
	}
	else if (ctype == KEY_CODE_YES) {
	    lprintf("got functionn key %d\n",wch);
	    switch(wch) {

		case KEY_LEFT: {
				   curpos--; if (curpos < 0) { curpos=0; beep();} break; }
		case KEY_RIGHT: {
				    curpos++; if (curpos > utf8_count_chars(textbox)) {curpos = utf8_count_chars(textbox); beep();} break; }

		case KEY_BACKSPACE: {
					int r = utf8_delete_char((uint8_t*)textbox,maxchars,curpos-1);
					curpos--; if (curpos <0) {curpos = 0; beep();}
					if (r == 1) beep();	
					break; }	

		case KEY_DC: {
				 int r = utf8_delete_char((uint8_t*)textbox,maxchars,curpos);
				 if (curpos > utf8_count_chars(textbox)) {curpos = utf8_count_chars(textbox);}
				 if (r == 1) beep();
				 break; }
		case KEY_HOME: {
				   curpos = 0; break; }
		case KEY_END:
			       {
				   curpos = utf8_count_chars(textbox); break; }
		default: {
			     beep();
			     break; }
	    }
	}

	ocp = composepad;

	composepad = render_compose_pad(textbox, NULL, cpad->acct, &cwlines, 1, curpos);
	cpad->lines = cwlines;

	cpad->window = composepad;
	if (ocp) { delwin(ocp); ocp = NULL; }

	draw_column(column,0);
    }

    if (composepad) { delwin(composepad); composepad = NULL; }

    bt_remove(cols[column]->padbt,UINT64_MAX);

    draw_column(column,cols[column]->scrollback);

    return 0;
}

void drawcol_cb(uint64_t id, void* data, void* ctx) {

    struct drawcol_ctx *dc = (struct drawcol_ctx *) ctx;

    WINDOW* tp;

    int lines;

    struct t_colpad* pad = bt_data(cols[dc->column]->padbt,id); 

    int cursel = ( (cur_col == dc->column) && (id == curtwtid) );

    if (pad == NULL) return;

    if ((pad->pt == CP_TWEET) && (cursel)) {

	struct t_tweet* tt = tht_search(id);
	if (tt == NULL) return;

	tp = tweetpad(tt,&lines,1);
	if (tp == NULL) return;

	tweetdel(tt);

    } else { tp = pad->window; lines = pad->lines; }

    if ((dc->curline + lines >= dc->scrollback) && (dc->curline - dc->scrollback <= COLHEIGHT -1 )) {

	int skipy = -(dc->curline - dc->scrollback);
	int topy = ( (dc->curline - dc->scrollback > 0) ? (dc->curline - dc->scrollback) : 0);
	int boty = ( (dc->curline - dc->scrollback + lines <= COLHEIGHT) ? (dc->curline - dc->scrollback + lines) : COLHEIGHT);

	if ( (dc->lines == 0) || (boty >= dc->topline) || (topy <= dc->topline + dc->lines)) {
	    pnoutrefresh(tp,skipy,0,topy+2,( (dc->column - leftmostcol)  * colwidth),boty+1,(dc->column - leftmostcol +1) *colwidth - 1);
	    pad->read = 1;
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
    int unread_tweets;
};

void unread_cb(uint64_t id, void* data, void* param) {
    struct unread_ctx* ctx = (struct unread_ctx*) param;
    if (data) { struct t_colpad* pad = (struct t_colpad*) data;
	if (!(pad->read)) (ctx->unread_tweets)++; }
}
int get_unread_number(int column) {

    struct unread_ctx ctx = {.unread_tweets=0};
    bt_read(cols[column]->padbt,unread_cb,&ctx,desc);

    return ctx.unread_tweets;
}

void draw_coldesc(int column) {

    int length = colwidth - 8;

    WINDOW* newhdr = derwin(colhdrs, 1, colwidth - 8, 0, (column - leftmostcol) * colwidth);

    werase(newhdr);

    wprintw(newhdr,"%32s",cols[column]->title);

    touchwin(colhdrs);
    wrefresh(newhdr);
    delwin(newhdr);

}
int update_unread(int column, int colsleft) {

    WINDOW* unwin = derwin(colhdrs, 1, 8, 0, (column - leftmostcol) * colwidth + (colwidth - 8));

    werase(unwin);
    int unread = get_unread_number(column);

    int r = 0;

    if (unread) {
	char unreadstr[7];
	snprintf(unreadstr,6,"%d",unread);

	wmove(unwin,0,8 - strlen(unreadstr)-1);

	r = strlen(unreadstr) + 1;
	wattron(unwin,CP_UNREAD);
	waddstr(unwin,unreadstr);
	wattroff(unwin,CP_UNREAD);
    }
    touchwin(colhdrs);
    wrefresh(unwin);
    delwin(unwin);
    return r;
}

// draw column functions

uint64_t unique_id (struct btree* timeline, uint64_t id) {
    //looks for ID in a btree and increments until free ID is found.

    while (bt_contains(timeline,id) == 0)
	id++;

    return id;    
}

void draw_bg(int column, int startline) {

    if ((column - leftmostcol) < 0) return;
    if ((column - leftmostcol) >= visiblecolumns) return;

    chtype backgd[colwidth];
    for (int i=0; i < colwidth; i++) backgd[i] = (ACS_CKBOARD | CP_TWTBG);

    for (int y=startline; y < LINES-1; y++) {
	mvaddchnstr(y, (column - leftmostcol) * colwidth , backgd, colwidth);
    }
    doupdate();
}

void draw_column_limit(int column, int scrollback, int topline, int lines) {
    //btree should contain tweet IDs.

    struct drawcol_ctx dc = { .curline=0, .column=column, .row=0, .scrollback=scrollback, .topline=topline, .lines=lines};
    bt_read(cols[column]->padbt, drawcol_cb, &dc, desc);

    draw_bg(column,dc.curline - dc.scrollback + 1);
    //int curcol = (dc.curline - dc.scrollback);
    doupdate();
} 

void draw_column2(int column, int scrollback, int do_update) {
    //btree should contain tweet IDs.

    struct drawcol_ctx dc = { .curline=0, .column=column, .row=0, .scrollback=scrollback};
    bt_read(cols[column]->padbt, drawcol_cb, &dc, desc);

    draw_bg(column,dc.curline - dc.scrollback + 1);
    //int curcol = (dc.curline - dc.scrollback);

    if (do_update) doupdate();
} 

void render_timeline(struct btree* timeline, struct btree* padbt) {
    bt_read(timeline, rendertwt_cb, (void*)padbt, desc);
}

void render_column(struct t_column* column) {

    // 1. updates column btrees based on timeline btree contents.
    // 2. draws pads for tweets which don't have pads yet.

    for (int i=0; i < (column->tl_c); i++) {
	render_timeline(column->tl[i]->bt, column->padbt);
    }


    //btree should contain tweet IDs.

} 

void draw_column(int column, int scrollback) {

    if (column < 0) return;
    draw_column2(column,scrollback,1);
}

int col_id (struct t_column* col) {
    for (int i=0; i< MAXCOLUMNS; i++)
	if (cols[i] == col) return i;

    return -1;
}

WINDOW* render_compose_pad(char* text, struct t_tweet* respond_to, struct t_account* acct, int* linecount, int selected, int cursor) {

    if (acct == NULL) acct = def_acct(NULL);

    if (respond_to != NULL) {


    }

    char comptext[640];

    int cp = cursor;

    utf8_wrap_text2(text,comptext,640,colwidth - 2,&cp); 

    int ucp = point_to_char_by_idx(comptext,cp) - comptext;

    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // * | Screen name
    // -------------------------
    // Tweet content
    // RT by screen name | time

    lprintf("CP = %d, UCP = %d\n",cp,ucp);

    int textlines = countlines(comptext,640);

    int lines = (respond_to ? textlines + 6 : textlines + 4);

    WINDOW* tp = newpad(lines, colwidth);

    chtype bkgtype = ( selected ? CP_COMPSEL : CP_COMPBG1 );
    chtype texttype = ( selected ? CP_CARD : CP_GRAY );

    wbkgd(tp,bkgtype);

    wattron(tp,bkgtype);

    int response = ( respond_to != NULL);
    if (response) {
	wattron(tp,CP_MENT);
	mvwaddstr(tp,0,1,"█");
	wattroff(tp,CP_MENT);
    }

    wattron(tp,bkgtype);

    if (COLORS > 16) wattron(tp,A_BOLD);
    mvwprintw(tp,1,1,"@%s:",acct->name);
    if (COLORS > 16) wattroff(tp,A_BOLD);

    WINDOW* textpad = subpad(tp,textlines,colwidth-1,3,1);

    wattron(textpad,texttype);

    WINDOW* clearpad = subpad(tp,textlines,colwidth-2,3,1);

    wbkgdset(clearpad,texttype);

    mvwaddnstr(textpad,0,0,comptext,ucp);
    wattron(textpad,A_REVERSE);
    char* cchar = point_to_char_by_idx(comptext, cp);
    if ((cchar == NULL) || (*cchar == 0)) cchar = " ";
    waddnstr(textpad,cchar,utf8_charcount((uint8_t*)cchar));
    wattroff(textpad,A_REVERSE);
    cchar = point_to_char_by_idx(comptext, cp+1);
    if (cchar && *cchar) waddstr(textpad,cchar);
    wattroff(textpad,texttype);

    delwin(clearpad);

    touchwin(tp);

    if (linecount != NULL) *linecount = lines;

    delwin(textpad);

    wmove(tp, lines-1, colwidth-1);

    return tp;
}

WINDOW* userpad(struct t_user* user, int* linecount, int selected) {
    if (user == NULL) return NULL;
    char tweettext[640];

    return NULL;
}

WINDOW* tweetpad(struct t_tweet* tweet, int* linecount, int selected) {
    if (tweet == NULL) return NULL;
    char tweettext[640];

    struct t_tweet* ot = (tweet->retweeted_status_id ? tht_search(tweet->retweeted_status_id) : tweet ); //original tweet

    char* text = parse_entities(ot->text, ot->entity_count, ot->entities);

    struct t_user* rtu = uht_search(ot->user_id);

    char* usn = rtu->screen_name;
    char* uname = rtu->name;

    utf8_wrap_text(text,tweettext,640,colwidth - 2); 

    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // * | Screen name
    // -------------------------
    // Tweet content
    // RT by screen name | time

    int textlines = countlines(tweettext,640);

    int lines = (tweet->retweeted_status_id ? textlines + 5 : textlines + 4);

    WINDOW* tp = newpad(lines, colwidth);

    chtype bkgtype = ( selected ? CP_CARDSEL : CP_CARD );
    chtype graytype = ( selected ? CP_GRAYSEL : CP_GRAY );

    wbkgd(tp,bkgtype);

    wattron(tp,bkgtype);

    wattron(tp,graytype);
    for (int i=0; i < colwidth; i++) {
	waddstr(tp, "▔");
    }
    wattroff(tp,graytype);

    int specialtw = (tweet->retweeted_status_id);
    if (specialtw) {
	wattron(tp,CP_RT);
	mvwaddstr(tp,0,1,"█");
	wattroff(tp,CP_RT);
    }

    wattron(tp,bkgtype);

    if (specialtw) {

	wattron(tp,graytype);

	struct t_user* rtu = uht_search(tweet->user_id); 

	char* rtusn = (rtu ? rtu->screen_name : "(unknown)");
	char* rtuname = (rtu ? rtu->name : "NULL");

	if (rtuname) mvwprintw(tp,1,1,"%s retweeted",rtuname); else mvwprintw(tp,1,1,"@%s retweeted",rtusn);

	userdel(rtu);

	char rttime[8];
	reltimestr(tweet->created_at,rttime);
	mvwaddstr(tp,1,colwidth-1-strlen(rttime),rttime);

	wattroff(tp,graytype);
    }
    
    int sl = (tweet->retweeted_status_id ? 2 : 1);

    if (COLORS > 16) wattron(tp,A_BOLD);
    if (uname) mvwprintw(tp,sl,1,"%s",uname); else mvwprintw(tp,sl,1,"@%s",usn);
    if (COLORS > 16) wattroff(tp,A_BOLD);

    char reltime[8];
    reltimestr(ot->created_at,reltime);
    mvwaddstr(tp,sl,colwidth-1-strlen(reltime),reltime);

    WINDOW* textpad = subpad(tp,textlines,colwidth-1,sl+2,1);

    mvwaddstr(textpad,0,0,tweettext);

    wmove(tp, lines-1,1);
	
    wattron(tp,graytype);

    if (tweet->retweet_count) {
	if (tweet->retweeted) wattron(tp, (selected ? CP_RTSEL : CP_RT));
	wprintw(tp,"%'" PRIu64 " RT  ",tweet->retweet_count);
	if (tweet->retweeted) wattroff(tp, (selected ? CP_RTSEL : CP_RT));
    }
    if (tweet->favorite_count) {
	if (tweet->favorited) wattron(tp, (selected ? CP_FAVSEL : CP_FAV));
	wprintw(tp,"%'" PRIu64 " FAV  ",tweet->favorite_count);
	if (tweet->favorited) wattroff(tp, (selected ? CP_FAVSEL : CP_FAV));
    }
	
    wattroff(tp,graytype);

    touchwin(tp);

    if (linecount != NULL) *linecount = lines;

    delwin(textpad);

    return tp;
}

