#ifndef GAME_H
#define GAME_H

#include <efi.h>

#define BOARD_SIZE 4
#define TARGET_TILE 2048

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

static inline VOID game_map_line_index_to_cell(
    MoveDir dir,
    UINTN line,
    UINTN k,
    UINTN *out_r,
    UINTN *out_c
) {
    if (dir == DIR_LEFT) {
        *out_r = line;
        *out_c = k;
    } else if (dir == DIR_RIGHT) {
        *out_r = line;
        *out_c = BOARD_SIZE - 1 - k;
    } else if (dir == DIR_UP) {
        *out_r = k;
        *out_c = line;
    } else {
        *out_r = BOARD_SIZE - 1 - k;
        *out_c = line;
    }
}

VOID game_seed_rng(EFI_SYSTEM_TABLE *system_table);
VOID game_start_new(GameState *game);
BOOLEAN game_apply_move(GameState *game, MoveDir dir);
BOOLEAN game_has_moves(const GameState *game);

#endif
