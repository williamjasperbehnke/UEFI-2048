#ifndef UI_BOARD_TRANSITION_CACHE_H
#define UI_BOARD_TRANSITION_CACHE_H

#include <efi.h>

#include "game.h"
#include "ui/ui_context.h"

typedef struct {
    UINT32 value;
    UINTN from_r;
    UINTN from_c;
    UINTN to_r;
    UINTN to_c;
    BOOLEAN moved;
} UiAnimTile;

typedef struct {
    UiAnimTile anim_tiles[BOARD_SIZE * BOARD_SIZE];
    UINTN anim_tile_count;
    UINT32 anim_board[BOARD_SIZE][BOARD_SIZE];
    MoveDir anim_dir;
    BOOLEAN anim_valid;

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *board_base_buffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *board_frame_buffer;
    UINTN board_buf_w;
    UINTN board_buf_h;

    UINTN last_board_x;
    UINTN last_board_y;
    UINTN last_board_w;
    UINTN last_board_h;
    BOOLEAN auto_present;
} UiBoardTransitionCache;

UiBoardTransitionCache *ui_board_transition_cache_get(VOID);
VOID ui_board_transition_cache_reset_anim(VOID);
VOID ui_board_transition_cache_free_board_buffers(EFI_SYSTEM_TABLE *system_table);
BOOLEAN ui_board_transition_cache_ensure_board_buffers(UiContext *ctx, const UiGridLayout *layout);

#endif
