#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "conf.h"
#include CURSES_HEADER
#include "globals.h"
#include "colors.h"
#include "ui.h"
#include "quit.h"
#include "standings.h"

/**
 * Asks the users if they're sure they want to quit.
 */
void quit_yn(void)
{
    WINDOW *yn;

    if ((yn = sl_newwin(5, 42, 5, (SCREEN_COLUMNS - 42) / 2)) == NULL) {
        fprintf(stderr,
                "starlanes: couldn't open window for y/n prompt\n");
        exit(1);
    }

    wattron(yn, QUIT_BORDER);
    box(yn, '|', '=');
    wattroff(yn, QUIT_BORDER);
    wattron(yn, QUIT_TITLE);
    center(yn, 42, 0, " Really Quit? ");
    wattroff(yn, QUIT_TITLE);
    wattron(yn, QUIT_TEXT);
    mvwaddstr(yn, 2, 2, "Are you sure you want to quit (y/n)? ");
    wattroff(yn, QUIT_TEXT);
    wnoutrefresh(yn);
    doupdate();
    if (toupper(sl_getch()) == 'Y') {
        sl_delwin(yn);
        shutdown();
        exit(0);
    }
    sl_delwin(yn);
    touchwin(mapwin);
    touchwin(coinfo);
    wnoutrefresh(mapwin);
    wnoutrefresh(coinfo);
}

/**
 * Cleans up.
 */
void shutdown(void)
{
    show_standings(" GAME OVER - Final Standings ");
    sl_delwin(mapwin);
    sl_delwin(coinfo);
    sl_delwin(general);
    noraw();
    echo();
    clear();
    refresh();
    endwin();
    printf("Thank you for playing Star Lanes!\n");
}

