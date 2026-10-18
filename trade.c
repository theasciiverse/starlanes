#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "conf.h"
#include CURSES_HEADER
#include "colors.h"
#include "company.h"
#include "consts.h"
#include "globals.h"
#include "quit.h"
#include "standings.h"
#include "ui.h"

/**
 * Screen that allows the user to buy and sell stocks.
 */
void buy_sell(void)
{
    int i, cos[NUMCO], cocount = 0, cursor = 0, newcur, done =
        0, amt, max, min, pos1, pos2;
    int attrs;
    char s[160], amtstr[15];

    for (i = 0; i < NUMCO; i++) // Make a list of active companies
        if (co[i].size)
            cos[cocount++] = i;

    if (cocount == 0)
        return;                 // No companies yet

    sprintf(s, " %s (Cash: $%d) ", pl[turn].name, pl[turn].cash);
    clear_general(s, 0);

    center(general, SCREEN_COLUMNS - 2, 1,
           "Arrow keys to select a company, return to trade, escape when done:");

    for (i = 0; i < cocount; i++) {
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
        if (i == cursor)
            wattron(general, A_REVERSE);
        else
            wattron(general, attrs);
        mvwprintw(general, i + 3, 20, "%s", co[cos[i]].name);
        if (i == cursor)
            wattroff(general, A_REVERSE);
        else
            wattroff(general, attrs);
    }
    wnoutrefresh(general);

    do {
        newcur = cursor;
        raw();
        noecho();
        doupdate();
        switch (sl_getch()) {
        case CTRL_L:
            redraw();
            break;
        case 's':
        case 'S':
            show_standings(" Current Standings ");
            wmove(general, cursor + 3, strlen(co[cos[cursor]].name) + 20);
            wnoutrefresh(general);
            break;
        case 'c':
        case 'C':
            more_coinfo();
            wmove(general, cursor + 3, strlen(co[cos[cursor]].name) + 20);
            wnoutrefresh(general);
            break;
        case 'q':
        case 'Q':
        case CTRL_C:
            quit_yn();
            wmove(general, cursor + 3, strlen(co[cos[cursor]].name) + 20);
            wnoutrefresh(general);
            break;
        case KEY_UP:
        case '8':
        case 'k':
        case 'K':
            newcur = cursor ? cursor - 1 : cocount - 1;
            break;
        case KEY_DOWN:
        case '5':
        case 'j':
        case 'J':
            newcur = cursor < cocount - 1 ? cursor + 1 : 0;
            break;
        case KEY_RIGHT:
        case 6:
        case CR:
        case LF:
            max = pl[turn].cash / co[cos[cursor]].price;
            min = -pl[turn].holdings[cos[cursor]];
            sprintf(s, "Amount (%d to %d): ", min, max);
            mvwprintw(general, cursor + 3, 40, "%s", s);
            wnoutrefresh(general);
            my_mvwgetstr(general, cursor + 3, 40 + strlen(s), 9, 1,
                         amtstr);
            if (tolower(amtstr[0]) == 'm')
                amt = max;
            else if (tolower(amtstr[0]) == 'n')
                amt = min;
            else
                amt = atoi(amtstr);
            if (amt >= min && amt <= max) {
                pl[turn].cash += (-amt * co[cos[cursor]].price);
                pl[turn].holdings[cos[cursor]] += amt;
            } else {
                mvwprintw(general, cursor + 3, 40,
                          "Invalid amount!                      ");
                wmove(general, cursor + 3, 55);
                wnoutrefresh(general);
                doupdate();
                sleep(1);
            }
            mvwprintw(general, cursor + 3, 40,
                      "                                     ");
            sprintf(s, " %s (Cash: $%d) ", pl[turn].name, pl[turn].cash);

            if (amt)
                show_coinfo();

            pos1 = ((SCREEN_COLUMNS - 2) - strlen(s)) / 2;
            pos2 = pos1 + strlen(s);
            wattron(general, GENERAL_TITLE);
            center(general, SCREEN_COLUMNS - 2, 0, s);
            wattroff(general, GENERAL_TITLE);
            wattron(general, GENERAL_BORDER);
            mvwaddstr(general, 0, pos1 - 4, "////");
            mvwaddstr(general, 0, pos2, "////");
            wattroff(general, GENERAL_BORDER);
            wmove(general, cursor + 3, strlen(co[cos[cursor]].name) + 20);
            wnoutrefresh(general);

            break;

        case ESC:
            done = 1;
        }                       // Switch

        if (cursor != newcur) { // Move cursor
            switch (cos[cursor]) {
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

            wattron(general, attrs);
            mvwprintw(general, cursor + 3, 20, "%s", co[cos[cursor]].name);
            wattroff(general, attrs);

            wattron(general, color ? BLACK_ON_WHITE : A_REVERSE);
            mvwprintw(general, newcur + 3, 20, "%s", co[cos[newcur]].name);
            wattroff(general, color ? BLACK_ON_WHITE : A_REVERSE);

            wnoutrefresh(general);
            cursor = newcur;
        }
    } while (!done);
}


