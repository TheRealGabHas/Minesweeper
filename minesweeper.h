//
// Created by jilso on 14/11/2026.
//

#ifndef MINESWEEPER_MINESWEEPER_H
#define MINESWEEPER_MINESWEEPER_H

#define MS_MAX_HEIGHT 120
#define MS_MAX_WIDTH 120

typedef enum ms_click_types {
    LEFT_CLICK,
    RIGHT_CLICK
} MS_CLICK_TYPE;

typedef enum ms_game_states {
    VALID_BOARD,
    DEFEAT,
    VICTORY
} MS_GAME_STATE;

typedef enum ms_cell_types {
    EMPTY,
    BOMB,
    UNKNOWN
} MS_CELL_TYPE;

static int *ms_board = NULL;

int update_board_state(MS_CLICK_TYPE clickType, int x, int y);
MS_GAME_STATE check_game_state();
void ms_init_board(int width, int height, int initial_x, int initial_y, int mines_count);
void ms_free_board();
void ms_print_board(int width, int height);
int ms_get_value_at(int x, int y, int board_width);
static int random_int(int min, int max);
static int ms_are_neighbors(int x, int y, int initial_x, int initial_y);
int ms_count_nearby_bombs(int x, int y, int board_width, int board_height);
int ms_is_in_board(int x, int y, int board_width, int board_height);
#endif //MINESWEEPER_MINESWEEPER_H


/*
 * 1. Clic sur une case
 * 2. Savoir si c'est un clic droit ou gauche
 * 3. Envoyer le type de clic + les coordonnées à update_game_state
 * 4. check_game_state:
 *      - si une case est invalide -> défaite
 *      - si tout est valide et 0 mine restante -> victoire
 */