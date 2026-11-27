//
// Created by jilso on 14/11/2026.
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "minesweeper.h"

static unsigned int ms_random_seed;

static int random_int(const int min, const int max) {
    const int rd_num = rand_r(&ms_random_seed) % (max - min + 1) + min;
    return rd_num;
}

void ms_init_board(const int width, const int height, const int initial_x, const int initial_y, const int mines_count) {
    if (width <= 0 || height <= 0 ||
        width > MS_MAX_WIDTH || height > MS_MAX_HEIGHT) {
        return;
    }
    ms_random_seed = time(0);

    int *new_board = calloc(width * height, sizeof(int));

    if (new_board == NULL) return;
    // if (ms_board != NULL) ms_free_board();

    ms_board = new_board;

    // Populate the board
    int mines_to_place = mines_count;
    int failure_count = 0;
    while (mines_to_place > 0) {
        if (failure_count > 1000) {
            printf("To many collisions, aborting board generation (%d)\n", failure_count);
            return;
        }
        const int x = random_int(0, width - 1);
        const int y = random_int(0, height - 1);
        // Check if (x, y) is a safe place (nearby opening)
        if (x == initial_x && y == initial_y) {
            failure_count++;
            continue;
        }
        // Make surroundings safe
        if (ms_are_neighbors(x, y, initial_x, initial_y)) {
            failure_count++;
            continue;
        }
        ms_board[y * width + x] = BOMB;
        mines_to_place--;
    }
    printf("board initialized successfully. (%d collisions)\n", failure_count);
}

void ms_free_board() {
    free(ms_board);
    ms_board = NULL;
}

void ms_print_board(const int width, const int height) {
    if (ms_board == NULL) {
        printf("Board in null\n");
        return;
    }
    for (int i = 0; i < width * height; i++) {
        printf("%d ", ms_board[i]);
        if ((i + 1) % width == 0) printf("\n");
    }
    printf("\n");
}

int ms_get_value_at(const int x, const int y, const int board_width) {
    if (ms_board == NULL) return -1;

    return ms_board[y * board_width + x];
}

static int ms_are_neighbors(const int x, const int y, const int initial_x, const int initial_y) {
    return x == initial_x + 1 && y == initial_y + 1 ||
           x == initial_x + 1 && y == initial_y - 1 ||
           x == initial_x - 1 && y == initial_y + 1 ||
           x == initial_x - 1 && y == initial_y - 1 ||
           x == initial_x     && y == initial_y + 1 ||
           x == initial_x     && y == initial_y - 1 ||
           x == initial_x + 1 && y == initial_y     ||
           x == initial_x - 1 && y == initial_y;
}

int ms_is_in_board(const int x, const int y, const int board_width, const int board_height) {
    return (y >= 0 && y < board_height && x >= 0 && x < board_width);
}

int ms_count_nearby_bombs(const int x, const int y, const int board_width, const int board_height) {
    if (ms_board == NULL) return -1;
    int total = 0;
    if (ms_is_in_board(x + 1, y + 1, board_width, board_height)) total += ms_get_value_at(x + 1, y + 1, board_width);
    if (ms_is_in_board(x + 1, y - 1, board_width, board_height)) total += ms_get_value_at(x + 1, y - 1, board_width);
    if (ms_is_in_board(x - 1, y + 1, board_width, board_height)) total += ms_get_value_at(x - 1, y + 1, board_width);
    if (ms_is_in_board(x - 1, y - 1, board_width, board_height)) total += ms_get_value_at(x - 1, y - 1, board_width);
    if (ms_is_in_board(x, y - 1, board_width, board_height)) total += ms_get_value_at(x, y - 1, board_width);
    if (ms_is_in_board(x, y + 1, board_width, board_height)) total += ms_get_value_at(x, y + 1, board_width);
    if (ms_is_in_board(x - 1, y, board_width, board_height)) total += ms_get_value_at(x - 1, y, board_width);
    if (ms_is_in_board(x + 1, y, board_width, board_height)) total += ms_get_value_at(x + 1, y, board_width);
    return total;
}