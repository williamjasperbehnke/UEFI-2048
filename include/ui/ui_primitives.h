#ifndef UI_PRIMITIVES_H
#define UI_PRIMITIVES_H

#include <efi.h>

#include "ui/ui_context.h"

VOID ui_draw_text(UiContext *ctx, const CHAR16 *text, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
VOID ui_draw_text_number(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
VOID ui_draw_number_centered(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
UINTN ui_text_width(const CHAR16 *text, UINTN scale);
UINTN ui_number_width(UINTN value, UINTN scale);
EFI_GRAPHICS_OUTPUT_BLT_PIXEL ui_tile_color(UINT32 value);

#endif
