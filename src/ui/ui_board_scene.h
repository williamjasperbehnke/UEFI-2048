#ifndef UI_BOARD_SCENE_H
#define UI_BOARD_SCENE_H

#include "game.h"
#include "ui_context.h"

VOID ui_render_board_scene(UiContext *ctx, const GameState *game, BOOLEAN is_game_over, BOOLEAN show_win_overlay);

#endif
