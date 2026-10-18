#include <stdio.h>
#include <stdlib.h>
#include "conf.h"
#include CURSES_HEADER
#include "colors.h"
#include "company.h"
#include "consts.h"
#include "globals.h"
#include "ui.h"

/**
 * Draws the company info screen
 */
void show_coinfo(void)
{
    int i, gotco = 0, attrs;

    werase(coinfo);
    wattron(coinfo, COINFO_TITLE);
    mvwprintw(coinfo, 0, 0, " %-19s    %5s  %8s ", "Company", "Price",
              "Holdings");
    wattroff(coinfo, COINFO_TITLE);

    wmove(coinfo, 2, 0);
    for (i = 0; i < NUMCO; i++)
        if (co[i].size) {
            switch (i) {
            case 0:
                attrs = CO_A;
                break;
            case 1:
                attrs = CO_B;
                break;
            case 2:
                attrs = CO_C;
                break;
            case 3:
                attrs = CO_D;
                break;
            case 4:
                attrs = CO_E;
                break;
            }
            wattron(coinfo, attrs);
            wprintw(coinfo, " %-19s    ", co[i].name);
            wattroff(coinfo, attrs);
            wprintw(coinfo, "$%-4d   ", co[i].price);
            //if (pl[turn].holdings[i] == 0) wattron(coinfo,A_BLINK);
            wprintw(coinfo, "%-5d\n\n\r", pl[turn].holdings[i]);
            //if (pl[turn].holdings[i] == 0) wattroff(coinfo,A_BLINK);
            gotco = 1;
        }

    if (!gotco) {
        wattron(coinfo, COINFO_TEXT);
        wprintw(coinfo, " no active companies");
        wattroff(coinfo, COINFO_TEXT);
    }

    wnoutrefresh(coinfo);
}

/**
 * Shows detailed company information.
 */
void more_coinfo(void)
{
    WINDOW *more_coinfo;
    int numco = 0, cos[NUMCO], i, j, attrs, total;

    for (i = 0; i < NUMCO; i++) {       // Get list of active companies
        if (co[i].size)
            cos[numco++] = i;
    }

    if (numco == 0)
        return;                 // Don't bother if there are no active co's

    if ((more_coinfo =
         sl_newwin(7 + numco, 52, 5, (SCREEN_COLUMNS - 52) / 2)) == NULL) {
        fprintf(stderr, "starlanes: couldn't create more_coinfo window\n");
        exit(1);
    }

    wattron(more_coinfo, MORE_COINFO_BORDER);
    box(more_coinfo, '|', '=');
    wattroff(more_coinfo, MORE_COINFO_BORDER);

    wattron(more_coinfo, MORE_COINFO_TITLE);
    center(more_coinfo, 52, 0, " Detailed Company Information ");
    wattroff(more_coinfo, MORE_COINFO_TITLE);

    wattron(more_coinfo, MORE_COINFO_HEADER);
    mvwaddstr(more_coinfo, 2, 1,
              " Company name          Price   Size   Total Worth ");
    wattroff(more_coinfo, MORE_COINFO_HEADER);


    for (i = 0; i < numco; i++) {
        switch (cos[i]) {
        case 0:
            attrs = CO_A;
            break;
        case 1:
            attrs = CO_B;
            break;
        case 2:
            attrs = CO_C;
            break;
        case 3:
            attrs = CO_D;
            break;
        case 4:
            attrs = CO_E;
            break;
        }

        wattron(more_coinfo, attrs);
        mvwprintw(more_coinfo, i + 4, 2, "%-20s  ", co[cos[i]].name);
        wattroff(more_coinfo, attrs);
        wprintw(more_coinfo, "$%-4d    ", co[cos[i]].price);
        wprintw(more_coinfo, "%-3d    ", co[cos[i]].size);

        for (j = total = 0; j < numplayers; j++)
            total += co[cos[i]].price * pl[j].holdings[cos[i]];

        wattron(more_coinfo, A_BOLD);
        wprintw(more_coinfo, "$%-9d", total);
        wattroff(more_coinfo, A_BOLD);
    }

    center(more_coinfo, 52, 5 + numco, "Press any key to continue...");
    wnoutrefresh(more_coinfo);
    doupdate();
    sl_getch();
    sl_delwin(more_coinfo);
    touchwin(mapwin);
    touchwin(general);
    touchwin(coinfo);
    wnoutrefresh(mapwin);
    wnoutrefresh(general);
    wnoutrefresh(coinfo);
}

/**
 * Returns the index of the next available company, or -1 if all
 * companies exist.
 */
int co_avail(void)
{
    int i;
    for (i = 0; i < NUMCO; i++)
        if (!co[i].size)
            return i;
    return -1;
}

