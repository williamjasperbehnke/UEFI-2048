#ifndef UI_TILE_ATLAS_H
#define UI_TILE_ATLAS_H

#include <efi.h>

#include "ui/ui_context.h"

VOID ui_tile_atlas_ensure_size(EFI_SYSTEM_TABLE *system_table, UINTN tile_size);
VOID ui_tile_atlas_learn_from_cell(UiContext *ctx, const UiGridLayout *layout, UINTN row, UINTN col, UINT32 value);
const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *ui_tile_atlas_sprite(UINT32 value);
BOOLEAN ui_tile_atlas_blit_video(UiContext *ctx, const UiGridLayout *layout, UINT32 value, UINTN x, UINTN y);

#endif
