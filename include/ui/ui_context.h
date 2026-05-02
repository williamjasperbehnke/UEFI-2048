#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include <efi.h>
#include <efiprot.h>
#include "game.h"

typedef struct {
    EFI_SYSTEM_TABLE *system_table;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    UINTN screen_w;
    UINTN screen_h;
} UiContext;

typedef struct {
    UINTN x;
    UINTN y;
    UINTN w;
    UINTN h;
} UiRect;

typedef struct {
    UiRect board_rect;
    UINTN gap;
    UINTN tile;
} UiGridLayout;

BOOLEAN ui_ctx_begin(UiContext *ctx, EFI_SYSTEM_TABLE *system_table);
EFI_GRAPHICS_OUTPUT_BLT_PIXEL ui_rgb(UINT8 r, UINT8 g, UINT8 b);
VOID ui_fill_rect(UiContext *ctx, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
UiGridLayout ui_make_grid_layout(const UiContext *ctx, UINTN board_scale_percent, UINTN min_gap);
UiRect ui_grid_cell_rect(const UiGridLayout *layout, UINTN row, UINTN col);

#endif
