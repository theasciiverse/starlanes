#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "conf.h"
#include CURSES_HEADER
#include "colors.h"
#include "company.h"
#include "consts.h"
#include "globals.h"
#include "map.h"
#include "move.h"
#include "player.h"
#include "quit.h"
#include "trade.h"
#include "ui.h"

/**
 * Sets up the map, players, and companies
 */
void initialize(void)
{
    int i, j;

    // Get the size of the screen
    SCREEN_LINES = DEF_SCREEN_LINES;
    SCREEN_COLUMNS = DEF_SCREEN_COLUMNS;
    place_origin();

    // Allocate space for everything

    if ((map = malloc(MAPX * MAPY)) == NULL) {
        fprintf(stderr, "starlanes: error mallocing space for map\n");
        exit(1);
    }

    if ((co = calloc(1, NUMCO * sizeof(COMPANY))) == NULL) {
        fprintf(stderr,
                "starlanes: error mallocing space for companies\n");
        exit(1);
    }

    if ((pl = calloc(1, MAXPLAYERS * sizeof(PLAYER))) == NULL) {
        fprintf(stderr, "starlanes: error mallocing space for players\n");
        exit(1);
    }

    // Set up the map

    for (i = 0; i < MAPX; i++) {
        for (j = 0; j < MAPY; j++) {
            int n = rand() % 60;
            if (n == 0)
                map[i + j * MAPX] = BLACKHOLE;  // 1/60 chance
            else if (n <= 3)
                map[i + j * MAPX] = STAR;       // 3/60 chance
            else
                map[i + j * MAPX] = SPACE;      // 56/60 chance
        }
    }

    // Initialize the companies

    for (i = 0; i < NUMCO; i++) {
        switch (i) {
        case 0:
            strcpy(co[i].name, "Altair Starways");
            break;
        case 1:
            strcpy(co[i].name, "Beetlejuice, Ltd.");
            break;
        case 2:
            strcpy(co[i].name, "Capella Freight Co.");
            break;
        case 3:
            strcpy(co[i].name, "Denebola Shippers");
            break;
        case 4:
            strcpy(co[i].name, "Eridani Expediters");
            break;
        }
        co[i].size = co[i].price = 0;
    }

    // Initialize the players

    for (i = 0; i < MAXPLAYERS; i++) {
        pl[i].name[0] = '\0';
        pl[i].cash = INIT_CASH;
        for (j = 0; j < NUMCO; j++)
            pl[i].holdings[j] = 0;
    }

    // Create the windows

    if ((mapwin = sl_newwin(MAPY + 2, MAPX * 3 + 2, 1, 1)) == NULL) {
        fprintf(stderr, "starlanes: can't create map window\n");
        exit(1);
    }

    if ((general =
         sl_newwin(SCREEN_LINES - 2 - MAPY - 2, SCREEN_COLUMNS - 2, MAPY + 3, 1)) == NULL) {
        fprintf(stderr, "starlanes: can't create general window\n");
        exit(1);
    }

    if ((coinfo =
         sl_newwin(MAPY + 2, SCREEN_COLUMNS - 2 - (MAPX * 3 + 2), 1,
                MAPX * 3 + 3)) == NULL) {
        fprintf(stderr, "starlanes: can't create coinfo window\n");
        exit(1);
    }

    // Set keypad mode so the arrow keys work

    keypad(stdscr, 1);
    keypad(general, 1);
}

/**
 * Prompts for the number of players
 */
void get_num_players(void)
{
    char c, s[80];
    int i;

    clear();
    refresh();

    // One row short of SCREEN_LINES, so it fits an 80x24 terminal like
    // the game screen does
    WINDOW *npwin = sl_newwin(SCREEN_LINES - 1, SCREEN_COLUMNS, 0, 0);

    if (color)
        wattron(npwin, YELLOW_ON_BLACK | A_BOLD);

    box(npwin, '$', '$');

    sprintf(s, " V%s ", VERSION);
    center(npwin, SCREEN_COLUMNS, SCREEN_LINES - 2, s);
    if (color)
        wattroff(npwin, YELLOW_ON_BLACK | A_BOLD);

    wattron(npwin, color ? YELLOW_ON_BLUE | A_BOLD : A_REVERSE);
    center(npwin, SCREEN_COLUMNS, 6, "* S * T * A * R ** L * A * N * E * S *");
    wattroff(npwin, color ? YELLOW_ON_BLUE | A_BOLD : A_REVERSE);
    sprintf(s, "Please enter the number of players [1-%d]: ", MAXPLAYERS);
    center(npwin, SCREEN_COLUMNS, 9, s);
    wrefresh(npwin);
    noecho();
    raw();
    do {
        c = sl_getch() - '0';
    } while (c < 1 || c > MAXPLAYERS);
    waddch(npwin, c + '0');

    numplayers = (int) c;
    srand(getpid());            // Reseed the dumb random number generator
    turn = rand() % numplayers;

    nl();
    for (i = 0; i < numplayers; i++) {
        sprintf(s, "Player %d, enter your name: ", i + 1);
        center(npwin, SCREEN_COLUMNS - 8, 11 + i, s);
        wrefresh(npwin);
        my_mvwgetstr(npwin, i + 11, 49, 20, 0, pl[i].name);
        if (pl[i].name[0] == '\0')
            i--;
    }
    nonl();

    sl_delwin(npwin);
    clear();
    refresh();
}

/**
 * Counts the number of non-empty sectors.
 */
int count_used_sectors(void)
{
    int maptotal, i;

    for (i = maptotal = 0; i < MAPX * MAPY; i++)        // Must be enough room to move
        if (map[i] != SPACE)
            maptotal++;

    return maptotal;
}

/**
 * Returns true if the game is over.
 */
int check_endgame(void)
{
    int sum = 0, i, maptotal;

    for (i = 0; i < NUMCO; i++)
        sum += co[i].size;

    maptotal = count_used_sectors();

    return (sum * 100) / (MAPX * MAPY) >= END_PERCENT
        && maptotal <= MAPX * MAPY - 4;
}

/**
 * Prints a stderr usage message.
 */
void usage(void)
{
    fprintf(stderr, "usage: starlanesb [-v|c|m]\n");
    exit(1);
}

/**
 * Main.
 */
int main(int argc, char *argv[])
{
    int done = 0, move, colorforce = 0, monoforce = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v") || !strncmp(argv[i], "--v", 3)) {
            fprintf(stderr, "Starlanes for ncurses v%s\n", VERSION);
            return 0;
        }

        else if (!strcmp(argv[i], "-m") || !strncmp(argv[i], "--m", 3)) {
            monoforce = 1;
        }

        else if (!strcmp(argv[i], "-c") || !strncmp(argv[i], "--c", 3)) {
            colorforce = 1;
        }

        else usage();
    }

    initscr();
    if (LINES < DEF_SCREEN_LINES - 1 || COLS < DEF_SCREEN_COLUMNS) {
        endwin();
        fprintf(stderr, "starlanes: the terminal must be at least %dx%d\n",
                DEF_SCREEN_COLUMNS, DEF_SCREEN_LINES - 1);
        return 1;
    }
    start_color();

    if (colorforce)
        color = 1;
    else if (monoforce)
        color = 0;
    else
        color = has_colors();

    if (color)
        color_setup();

    raw();

    // Init map, stocks
    srand(time(NULL));
    initialize();

    // Num players
    get_num_players();

    clear();
    in_game = 1;
    draw_titlebar();
    wnoutrefresh(stdscr);
    showmap();
    show_coinfo();

    do {
        move = get_move();
        do_move(move);
        holding_bonus();
        if ((done = check_endgame()) != 1) {
            buy_sell();
            turn = (turn + 1) % numplayers;
        }
    } while (!done);

    shutdown();

    return 0;
}

