#include <string.h>
#include <ctype.h>
#include "conf.h"
#include CURSES_HEADER
#include "colors.h"
#include "consts.h"
#include "globals.h"
#include "ui.h"

/**
 * Centers a piece of text on a window on the specified row.
 */
void center(WINDOW *win, int width, int row, char *s)
{
    mvwaddstr(win, row, (width - strlen(s)) / 2, s);
}

/**
 * Sets up the color pairs
 */
void color_setup(void)
{
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_YELLOW, COLOR_BLUE);
    init_pair(9, COLOR_WHITE, COLOR_BLUE);
    init_pair(10, COLOR_BLACK, COLOR_YELLOW);
    init_pair(11, COLOR_BLACK, COLOR_WHITE);
    init_pair(12, COLOR_BLACK, COLOR_RED);
    init_pair(13, COLOR_BLACK, COLOR_BLUE);
    init_pair(14, COLOR_BLACK, COLOR_GREEN);
}

int origin_y, origin_x;         // Where the 80x24 screen starts
int in_game;                    // True once the map is on the screen

#define USED_LINES 24           // Rows the game draws on
#define MAX_WINS 8

static struct {
    WINDOW *win;
    WINDOW *copy;               // A pad: ncurses never cuts those down
    int lines, cols, y, x;
} wins[MAX_WINS];
static int nwins;

/**
 * Works out where the screen goes to sit centred in the terminal.
 */
void place_origin(void)
{
    origin_y = LINES > USED_LINES ? (LINES - USED_LINES) / 2 : 0;
    origin_x = COLS > SCREEN_COLUMNS ? (COLS - SCREEN_COLUMNS) / 2 : 0;
}

/**
 * newwin at screen coordinates, remembered so it can be moved on resize.
 */
WINDOW *sl_newwin(int lines, int cols, int y, int x)
{
    WINDOW *win = newwin(lines, cols, y + origin_y, x + origin_x);

    if (win != NULL && nwins < MAX_WINS) {
        wins[nwins].win = win;
        wins[nwins].copy = newpad(lines, cols);
        wins[nwins].lines = lines;
        wins[nwins].cols = cols;
        wins[nwins].y = y;
        wins[nwins].x = x;
        nwins++;
    }
    return win;
}

/**
 * delwin for a window made with sl_newwin.
 */
void sl_delwin(WINDOW *win)
{
    int i;

    for (i = 0; i < nwins; i++)
        if (wins[i].win == win) {
            if (wins[i].copy != NULL)
                delwin(wins[i].copy);
            nwins--;
            for (; i < nwins; i++)
                wins[i] = wins[i + 1];
            break;
        }
    delwin(win);
}

/**
 * The title bar along the top of the game screen.
 */
void draw_titlebar(void)
{
    attron(color ? (YELLOW_ON_BLUE | A_BOLD) : A_REVERSE);
    mvprintw(origin_y, origin_x, " StarLanes ");
    attroff(color ? (YELLOW_ON_BLUE | A_BOLD) : A_REVERSE);
    attron(color ? BLUE_ON_BLACK : A_NORMAL);
    printw
        ("=====================================================================");
    attroff(color ? BLUE_ON_BLACK : A_NORMAL);
}

/**
 * Moves everything to the middle of a resized terminal and draws it again.
 */
static void relayout(void)
{
    int i, h, w;

#ifdef PDCURSES
    resize_term(0, 0);          // PDCurses leaves this to the program
#endif
    place_origin();
    erase();
    if (in_game)
        draw_titlebar();
    wnoutrefresh(stdscr);
    for (i = 0; i < nwins; i++) {
        getmaxyx(wins[i].win, h, w);
        if (h != wins[i].lines || w != wins[i].cols) {
            // ncurses cut the window down to fit a smaller terminal:
            // grow it back and put back what it held
            wresize(wins[i].win, wins[i].lines, wins[i].cols);
            if (wins[i].copy != NULL)
                copywin(wins[i].copy, wins[i].win, 0, 0, 0, 0,
                        wins[i].lines - 1, wins[i].cols - 1, FALSE);
        }
        mvwin(wins[i].win, wins[i].y + origin_y, wins[i].x + origin_x);
    }
    for (i = 0; i < nwins; i++) {
        WINDOW *win = wins[i].win;
        if (!in_game && (win == mapwin || win == coinfo || win == general))
            continue;
        touchwin(win);
        wnoutrefresh(win);
    }
    doupdate();
}

/**
 * getch that follows terminal resizes instead of returning them.
 */
int sl_getch(void)
{
    int c, i;

    for (;;) {
        for (i = 0; i < nwins; i++)     // What to put back if it gets cut
            if (wins[i].copy != NULL)
                copywin(wins[i].win, wins[i].copy, 0, 0, 0, 0,
                        wins[i].lines - 1, wins[i].cols - 1, FALSE);
        if ((c = getch()) != KEY_RESIZE)
            return c;
        relayout();
    }
}

/**
 * Redraws the screen.
 */
void redraw(void)
{
    if (!in_game) {             // Only the title screen is up
        relayout();
        return;
    }
    touchwin(stdscr);
    touchwin(mapwin);
    touchwin(coinfo);
    touchwin(general);
    refresh();
    wrefresh(mapwin);
    wrefresh(coinfo);
    wrefresh(general);
}

/**
 * Clears and titles the general window.
 */
void clear_general(char *s, int blink)
{
    werase(general);
    wattron(general, GENERAL_BORDER);
    box(general, '|', '/');
    wattroff(general, GENERAL_BORDER);
    wattron(general, blink ? GENERAL_TITLE_BLINK : GENERAL_TITLE);
    center(general, SCREEN_COLUMNS - 2, 0, s);
    wattroff(general, blink ? GENERAL_TITLE_BLINK : GENERAL_TITLE);
}

/**
 * Does mywgetstr my way.
 */
int my_mvwgetstr(WINDOW *win, int y, int x, int max, int num_only, char *s)
{
    int cur = 0, c, done = 0;

    s[0] = '\0';

    nl();
    noecho();
    raw();
    keypad(stdscr, 0);
    while (doupdate(), c = sl_getch(), !done && (c != LF && c != CR)) {
        if (c == CTRL_L) {
            redraw();
            continue;
        }
        if (c == erasechar() || c == BS || c == DEL) {
            if (cur) {
                cur--;
                mvwaddch(win, y, x + cur, ' ');
                s[cur] = '\0';
                wmove(win, y, x + cur);
                wnoutrefresh(win);
            }
        } else {
            if (num_only == 1) {        // Only numbers and '-' allowed
                if (!isdigit(c) && c != '-' && tolower(c) != 'm'
                    && tolower(c) != 'n')
                    continue;
                if (tolower(c) == 'm' || tolower(c) == 'n') {
                    if (cur == 0)
                        done = 1;
                    else
                        continue;
                }
                if (c == '-' && cur != 0)
                    continue;
            }
            if (cur < max) {
                mvwaddch(win, y, x + cur, c);
                s[cur] = c;
                s[cur + 1] = '\0';
                cur++;
                wnoutrefresh(win);
            }
        }
    }

    nonl();
    keypad(stdscr, 1);
    return strlen(s);
}

