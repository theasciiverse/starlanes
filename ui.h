#ifndef UI_H
#define UI_H

#include "conf.h"
#include CURSES_HEADER

void color_setup(void);
void center(WINDOW * win, int width, int row, char *s);
void redraw(void);
void clear_general(char *s, int blink);
int my_mvwgetstr(WINDOW * win, int y, int x, int max, int num_only,
                 char *s);

// The 80x24 screen is drawn centred in the terminal: windows are made
// with sl_newwin at screen coordinates and moved when it is resized.
extern int origin_y, origin_x, in_game;
void place_origin(void);
WINDOW *sl_newwin(int lines, int cols, int y, int x);
void sl_delwin(WINDOW * win);
void draw_titlebar(void);
int sl_getch(void);
#endif
