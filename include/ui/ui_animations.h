#ifndef UI_ANIMATIONS_H
#define UI_ANIMATIONS_H

#include <efi.h>
#include "game.h"

typedef struct {
    BOOLEAN slide_enabled;
    UINTN move_frames;
    UINTN move_stall_us;
} UiAnimationConfig;

typedef struct {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *full_base;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *full_frame;
    UINTN full_w;
    UINTN full_h;
    UiAnimationConfig config;
} UiAnimationState;

VOID ui_animations_init(UiAnimationState *state);
VOID ui_animations_prime(EFI_SYSTEM_TABLE *system_table);
VOID ui_animations_run_move(UiAnimationState *state, EFI_SYSTEM_TABLE *system_table, const GameState *before, const GameState *after, MoveDir dir);

#endif
