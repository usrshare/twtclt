// vim: cin:sts=4:sw=4 
#include <string.h>
#include <wchar.h>

#include <curses.h>
#include <menu.h>

#include "ui.h"
#include "ui_windows.h"
#include "utf8.h"

enum c_alignment {
    C_ALIGN_LEFT,
    C_ALIGN_CENTER,
    C_ALIGN_RIGHT,
};

char* vimfile() {


return NULL;
}

size_t wintstrlen(int32_t* str) {
    size_t i=0;
    while (str[i] != 0) i++;
    return i;
}

int msg_window(const char* message, enum msgboxclass class, int* height, int* width, WINDOW** msgwin, WINDOW** textwin, WINDOW** ctnwin, enum c_alignment align) {

    // this is a helper function designed to simplify the rest of "message window" functions. It creates a window with text on top and a content window with appropriate dimensions.

    int textwidth, textheight;

    utf8_text_size(message,&textwidth,&textheight);

    char* cutmsg = NULL;

    if ((textwidth) > (COLS - 8)) {

	cutmsg = malloc(strlen(message) + 128); //should be enough for wrap

	int r = utf8_wrap_text(message,cutmsg,strlen(message) + 128,(COLS-8));

	if (r == -1) return -1;

	utf8_text_size(cutmsg,&textwidth,&textheight);

    }

    const char* dispmsg = (cutmsg ? cutmsg : message);

    int msgwidth = textwidth + 2;
    int ctnwidth = ((*width < COLS-8) ? *width : COLS-8);

    int clientwidth = ((ctnwidth > textwidth) ? ctnwidth : textwidth);
    int maxwidth = clientwidth + 4;

    *width = ctnwidth;

    int msgheight = textheight + 1;
    int maxctnheight = (LINES-8) - 2 - msgheight - 1;

    int clientheight = (*height < maxctnheight ? *height : maxctnheight);
    int ctnheight = clientheight + 1;
    
    *height = ctnheight;

    int maxheight = msgheight + ctnheight;
    int topline = (LINES-maxheight)/2;
    *msgwin = newwin(maxheight,maxwidth,topline, (COLS-maxwidth)/2);

    switch (class) {
	case msg_info:
	    wattron(*msgwin,COLOR_PAIR(10));
	    box(*msgwin,0,0);
	    wattroff(*msgwin,COLOR_PAIR(10));
	    break;
	case msg_warning:
	    wattron(*msgwin,COLOR_PAIR(11));
	    box(*msgwin,0,0);
	    wattroff(*msgwin,COLOR_PAIR(11));
	    break;
	case msg_error:
	    wattron(*msgwin,COLOR_PAIR(12));
	    box(*msgwin,0,0);
	    wattroff(*msgwin,COLOR_PAIR(12));
	    break;
	case msg_critical:
	    wattron(*msgwin,COLOR_PAIR(13));
	    box(*msgwin,0,0);
	    wattroff(*msgwin,COLOR_PAIR(13));
	    break;
    }

    *textwin = derwin(*msgwin,msgheight,msgwidth-2,1,2);
    waddstr(*textwin,dispmsg);

    int cleft = 0;
    switch(align) {
	case C_ALIGN_LEFT:
	    cleft = 2; break;
	case C_ALIGN_CENTER:
	    cleft = ((maxwidth - ctnwidth)/2); break;
	case C_ALIGN_RIGHT:
	    cleft = ((maxwidth - ctnwidth) - 2); break;
    }

    *ctnwin = derwin(*msgwin, ctnheight, ctnwidth, msgheight, cleft ); 

    return 0;
}

int menu(const char* message, enum msgboxclass class, int choices_n, char** choice_id, char** choice_desc) {

    ITEM **menuitems;
    int c;
    MENU *thismenu;
    int i;
    ITEM *cur_item;

    //TODO show and handle buttons.

    int maxheight = (choices_n);

    int maxwidth = 0;
    
    for (int i=0; i < choices_n; i++) {
	int itemwidth = 1 + utf8_count_chars(choice_id[i]) + (choice_desc ? (1 + utf8_count_chars(choice_desc[i])) : 0);
	if (itemwidth > maxwidth) maxwidth = itemwidth;
    }	

    WINDOW *msgwin, *textwin, *cntwin;

    msg_window(message, class, &maxheight, &maxwidth, &msgwin, &textwin, &cntwin, C_ALIGN_RIGHT);

    touchwin(msgwin);

    MENU *item_menu;
    ITEM **m_items;

    m_items = malloc(sizeof(ITEM*) * (choices_n + 1));

    for (int i=0; i < choices_n; i++) {
	m_items[i] = new_item(choice_id[i],(choice_desc ? choice_desc[i] : NULL));
    }

    m_items[choices_n] = NULL;

    item_menu = new_menu(m_items);

    set_menu_win(item_menu,msgwin);
    set_menu_sub(item_menu,cntwin);

    post_menu(item_menu);
    wrefresh(msgwin);

    int selectloop = 1;

    keypad(msgwin,1);

    while (selectloop) {

	wrefresh(msgwin);

	wmove(msgwin,0,0);
	int k = wgetch(msgwin); 

	switch(k) {
	    case 'j':
	    case '\t':
	    case KEY_DOWN:
		menu_driver(item_menu,REQ_DOWN_ITEM);
		break;
	    case 'k':
	    case KEY_UP:
		menu_driver(item_menu,REQ_UP_ITEM);
		break;
	    case KEY_NPAGE:
		menu_driver(item_menu,REQ_SCR_DPAGE);
		break;
	    case KEY_PPAGE:
		menu_driver(item_menu,REQ_SCR_UPAGE);
		break;
	    case 32:
	    case KEY_ENTER:
	    case '\r':
	    case '\n':
		selectloop=0;
		break;
	}
	wrefresh(msgwin);
    }

    ITEM* ci = current_item(item_menu);
    int ii = item_index(ci);

    unpost_menu(item_menu);
    free_menu(item_menu);

    for (int i=0; i<choices_n; i++) {
	free_item(m_items[i]);
    }

    free(m_items);

    delwin(cntwin);
    delwin(textwin);
    delwin(msgwin);

    touchwin(colarea);
    wrefresh(colarea);

    draw_all_columns();

    return ii;


}

int inputbox_utf8(const char* message, enum msgboxclass class, char* textfield, size_t maxchars, size_t maxbytes) {

    int textinpsize = maxchars+2;
    int h = 1;
    
    WINDOW *msgwin, *textwin, *textfw;

    msg_window(message, class, &h, &textinpsize, &msgwin, &textwin, &textfw, C_ALIGN_RIGHT);

    touchwin(msgwin);
    wrefresh(msgwin);

    mvwaddstr(textfw,0,0,"[");
    for (size_t i=0; i<maxchars; i++) waddch(textfw,' ');
    mvwaddstr(textfw,0,textinpsize-1,"]");

    wint_t widetext[maxchars+1]; 
    int32_t wtext32[maxchars+1];

    int ul=0;

    do {
   
    wmove(textfw,0,1);
    for (size_t i=0; i<maxchars; i++) waddch(textfw,' ');
    wmove(textfw,0,1);

    echo();
    wgetn_wstr(textfw,widetext,maxchars);
    noecho();

    for (size_t i=0; i<maxchars; i++) wtext32[i] = widetext[i]; //making sure.

    ul = utf8proc_reencode(wtext32,wintstrlen(wtext32),UTF8PROC_STRIPCC | UTF8PROC_COMPOSE);

    if (ul > maxbytes) { beep(); }

    } while (ul > maxbytes);

    strcpy(textfield,(char*)wtext32);

    delwin(textfw);
    delwin(textwin);
    delwin(msgwin);

    touchwin(colarea);
    wrefresh(colarea);

    draw_all_columns();
    
    return 0;
}

int inputbox(const char* message, enum msgboxclass class, char* textfield, size_t textsize) {

    int textinpsize = textsize+2;
    int h = 1;
    
    WINDOW *msgwin, *textwin, *cntwin;

    msg_window(message, class, &h, &textinpsize, &msgwin, &textwin, &cntwin, C_ALIGN_RIGHT);

    touchwin(msgwin);
    wrefresh(msgwin);

    mvwaddstr(cntwin,0,0,"[");
    mvwaddstr(cntwin,0,textinpsize-1,"]");
    wmove(cntwin,0,1);

    echo();
    wgetnstr(cntwin,textfield,textsize);
    noecho();

    delwin(cntwin);
    delwin(textwin);
    delwin(msgwin);

    touchwin(colarea);
    wrefresh(colarea);

    draw_all_columns();


    return 0;
}

int msgbox(char* message, enum msgboxclass class, int buttons_n, char** btntext) {

    int maxbtnwidth = (buttons_n - 1);

    if (buttons_n > 0)
	for (int i=0; i<buttons_n; i++) maxbtnwidth += 1 + strlen(btntext[i]) + 1; //[button]

    int h=1;

    int mh=0, mw=0;

    WINDOW *msgwin, *textwin, *cntwin;

    msg_window(message, class, &h, &(maxbtnwidth), &msgwin, &textwin, &cntwin, C_ALIGN_RIGHT);

    touchwin(msgwin);

    WINDOW* buttons[buttons_n]; //C99 VLAs FTW!

    if (buttons_n != 0) {

	int btnwidth = (buttons_n - 1); //1 space between buttons

	for (int i=0; i<buttons_n; i++) {
	    btnwidth += 1 + strlen(btntext[i]) + 1; //[button]
	}

	int curbutleft = 0;

	for (int i=0; i<buttons_n; i++) {

	    buttons[i] = derwin(cntwin,1,strlen(btntext[i])+2, 0, curbutleft);
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
	    touchwin(msgwin);
	}

	wrefresh(msgwin);

	wmove(inputbar,0,0);
	int k = wgetch(inputbar); 

	switch(k) {
	    case 'h':
	    case KEY_LEFT:
		selbtn = (selbtn -1) % buttons_n; if (selbtn < 0) selbtn = (buttons_n -1);
		break;
	    case 'l':
	    case '\t':
	    case KEY_RIGHT:
		selbtn = (selbtn +1) % buttons_n;
		break;
	    case 32:
	    case KEY_ENTER:
	    case '\r':
	    case '\n':
		selectloop=0;
		break;
	}

    }

    for (int i=0; i<buttons_n; i++) {

	delwin(buttons[i]);

    }

    delwin(cntwin);
    delwin(textwin);
    delwin(msgwin);

    touchwin(colarea);
    wrefresh(colarea);

    draw_all_columns();

    return selbtn;
}


