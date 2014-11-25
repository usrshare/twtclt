#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

#include <curses.h>

enum msgboxclass {
    msg_info, //blue borders
    msg_warning, //yellow borders
    msg_error, //red borders
    msg_critical, //red borders, darken background?
};

WINDOW* titlebar; //top line
WINDOW* colhdrs; //second line

WINDOW* colarea; //everything else

WINDOW* statusbar; //bottom line
WINDOW* inputbar; //bottom right space

int inputbox(const char* message, enum msgboxclass class, char* textfield, size_t textsize);
int msgbox(char* message, enum msgboxclass class, int buttons_n, char** btntext);

#endif
