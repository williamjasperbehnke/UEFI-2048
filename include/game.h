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

VOID game_seed_rng(EFI_SYSTEM_TABLE *system_table);
VOID game_start_new(GameState *game);
BOOLEAN game_apply_move(GameState *game, MoveDir dir);
BOOLEAN game_has_moves(const GameState *game);

#endif
