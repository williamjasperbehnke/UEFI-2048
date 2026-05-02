#ifndef UI_BOARD_TRANSITION_H
#define UI_BOARD_TRANSITION_H

#include <efi.h>

#include "game.h"
#include "ui/ui_context.h"

VOID ui_prime_board_tile_sprites(UiContext *ctx);
VOID ui_render_board_transition_scene(
    UiContext *ctx,
    const GameState *from_game,
    MoveDir dir,
    UINTN progress_numer,
    UINTN progress_denom
);
VOID ui_render_board_pop_scene(
    UiContext *ctx,
    const GameState *game,
    const BOOLEAN pop_mask[BOARD_SIZE][BOARD_SIZE],
    UINTN scale_percent
);
VOID ui_set_transition_auto_present(BOOLEAN enabled);
BOOLEAN ui_get_last_transition_frame(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL **out_buf,
    UINTN *out_x,
    UINTN *out_y,
    UINTN *out_w,
    UINTN *out_h
);

#endif
