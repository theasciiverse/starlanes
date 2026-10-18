#include <stdio.h>
#include <stdlib.h>
#include "conf.h"
#include CURSES_HEADER
#include "colors.h"
#include "consts.h"
#include "globals.h"
#include "ui.h"

/**
 * For sorting for show_standings().
 */
int order_compare(const void *v1, const void *v2)
{
    int nw1, nw2, p1, p2;

    p1 = *((int *) v1);
    p2 = *((int *) v2);

    nw1 = pl[p1].svalue + pl[p1].cash;
    nw2 = pl[p2].svalue + pl[p2].cash;

    if (nw1 < nw2)
        return 1;
    if (nw1 > nw2)
        return -1;
    return 0;
}

/**
* Pops up a window with the current standings.
*/
void show_standings(char *title)
{
    WINDOW *stand;
    char s[80];
    int i, j, order[MAXPLAYERS], togo, sum;

    if ((stand =
         sl_newwin(9 + numplayers, 60, 5, (SCREEN_COLUMNS - 60) / 2)) == NULL) {
        fprintf(stderr, "starlanes: couldn't create standings window\n");
        exit(1);
    }

    wattron(stand, STAND_BORDER);
    box(stand, '|', '=');
    wattroff(stand, STAND_BORDER);

    wattron(stand, STAND_TITLE);
    sprintf(s, " %s ", title);
    center(stand, 60, 0, s);
    wattroff(stand, STAND_TITLE);

    wattron(stand, STAND_HEADER);
    mvwaddstr(stand, 2, 2,
              "=Player================Stock Value===Cash=====Net Worth=");
    wattroff(stand, STAND_HEADER);

    for (i = 0; i < numplayers; i++) {
        order[i] = i;
        pl[i].svalue = 0;       // Calculate total player stock value
        for (j = 0; j < NUMCO; j++)
            if (co[j].size)
                pl[i].svalue += co[j].price * pl[i].holdings[j];
    }

    qsort(order, numplayers, sizeof(int), order_compare);       // Sort

    for (i = 0; i < numplayers; i++) {
        wattron(stand, A_BOLD);
        mvwprintw(stand, 4 + i, 3, "%-19s   ", pl[order[i]].name);
        wattroff(stand, A_BOLD);

        wprintw(stand, "$%-8d   $%-8d  ", pl[order[i]].svalue,
                pl[order[i]].cash);

        wattron(stand, A_BOLD);
        wprintw(stand, "$%-8d", pl[order[i]].cash + pl[order[i]].svalue);
        wattroff(stand, A_BOLD);
    }

    for (i = sum = 0; i < NUMCO; i++)   // Get company size
        sum += co[i].size;

    togo = (MAPX * MAPY * END_PERCENT / 100) - sum + 1;

    sprintf(s, "Available company sectors remaining: %d", togo);
    center(stand, 60, 5 + numplayers, s);
    center(stand, 60, 7 + numplayers, "Press any key to continue...");
    wnoutrefresh(stand);
    doupdate();
    sl_getch();
    sl_delwin(stand);
    touchwin(mapwin);
    touchwin(general);
    touchwin(coinfo);
    wnoutrefresh(mapwin);
    wnoutrefresh(general);
    wnoutrefresh(coinfo);
}

