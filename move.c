#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "conf.h"
#include CURSES_HEADER
#include "announce.h"
#include "company.h"
#include "consts.h"
#include "globals.h"
#include "map.h"
#include "merge.h"
#include "move.h"
#include "quit.h"
#include "standings.h"
#include "ui.h"

/**
 * Gets a players move
 */
int get_move(void)
{
    char s[160], c;
    int move[NUMMOVES], i, j, ok;

    wattron(mapwin, A_REVERSE);
    for (i = 0; i < NUMMOVES; i++) {
        do {
            ok = 1;
            move[i] = rand() % (MAPX * MAPY);
            if (map[move[i]] != SPACE)
                ok = 0;
            for (j = 0; j < i; j++)
                if (move[j] == move[i])
                    ok = 0;
            if (co_avail() == -1) {     // No more companies
                if ((ripe(up_obj(move[i])) || ripe(down_obj(move[i])) ||
                     ripe(left_obj(move[i])) || ripe(right_obj(move[i])))
                    && !co_near(move[i]))
                    ok = 0;
            }
        } while (!ok);
        mvwaddch(mapwin, move[i] / MAPX + 1, (move[i] % MAPX) * 3 + 2,
                 i + 1 + '0');
    }
    wattroff(mapwin, A_REVERSE);
    wnoutrefresh(mapwin);

    sprintf(s, " %s (Cash: $%d) ", pl[turn].name, pl[turn].cash);
    clear_general(s, 0);
    sprintf(s, " %s, enter your move [1-%d]: ", pl[turn].name, NUMMOVES);
    mvwprintw(general, 2, 1, "%s", s);
    show_coinfo();
    noecho();
    do {
        wmove(general, 2, 1 + strlen(s));
        wnoutrefresh(general);
        doupdate();
        c = sl_getch();
        if (c == CTRL_L) {
            redraw();
            continue;
        }
        if (c == CTRL_C || toupper(c) == 'Q') {
            quit_yn();
            continue;
        }
        if (toupper(c) == 'S') {
            show_standings(" Current Standings ");
            continue;
        }
        if (toupper(c) == 'C') {
            more_coinfo();
            continue;
        }

        c -= '0';
    } while (c < 1 || c > NUMMOVES);
    echo();

    for (i = 0; i < NUMMOVES; i++)
        mvwaddch(mapwin, move[i] / MAPX + 1, (move[i] % MAPX) * 3 + 2,
                 SPACE);
    wnoutrefresh(mapwin);

    return move[c - 1];
}

/**
 * Make the move, and follow up on the consequences if it does.
 */
void do_move(int move)
{
    int north, south, west, east, newc;
    char newc_type;

    north = up_obj(move);
    south = down_obj(move);
    west = left_obj(move);
    east = right_obj(move);

    if (s_or_bh(north) && s_or_bh(south) && s_or_bh(east) && s_or_bh(west)) {
        if (north == BLACKHOLE || south == BLACKHOLE || west == BLACKHOLE
            || east == BLACKHOLE) {
            suck_announce(-1, move);
            return;
        } else {
            map[move] = NEWCO;  // No one around
            drawmap(move, NEWCO);
            wnoutrefresh(mapwin);
        }
    } else {                    // Check for merge
        if (iscompany(north) && iscompany(south) && north != south) {
            do_merge(&north, &south, &west, &east);
        }

        if (iscompany(north) && iscompany(east) && north != east) {
            do_merge(&north, &east, &south, &west);
        }

        if (iscompany(north) && iscompany(west) && north != west) {
            do_merge(&north, &west, &south, &east);
        }

        if (iscompany(west) && iscompany(east) && west != east) {
            do_merge(&west, &east, &north, &south);
        }

        if (iscompany(east) && iscompany(south) && east != south) {
            do_merge(&east, &south, &west, &north);
        }

        if (iscompany(west) && iscompany(south) && west != south) {
            do_merge(&west, &south, &east, &north);
        }
    }

    if ((ripe(east) || ripe(west) || ripe(north) || ripe(south))
        && !co_near(move)) {
        if ((newc = co_avail()) == -1) {
            map[move] = NEWCO;
            drawmap(move, NEWCO);
            wnoutrefresh(mapwin);
        } else {                // New company!
            map[move] = newc + 'A';
            drawmap(move, newc + 'A');
            co[newc].price = INIT_CO_COST;
            co[newc].size = 1;
            calc_cost(newc, move, north, south, west, east);
            if (co[newc].price <= 0) {  // Too close to black hole
                suck_announce(newc, 0);
            } else {            // Came to life just fine
                pl[turn].holdings[newc] = FOUNDER_BONUS;
                show_coinfo();
                wnoutrefresh(mapwin);
                new_co_announce(newc);
            }
        }
    } else if (map[move] == SPACE) {    // Adding on
        if (iscompany(west))
            newc_type = west;
        if (iscompany(east))
            newc_type = east;
        if (iscompany(south))
            newc_type = south;
        if (iscompany(north))
            newc_type = north;
        map[move] = newc_type;
        drawmap(move, newc_type);
        wnoutrefresh(mapwin);
        co[newc_type - 'A'].size++;
        co[newc_type - 'A'].price += NEWCOCOST;
        calc_cost(newc_type - 'A', move, north, south, west, east);
        if (co[newc_type - 'A'].price <= 0)     // Black holed
            suck_announce(newc_type - 'A', 1);
        else {
            if (co[newc_type - 'A'].price > SPLIT_PRICE)
                split_announce(newc_type - 'A');
            else
                show_coinfo();
        }
    }
}

/**
 * Adds value to a company based on surroundings and converts NEWCOs to
 * the company
 */
void calc_cost(int cnum, int move, int n, int s, int w, int e)
{
    if (n == STAR)
        co[cnum].price += STARCOST;     // Stars
    if (s == STAR)
        co[cnum].price += STARCOST;
    if (w == STAR)
        co[cnum].price += STARCOST;
    if (e == STAR)
        co[cnum].price += STARCOST;

    if (n == BLACKHOLE)
        co[cnum].price += BLACKHOLECOST;        // Black holes
    if (s == BLACKHOLE)
        co[cnum].price += BLACKHOLECOST;
    if (w == BLACKHOLE)
        co[cnum].price += BLACKHOLECOST;
    if (e == BLACKHOLE)
        co[cnum].price += BLACKHOLECOST;

    if (n == NEWCO) {           // Starter companies
        map[move - MAPX] = cnum + 'A';
        drawmap(move - MAPX, cnum + 'A');
        co[cnum].size++;
        co[cnum].price += NEWCOCOST;
    }
    if (s == NEWCO) {
        map[move + MAPX] = cnum + 'A';
        drawmap(move + MAPX, cnum + 'A');
        co[cnum].size++;
        co[cnum].price += NEWCOCOST;
    }
    if (w == NEWCO) {
        map[move - 1] = cnum + 'A';
        drawmap(move - 1, cnum + 'A');
        co[cnum].size++;
        co[cnum].price += NEWCOCOST;
    }
    if (e == NEWCO) {
        map[move + 1] = cnum + 'A';
        drawmap(move + 1, cnum + 'A');
        co[cnum].size++;
        co[cnum].price += NEWCOCOST;
    }
    wnoutrefresh(mapwin);
}

