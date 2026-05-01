#ifndef UI_PRIMITIVES_H
#define UI_PRIMITIVES_H

#include <efi.h>

#include "ui_context.h"

VOID ui_draw_text(UiContext *ctx, const CHAR16 *text, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
VOID ui_draw_text_number(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
VOID ui_draw_number_centered(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
EFI_GRAPHICS_OUTPUT_BLT_PIXEL ui_tile_color(UINT32 value);

#endif
