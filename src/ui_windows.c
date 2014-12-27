// vim: cin:sts=4:sw=4 
#include "ui_windows.h"
#include <string.h>
#include <wchar.h>
#include "utf8.h"

char* vimfile() {



}

size_t wintstrlen(wint_t* str) {
    size_t i=0;
    while (str[i] != 0) i++;
    return i;
}

int inputbox_utf8(const char* message, enum msgboxclass class, char* textfield, size_t maxchars, size_t maxbytes) {

    int msgwidth, msgheight, textw;

    utf8_text_size(message,&msgwidth,&msgheight);

    char* cutmsg = NULL;

    if ((msgwidth) > (COLS - 8)) {

	cutmsg = malloc(strlen(message) + 128);

	int r = utf8_wrap_text(message,cutmsg,strlen(message) + 128,(COLS-8));

	utf8_text_size(cutmsg,&msgwidth,&msgheight);

    }

    char* dispmsg = (cutmsg ? cutmsg : message);

    int textinpsize = maxchars+2;

    int maxwidth = msgwidth+4;

    int maxheight = msgheight+4;

    if (textinpsize >= msgwidth) maxwidth = textinpsize+4;

    int topline = (LINES-maxheight)/2;
    WINDOW* msgwindow = newwin(maxheight,maxwidth,topline, (COLS-maxwidth)/2);

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
    wrefresh(msgwindow);

    WINDOW* textfw = derwin(msgwindow,1,textinpsize,maxheight-2,((maxwidth-textinpsize)/2));

    mvwaddstr(textfw,0,0,"[");
    for (int i=0; i<maxchars; i++) waddch(textfw,' ');
    mvwaddstr(textfw,0,textinpsize-1,"]");

    wint_t widetext[maxchars+1]; 
    int32_t wtext32[maxchars+1];

    int ul=0;

    do {
   
    wmove(textfw,0,1);
    for (int i=0; i<maxchars; i++) waddch(textfw,' ');
    wmove(textfw,0,1);

    echo();
    wgetn_wstr(textfw,widetext,maxchars);
    noecho();

    for (int i=0; i<maxchars; i++) wtext32[i] = widetext[i]; //making sure.

    ul = utf8proc_reencode(wtext32,wintstrlen(wtext32),UTF8PROC_STRIPCC | UTF8PROC_COMPOSE);

    if (ul > maxbytes) { beep(); }

    } while (ul > maxbytes);

    strcpy(textfield,(char*)wtext32);

    delwin(textwin);
    delwin(msgwindow);

    wtouchln(colarea,(LINES-maxheight)/2-2,maxheight,1);
    wrefresh(colarea);

    if (cutmsg) free (cutmsg);

    redraw_lines(topline,maxheight);

    return 0;
 

}

int inputbox(const char* message, enum msgboxclass class, char* textfield, size_t textsize) {

    int msgwidth, msgheight, textw;

    utf8_text_size(message,&msgwidth,&msgheight);

    char* cutmsg = NULL;

    if ((msgwidth) > (COLS - 8)) {

	cutmsg = malloc(strlen(message) + 128);

	int r = utf8_wrap_text(message,cutmsg,strlen(message) + 128,(COLS-8));

	utf8_text_size(cutmsg,&msgwidth,&msgheight);

    }

    char* dispmsg = (cutmsg ? cutmsg : message);

    int textinpsize = textsize+2;

    int maxwidth = msgwidth+4;

    int maxheight = msgheight+4;

    if (textinpsize >= msgwidth) maxwidth = textinpsize+4;

    int topline = (LINES-maxheight)/2;
    WINDOW* msgwindow = newwin(maxheight,maxwidth,topline, (COLS-maxwidth)/2);

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
    wrefresh(msgwindow);

    WINDOW* textfw = derwin(msgwindow,1,textinpsize,maxheight-2,((maxwidth-textinpsize)/2));

    mvwaddstr(textfw,0,0,"[");
    mvwaddstr(textfw,0,textinpsize-1,"]");
    wmove(textfw,0,1);

    echo();
    wgetnstr(textfw,textfield,textsize);
    noecho();

    delwin(textwin);
    delwin(msgwindow);

    wtouchln(colarea,(LINES-maxheight)/2-2,maxheight,1);
    wrefresh(colarea);

    if (cutmsg) free (cutmsg);

    redraw_lines(topline,maxheight);

    return 0;
}

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

    if (btnwidth >= textwidth) maxwidth = btnwidth+4;

    int topline = (LINES-maxheight)/2;
    WINDOW* msgwindow = newwin(maxheight,maxwidth,topline, (COLS-maxwidth)/2);

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

	// int btnleft = (maxwidth - btnwidth) / 2; //align center
	int btnleft = (maxwidth - btnwidth); //align right

	int curbutleft = btnleft - 2;

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

    delwin(textwin);
    delwin(msgwindow);

    wtouchln(colarea,(LINES-maxheight)/2-2,maxheight,1);
    wrefresh(colarea);

    if (cutmsg) free (cutmsg);

    redraw_lines(topline,maxheight);

    return selbtn;
}


