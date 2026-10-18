#include "conf.h"
#include <stdio.h>
#include CURSES_HEADER
#include "colors.h"
#include "company.h"
#include "consts.h"
#include "globals.h"
#include "map.h"
#include "ui.h"

/**
 * Announce the coming of a new company
 */
void new_co_announce(int newc)
{
    char s[160];

    clear_general(" Special Announcement! ", 1);
    wattron(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 2,
           "A new shipping company has been formed!");
    sprintf(s, "Its name is %s", co[newc].name);
    center(general, SCREEN_COLUMNS - 2, 4, s);
    wattroff(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 7, "Press any key to continue...");
    wnoutrefresh(general);
    noecho();
    raw();
    doupdate();
    while (sl_getch() == CTRL_L)
        redraw();
}

/**
 * When a company gets drawn into a black hole (value < 0)
 */
void suck_announce(int conum, int grown)
{
    int i;

    if (conum >= 0) {
        for (i = 0; i < MAPX * MAPY; i++) {     // Clear the company from the map
            if (map[i] == conum + 'A') {
                map[i] = SPACE;
                drawmap(i, SPACE);
            }
        }
        wnoutrefresh(mapwin);

        for (i = 0; i < numplayers; i++)        // Ditch all player holdings
            pl[i].holdings[conum] = 0;

        co[conum].size = co[conum].price = 0;   // Eliminate the company
    } else {
        map[grown] = SPACE;     // Grown contains the opposite of the site
        drawmap(grown, SPACE);
        wnoutrefresh(mapwin);
    }

    clear_general(" Special Announcement! ", 1);
    wattron(general, A_BOLD);
    if (conum >= 0 && grown == 1) {     // Already existed
        center(general, SCREEN_COLUMNS - 2, 2, "The company named");
        center(general, SCREEN_COLUMNS - 2, 3, co[conum].name);
        center(general, SCREEN_COLUMNS - 2, 4,
               "has been sucked into a black hole!");
        center(general, SCREEN_COLUMNS - 2, 6, "All players' holdings lost.");
        show_coinfo();          // Show change
    } else if (conum >= 0 && grown == 0) {      // Was trying to start up
        center(general, SCREEN_COLUMNS - 2, 2,
               "The company that would have been named");
        center(general, SCREEN_COLUMNS - 2, 3, co[conum].name);
        center(general, SCREEN_COLUMNS - 2, 4,
               "has been sucked into a black hole!");
    } else {                    // Was only a starter company, not a real one yet
        center(general, SCREEN_COLUMNS - 2, 2,
               "The new company site just placed");
        center(general, SCREEN_COLUMNS - 2, 3,
               "has been sucked into a black hole!");
    }
    wattroff(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 8, "Press any key to continue...");
    wnoutrefresh(general);
    noecho();
    raw();
    doupdate();
    while (sl_getch() == CTRL_L)
        redraw();
}

/**
 * Announce a merger.
 */
void merge_announce(int c1, int c2)
{
    clear_general(" Special Announcement! ", 1);
    wattron(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 2, co[c2].name);
    wattroff(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 3, "has just been merged into");
    wattron(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 4, co[c1].name);
    wattroff(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 7, "Press any key to continue...");
    wnoutrefresh(general);
    noecho();
    raw();
    doupdate();
    while (sl_getch() == CTRL_L)
        redraw();
}

/**
 * Announce transactions after a merger.
 */
void xaction_announce(int c1, int c2)
{
    int i, totalshares = 0, newshares, bonus, totalholdings;

    clear_general(" Stock Transactions ", 0);
    center(general, SCREEN_COLUMNS - 2, 2, co[c2].name);
    wattron(general, color ? YELLOW_ON_BLUE | A_BOLD : A_REVERSE);
    mvwprintw(general, 2, 4,
              "=Player===============Old Stock===New Stock===Total Holdings===Bonus=");
    wattroff(general, color ? YELLOW_ON_BLUE | A_BOLD : A_REVERSE);

    for (i = 0; i < numplayers; i++)
        totalshares += pl[i].holdings[c2];
    if (totalshares == 0)
        totalshares = 1;        // Prevent divide by zero

    for (i = 0; i < numplayers; i++) {
        newshares = (int) (((float) (pl[i].holdings[c2]) / 2.0) + 0.5);
        totalholdings = pl[i].holdings[c1] + newshares;
        bonus = 10 * co[c2].price * pl[i].holdings[c2] / totalshares;
        wattron(general, A_BOLD);
        mvwprintw(general, 3 + i, 4, " %-20s   ", pl[i].name);
        wattroff(general, A_BOLD);
        wprintw(general, "%-5d       %-5d     %5d            ",
                pl[i].holdings[c2], newshares, totalholdings);
        wattron(general, A_BOLD);
        wprintw(general, "$%-5d", bonus);
        wattroff(general, A_BOLD);

        pl[i].holdings[c1] = totalholdings;
        pl[i].holdings[c2] = 0;
        pl[i].cash += bonus;
    }

    center(general, SCREEN_COLUMNS - 2, 8, "Press any key to continue...");
    wnoutrefresh(general);
    noecho();
    raw();
    doupdate();
    while (sl_getch() == CTRL_L)
        redraw();
}

/**
 * Happens when a company splits 2 for 1.
 */
void split_announce(int conum)
{
    char s[160];
    int i;

    clear_general(" Special Announcement! ", 1);
    wattron(general, A_BOLD);
    sprintf(s, "The stock of %s", co[conum].name);
    center(general, SCREEN_COLUMNS - 2, 2, s);
    center(general, SCREEN_COLUMNS - 2, 3, "has split two-for-one!");
    wattroff(general, A_BOLD);
    center(general, SCREEN_COLUMNS - 2, 6, "Press any key to continue...");

    co[conum].price = (int) (((float) (co[conum].price) / 2.0) + 0.5);
    for (i = 0; i < numplayers; i++)
        pl[i].holdings[conum] *= 2;

    show_coinfo();
    wnoutrefresh(general);
    noecho();
    raw();
    doupdate();
    while (sl_getch() == CTRL_L)
        redraw();
}

