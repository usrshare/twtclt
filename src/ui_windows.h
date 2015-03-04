// vim: cin:sts=4:sw=4 
#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

#include <curses.h>

enum msgboxclass {
    msg_info, //blue borders
    msg_warning, //yellow borders
    msg_error, //red borders
    msg_critical, //red borders, darken background?
};

struct menuitem {
    uint64_t id;
    char* name;
    char* desc;
    int disabled;
};

WINDOW* titlebar; //top line
WINDOW* colhdrs; //second line

WINDOW* colarea; //everything else

WINDOW* statusbar; //bottom line
WINDOW* inputbar; //bottom right space

uint64_t menu(const char* message, enum msgboxclass class, int choices_n, struct menuitem* choices);
int inputbox(const char* message, enum msgboxclass class, char* textfield, size_t textsize);
int inputbox_utf8(const char* message, enum msgboxclass class, char* textfield, size_t maxchars, size_t maxbytes);
int msgbox(char* message, enum msgboxclass class, int buttons_n, char** btntext);

#endif
