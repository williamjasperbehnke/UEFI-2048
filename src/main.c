#include <efi.h>
#include <efilib.h>

#define BOARD_SIZE 4
#define BOARD_CELLS (BOARD_SIZE * BOARD_SIZE)
#define TARGET_TILE 2048
#define START_TILE_COUNT 2
#define RANDOM_FOUR_DENOM 10

typedef enum {
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} MoveDir;

typedef struct {
    UINT32 cells[BOARD_SIZE][BOARD_SIZE];
    UINTN score;
    BOOLEAN won;
} GameState;

static UINT64 g_rng = 0;

static UINT64 lcg_next(void) {
    g_rng = g_rng * 6364136223846793005ULL + 1;
    return g_rng;
}

static UINTN rand_n(UINTN n) {
    if (n == 0) {
        return 0;
    }
    return (UINTN)(lcg_next() % n);
}

static VOID seed_rng(EFI_SYSTEM_TABLE *system_table) {
    EFI_TIME t;
    EFI_STATUS st = uefi_call_wrapper(system_table->RuntimeServices->GetTime, 2, &t, NULL);
    UINT64 seed = 0xC0FFEEULL;

    if (!EFI_ERROR(st)) {
        seed ^= ((UINT64)t.Nanosecond << 16);
        seed ^= ((UINT64)t.Second << 8);
        seed ^= (UINT64)t.Minute;
    }

    seed ^= (UINT64)(UINTN)system_table;
    g_rng = seed ? seed : 1;
}

static VOID clear_screen(EFI_SYSTEM_TABLE *system_table) {
    uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
}

static VOID init_game(GameState *game) {
    game->score = 0;
    game->won = FALSE;

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            game->cells[r][c] = 0;
        }
    }
}

static BOOLEAN add_random_tile(GameState *game) {
    UINTN empty_rows[BOARD_CELLS];
    UINTN empty_cols[BOARD_CELLS];
    UINTN empty_count = 0;

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            if (game->cells[r][c] == 0) {
                empty_rows[empty_count] = r;
                empty_cols[empty_count] = c;
                empty_count++;
            }
        }
    }

    if (empty_count == 0) {
        return FALSE;
    }

    UINTN idx = rand_n(empty_count);
    game->cells[empty_rows[idx]][empty_cols[idx]] = (rand_n(RANDOM_FOUR_DENOM) == 0) ? 4 : 2;
    return TRUE;
}

static VOID start_new_game(GameState *game) {
    init_game(game);

    for (UINTN i = 0; i < START_TILE_COUNT; ++i) {
        add_random_tile(game);
    }
}

/*
 * 2048 line rule: compact non-zero values, merge equal neighbors once,
 * then compact again. Returns whether the line changed.
 */
static BOOLEAN compress_and_merge_line(const UINT32 in[BOARD_SIZE], UINT32 out[BOARD_SIZE], UINTN *score_gain, BOOLEAN *won) {
    UINT32 compacted[BOARD_SIZE] = {0, 0, 0, 0};
    UINT32 merged[BOARD_SIZE] = {0, 0, 0, 0};
    UINTN compact_count = 0;
    UINTN merged_count = 0;
    BOOLEAN moved = FALSE;

    for (UINTN i = 0; i < BOARD_SIZE; ++i) {
        if (in[i] != 0) {
            compacted[compact_count++] = in[i];
        }
    }

    for (UINTN i = 0; i < compact_count; ++i) {
        if (i + 1 < compact_count && compacted[i] == compacted[i + 1]) {
            UINT32 value = compacted[i] * 2;
            merged[merged_count++] = value;
            *score_gain += value;
            if (value >= TARGET_TILE) {
                *won = TRUE;
            }
            i++;
        } else {
            merged[merged_count++] = compacted[i];
        }
    }

    for (UINTN i = 0; i < BOARD_SIZE; ++i) {
        out[i] = merged[i];
        if (out[i] != in[i]) {
            moved = TRUE;
        }
    }

    return moved;
}

static VOID read_line(const GameState *game, MoveDir dir, UINTN index, UINT32 line[BOARD_SIZE]) {
    for (UINTN i = 0; i < BOARD_SIZE; ++i) {
        switch (dir) {
            case DIR_LEFT:
                line[i] = game->cells[index][i];
                break;
            case DIR_RIGHT:
                line[i] = game->cells[index][BOARD_SIZE - 1 - i];
                break;
            case DIR_UP:
                line[i] = game->cells[i][index];
                break;
            case DIR_DOWN:
                line[i] = game->cells[BOARD_SIZE - 1 - i][index];
                break;
        }
    }
}

static VOID write_line(GameState *game, MoveDir dir, UINTN index, const UINT32 line[BOARD_SIZE]) {
    for (UINTN i = 0; i < BOARD_SIZE; ++i) {
        switch (dir) {
            case DIR_LEFT:
                game->cells[index][i] = line[i];
                break;
            case DIR_RIGHT:
                game->cells[index][BOARD_SIZE - 1 - i] = line[i];
                break;
            case DIR_UP:
                game->cells[i][index] = line[i];
                break;
            case DIR_DOWN:
                game->cells[BOARD_SIZE - 1 - i][index] = line[i];
                break;
        }
    }
}

static BOOLEAN apply_move(GameState *game, MoveDir dir) {
    BOOLEAN moved_any = FALSE;

    for (UINTN line_idx = 0; line_idx < BOARD_SIZE; ++line_idx) {
        UINT32 in[BOARD_SIZE];
        UINT32 out[BOARD_SIZE];
        UINTN score_gain = 0;
        BOOLEAN won_line = FALSE;

        read_line(game, dir, line_idx, in);

        if (!compress_and_merge_line(in, out, &score_gain, &won_line)) {
            continue;
        }

        moved_any = TRUE;
        write_line(game, dir, line_idx, out);
        game->score += score_gain;
        if (won_line) {
            game->won = TRUE;
        }
    }

    return moved_any;
}

static BOOLEAN has_moves(const GameState *game) {
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            UINT32 value = game->cells[r][c];
            if (value == 0) {
                return TRUE;
            }
            if (c + 1 < BOARD_SIZE && game->cells[r][c + 1] == value) {
                return TRUE;
            }
            if (r + 1 < BOARD_SIZE && game->cells[r + 1][c] == value) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static VOID print_separator(void) {
    Print(L"+------+------+------+------+\n");
}

static VOID draw_board(EFI_SYSTEM_TABLE *system_table, const GameState *game) {
    clear_screen(system_table);

    Print(L"UEFI 2048\n");
    Print(L"Score: %lu\n", (unsigned long)game->score);
    Print(L"Use Arrow Keys (or WASD), R to restart, Q to quit\n\n");

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        print_separator();
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            if (game->cells[r][c] == 0) {
                Print(L"|%6s", L".");
            } else {
                Print(L"|%6u", game->cells[r][c]);
            }
        }
        Print(L"|\n");
    }
    print_separator();

    if (game->won) {
        Print(L"\nYou reached %d! Keep going or press R to restart.\n", TARGET_TILE);
    }
    if (!has_moves(game)) {
        Print(L"\nGame Over! Press R to restart or Q to quit.\n");
    }
}

static EFI_STATUS wait_key(EFI_SYSTEM_TABLE *system_table, EFI_INPUT_KEY *key) {
    UINTN index = 0;
    EFI_STATUS st = uefi_call_wrapper(system_table->BootServices->WaitForEvent, 3, 1, &system_table->ConIn->WaitForKey, &index);

    if (EFI_ERROR(st)) {
        return st;
    }

    return uefi_call_wrapper(system_table->ConIn->ReadKeyStroke, 2, system_table->ConIn, key);
}

static BOOLEAN is_quit_key(const EFI_INPUT_KEY *key) {
    return key->UnicodeChar == L'q' || key->UnicodeChar == L'Q' || key->ScanCode == SCAN_ESC;
}

static BOOLEAN is_restart_key(const EFI_INPUT_KEY *key) {
    return key->UnicodeChar == L'r' || key->UnicodeChar == L'R';
}

/* Map firmware key input to a board move direction. */
static BOOLEAN key_to_move(const EFI_INPUT_KEY *key, MoveDir *dir) {
    if (key->ScanCode == SCAN_LEFT || key->UnicodeChar == L'a' || key->UnicodeChar == L'A') {
        *dir = DIR_LEFT;
        return TRUE;
    }
    if (key->ScanCode == SCAN_RIGHT || key->UnicodeChar == L'd' || key->UnicodeChar == L'D') {
        *dir = DIR_RIGHT;
        return TRUE;
    }
    if (key->ScanCode == SCAN_UP || key->UnicodeChar == L'w' || key->UnicodeChar == L'W') {
        *dir = DIR_UP;
        return TRUE;
    }
    if (key->ScanCode == SCAN_DOWN || key->UnicodeChar == L's' || key->UnicodeChar == L'S') {
        *dir = DIR_DOWN;
        return TRUE;
    }
    return FALSE;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    seed_rng(SystemTable);

    GameState game;
    start_new_game(&game);

    while (1) {
        draw_board(SystemTable, &game);

        EFI_INPUT_KEY key;
        EFI_STATUS st = wait_key(SystemTable, &key);
        if (EFI_ERROR(st)) {
            continue;
        }

        if (is_restart_key(&key)) {
            start_new_game(&game);
            continue;
        }
        if (is_quit_key(&key)) {
            break;
        }

        if (!has_moves(&game)) {
            continue;
        }

        MoveDir dir;
        if (key_to_move(&key, &dir) && apply_move(&game, dir)) {
            add_random_tile(&game);
        }
    }

    clear_screen(SystemTable);
    Print(L"Thanks for playing UEFI 2048.\n");
    return EFI_SUCCESS;
}
