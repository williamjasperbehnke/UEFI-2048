#ifndef UI_H
#define UI_H

#include <efi.h>
#include "game.h"

VOID ui_clear_screen(EFI_SYSTEM_TABLE *system_table);
VOID ui_draw_board(EFI_SYSTEM_TABLE *system_table, const GameState *game);

#endif
