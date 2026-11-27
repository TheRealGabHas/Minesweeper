#include <Raven.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

#include "minesweeper.h"


static void render_grid(int last_click_x, int last_click_y);
static void update_timer_label();

static int game_click_count = 0;
static int board_initialized = 0;

static thrd_t timer_thread = 0;
static int timer_running = 0;
static int game_timer = 0;

static Rid window;
static Rid grid;
static Rid *cells_button_list = NULL;

#define MAX_MINES_COUNT 5
static int MINES_LEVEL[MAX_MINES_COUNT] = {16, 64, 128, 256, 348};
static int MINES_COUNT_INDEX = 2;
static int mines_flagged_count = 0;

static int WINDOW_SPACING = 16;
static int BOARD_WIDTH = 32;
static int BOARD_HEIGHT = 16;
static int GRID_SPACING = 2;
static int CELL_WIDTH = 32;
static int CELL_HEIGHT = 32;

typedef struct {
    int x;
    int y;
    int discovered;
    MS_CELL_TYPE cell_type;
    int nearby_bombs;
} CellData;

static CellData *board_cells_list = NULL;

// ReSharper disable once CppParameterNeverUsed
static int increase_timer(void *args) {
    while (atomic_load(&timer_running) == 1) {
        const int current_value = atomic_load(&game_timer);

        if (current_value > 999) return 1;
        if (current_value < 999) game_timer++;
        if (current_value < 0) game_timer = 0;

        thrd_sleep(&(struct timespec) { .tv_sec=1 }, NULL);
    }

    return 1;
}

static int has_been_discovered(const int x, const int y) {
    if (board_cells_list == NULL) return 0;

    return board_cells_list[y * BOARD_WIDTH + x].discovered;
}

static void discover_bombs() {
    if (board_cells_list == NULL) return;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        const int x = i % BOARD_WIDTH;
        const int y = i / BOARD_WIDTH;
        if (board_cells_list[y * BOARD_WIDTH + x].cell_type == BOMB) {
            board_cells_list[y * BOARD_WIDTH + x].discovered = 1;
        }
    }
}

static void end_game() {
    // block the timer, lock the board
    timer_running = 0;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        const int x = i % BOARD_WIDTH;
        const int y = i / BOARD_WIDTH;

        const Rid button = cells_button_list[y * BOARD_WIDTH + x];
        Rwidget_set_enabled(button, 0);
    }
}

static int perform_discovery(const int x, const int y) {
    if (!ms_is_in_board(x, y, BOARD_WIDTH, BOARD_HEIGHT)) return 0;

    const int nearby_bombs = board_cells_list[y * BOARD_WIDTH + x].nearby_bombs;
    const int discovered = board_cells_list[y * BOARD_WIDTH + x].discovered;
    const MS_CELL_TYPE cell_type = board_cells_list[y * BOARD_WIDTH + x].cell_type;

    if (cell_type != EMPTY) return 0;   // Bomb, end of discovery
    if (nearby_bombs > 0 && discovered == 0) {
        board_cells_list[y * BOARD_WIDTH + x].discovered = 1;
        return 0;
    }
    if (nearby_bombs == 0 && discovered == 0) {
        board_cells_list[y * BOARD_WIDTH + x].discovered = 1;
        return 1;
    }
    if (discovered == 1) return 0; // Already discovered, end of discovery

    return 0;
}

static void cascade_discovery(const int x, const int y) {
    if (board_cells_list == NULL || cells_button_list == NULL) return;

    if (perform_discovery(x + 1, y) == 1) cascade_discovery(x + 1, y);
    if (perform_discovery(x - 1, y) == 1) cascade_discovery(x - 1, y);
    if (perform_discovery(x, y + 1) == 1) cascade_discovery(x, y + 1);
    if (perform_discovery(x, y - 1) == 1) cascade_discovery(x, y - 1);

    if (perform_discovery(x + 1, y - 1) == 1) cascade_discovery(x + 1, y - 1);
    if (perform_discovery(x - 1, y - 1) == 1) cascade_discovery(x - 1, y - 1);
    if (perform_discovery(x - 1, y + 1) == 1) cascade_discovery(x - 1, y + 1);
    if (perform_discovery(x + 1, y + 1) == 1) cascade_discovery(x + 1, y + 1);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static void on_cell_clicked(const Rid button, void* user_data) {
    (void) button;
    const CellData *data = user_data;
    CellData *actual_cell = &board_cells_list[data->y * BOARD_WIDTH + data->x];
    game_click_count++;

    if (board_initialized == 0) { // First click: generate the board
        ms_init_board(BOARD_WIDTH, BOARD_HEIGHT, data->x, data->y, MINES_LEVEL[MINES_COUNT_INDEX]);
        board_initialized = 1;

        for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
            const int x = i % BOARD_WIDTH;
            const int y = i / BOARD_WIDTH;
            CellData *cell = &board_cells_list[y * BOARD_WIDTH + x];
            if (cell->x == x && cell->y == y) {
                cell->cell_type = ms_get_value_at(x, y, BOARD_WIDTH);
                cell->nearby_bombs = ms_count_nearby_bombs(x, y, BOARD_WIDTH, BOARD_HEIGHT);
                cell->discovered = 0;
            }
        }
        if (thrd_create(&timer_thread, increase_timer, NULL) == thrd_success) {
            atomic_store(&timer_running, 1);
        } else printf("Failed to start the timer\n");
    }

    if (ms_is_in_board(data->x, data->y, BOARD_WIDTH, BOARD_HEIGHT)) {
        actual_cell->discovered = 1;
        if (actual_cell->cell_type == BOMB) {
            discover_bombs();
            end_game();
        }
        render_grid(data->x, data->y);
    }
}

static void on_cell_destroy(const Rid widget, void *user_data) {
    (void) widget;
    free(user_data);
}

static void update_mine_density_label() {
    char text[26] = "Mine Density: ";
    snprintf(text, sizeof(text), "Mine Density: %d", MINES_COUNT_INDEX + 1);

    const Rid mine_density_label = Rwidget_find(window, "mine-density-label");
    Rlabel_set_text(mine_density_label, text);
}

static void update_mine_count_label() {
    char text[21] = "Mines remaining: ";
    snprintf(text, sizeof(text), "Mines remaining: %d", MINES_LEVEL[MINES_COUNT_INDEX] - mines_flagged_count);

    const Rid mines_counter_label = Rwidget_find(window, "mine-counter-label");
    Rlabel_set_text(mines_counter_label, text);
}

static void update_timer_label() {
    char text[11] = "Timer: ";
    snprintf(text, sizeof(text), "Timer: %d", game_timer);

    const Rid timer_label = Rwidget_find(window, "timer-label");
    Rlabel_set_text(timer_label, text);
}

static void render_grid(const int last_click_x, const int last_click_y) {
    // First click, board generation: no new render needed
    if (board_initialized != 1) return;

    const CellData *last_clicked_cell = &board_cells_list[last_click_y * BOARD_WIDTH + last_click_x];

    if (last_clicked_cell->cell_type == EMPTY && last_clicked_cell->nearby_bombs == 0) {
        cascade_discovery(last_click_x, last_click_y);
    }

    update_mine_count_label();

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        const int x = i % BOARD_WIDTH;
        const int y = i / BOARD_WIDTH;

        const Rid current_cell = cells_button_list[y * BOARD_WIDTH + x];
        const CellData *board_cell = &board_cells_list[y * BOARD_WIDTH + x];

        char text[8] = " ";
        const int current_cell_value = board_cell->cell_type;
        const int nearby_bombs = board_cell->nearby_bombs;

        if (has_been_discovered(x, y) == 1) {
            if (current_cell_value == BOMB) {
                snprintf(text, sizeof(text), "x");
                Rwidget_set_class(current_cell, "bomb_discovered");
                Rwidget_set_enabled(current_cell, 0);
                Rbutton_set_text(current_cell, text);
                continue;
            }
            if (nearby_bombs > 0 && nearby_bombs <= 8) snprintf(text, sizeof(text), "%d", nearby_bombs);
            else if (current_cell_value == EMPTY) snprintf(text, sizeof(text), " ");
            Rbutton_set_text(current_cell, text);
            Rbutton_set_style(current_cell, R_BUTTON_FLAT);
        }
    }
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static void on_grid_generate(const Rid sender, const char* action, void* user_data) {
    (void) sender;
    (void) action;
    (void) user_data;

    update_mine_count_label();

    game_click_count = 0;
    board_initialized = 0;
    mines_flagged_count = 0;
    atomic_store(&timer_running, 0);
    atomic_store(&game_timer, 0);

    if (cells_button_list != NULL) {
        free(cells_button_list);
        cells_button_list = NULL;
    }
    if (board_cells_list != NULL) {
        free(board_cells_list);
        board_cells_list = NULL;
    }

    cells_button_list = malloc(BOARD_WIDTH * BOARD_HEIGHT * sizeof(Rid));
    board_cells_list = malloc(BOARD_WIDTH * BOARD_HEIGHT * sizeof(CellData));

    Rwidget_clear_children(grid);

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        const int x = i % BOARD_WIDTH;
        const int y = i / BOARD_WIDTH;
        const char text[8] = " ";

        const Rid cell = Rbutton_create(text);
        Rbutton_set_style(cell, R_BUTTON_PRIMARY);

        Rwidget_set_attribute(cell, "Appear", "Pop");
        Rwidget_set_attribute(cell, "AppearDelay", "0.1");
        Rwidget_set_size(cell, (float) CELL_WIDTH, (float) CELL_HEIGHT);
        Rslot_set_grid(cell, x, y);
        Rwidget_add_child(grid, cell);

        Rgrid_set_column_fill(grid, x, 1);
        Rgrid_set_row_fill(grid, y, 1);

        CellData *data = malloc(sizeof(CellData));
        data->x = x;
        data->y = y;
        data->discovered = 0;
        data->cell_type = UNKNOWN;
        data->nearby_bombs = 0;

        Rbutton_set_clicked_callback(cell, on_cell_clicked, data);
        Rwidget_set_destroy_callback(cell, on_cell_destroy, data);

        cells_button_list[i] = cell;
        board_cells_list[i] = *data;
    }

    render_grid(-1, -1);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static void on_grid_nextsize(const Rid sender, const char* action, void* user_data) {
    (void) sender;
    (void) action;
    (void) user_data;

    MINES_COUNT_INDEX = (MINES_COUNT_INDEX + 1) % MAX_MINES_COUNT;
    update_mine_density_label();

    if (board_initialized != 1) { update_mine_count_label(); }

}

int main(void) {
    if (!Rinit(R_BACKEND_VULKAN)) return 1;

    window = Rwindow_create_from_xml_file("views/main.xml");

    if (window == R_INVALID_ID) { return 1; }
    const int min_width = BOARD_WIDTH * CELL_WIDTH + (BOARD_WIDTH - 1) * GRID_SPACING + WINDOW_SPACING * 2;
    const int min_height = BOARD_HEIGHT * CELL_HEIGHT + (BOARD_HEIGHT - 1) * GRID_SPACING + WINDOW_SPACING * 2;

    Rwindow_set_min_size(window, min_width, min_height);

    update_mine_density_label();
    update_mine_count_label();

    Rtheme_set_style_color("Button", "bomb_discovered", R_COLOR_ACCENT, 0.8f, 0.4f, 0.3f, 0.8f);

    // Populating variables
    grid = Rwidget_find(window, "minefield-grid");

    // Action binding
    Raction_bind("app.grid.generate", on_grid_generate, NULL);
    Raction_bind("app.grid.nextsize", on_grid_nextsize, NULL);

    // Main loop
    while (Rwait_frame(0.25)) {
        update_timer_label();
    }

    Rshutdown();
    return 0;
}