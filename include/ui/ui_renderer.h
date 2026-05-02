#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "game.h"

VOID ui_clear_screen(EFI_SYSTEM_TABLE *system_table);
VOID ui_draw_board(EFI_SYSTEM_TABLE *system_table, const GameState *game, BOOLEAN is_game_over, BOOLEAN show_win_overlay);
VOID ui_draw_tutorial_frame(EFI_SYSTEM_TABLE *system_table, UINTN frame_index);

#endif
