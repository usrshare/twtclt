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

enum optiontype {

    ot_static,
    ot_separator,

    ot_input_int,
    ot_input_uint64,
    ot_input_text,
    ot_input_utf8,

    ot_checkbox,

    ot_menu,
    ot_optionmenu,
};

struct option {
    uint64_t id;
    char* description;
    enum optiontype type;
    void* vptr; //pointer to appropriate item
    size_t size; //items for ot_optionmenu, max bytes for inputboxes
    size_t text_max_c; //for text fields
};

WINDOW* titlebar; //top line
WINDOW* colhdrs; //second line

WINDOW* colarea; //everything else

WINDOW* statusbar; //bottom line
WINDOW* inputbar; //bottom right space

uint64_t menu(const char* message, enum msgboxclass class, int choices_n, struct menuitem* choices);
uint64_t option_menu(const char* message, enum msgboxclass class, unsigned int items_n, struct option* options);
int inputbox(const char* message, enum msgboxclass class, char* textfield, size_t textsize);
int inputbox_utf8(const char* message, enum msgboxclass class, char* textfield, size_t maxchars, size_t maxbytes);
int msgbox(char* message, enum msgboxclass class, int buttons_n, char** btntext);

#endif
