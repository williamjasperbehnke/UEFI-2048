/* Game module: board state, movement rules, scoring, and RNG tile spawns. */
#include "game.h"

#define BOARD_CELLS (BOARD_SIZE * BOARD_SIZE)
#define START_TILE_COUNT 2
#define RANDOM_FOUR_DENOM 10

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

VOID game_seed_rng(EFI_SYSTEM_TABLE *system_table) {
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

static VOID game_init(GameState *game) {
    game->score = 0;
    game->won = FALSE;

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            game->cells[r][c] = 0;
        }
    }
}

static BOOLEAN game_add_random_tile(GameState *game) {
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

VOID game_start_new(GameState *game) {
    game_init(game);

    for (UINTN i = 0; i < START_TILE_COUNT; ++i) {
        game_add_random_tile(game);
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

BOOLEAN game_apply_move(GameState *game, MoveDir dir) {
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

    if (moved_any) {
        game_add_random_tile(game);
    }

    return moved_any;
}

BOOLEAN game_has_moves(const GameState *game) {
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
