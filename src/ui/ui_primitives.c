/* UI primitives: bitmap font, 7-segment digits, and shared color palette. */
#include "ui_primitives.h"
#include <efilib.h>

typedef struct {
    CHAR16 ch;
    UINT8 rows[7];
} Glyph5x7;

static const Glyph5x7 GLYPHS[] = {
    {L'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {L'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {L'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {L'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {L'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {L'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {L'G', {0x0E, 0x11, 0x10, 0x13, 0x11, 0x11, 0x0E}},
    {L'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {L'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {L'J', {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}},
    {L'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {L'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {L'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {L'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {L'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {L'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {L'R', {0x1E, 0x11, 0x11, 0x1E, 0x12, 0x11, 0x11}},
    {L'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {L'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {L'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {L'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {L'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
    {L'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {L'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {L'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {L'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {L'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {L'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {L'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {L'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {L'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {L'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {L'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {L'=', {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {L'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {L' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

static UINT8 glyph_row(CHAR16 ch, UINTN row) {
    for (UINTN i = 0; i < sizeof(GLYPHS) / sizeof(GLYPHS[0]); ++i) {
        if (GLYPHS[i].ch == ch) {
            return GLYPHS[i].rows[row];
        }
    }
    return 0;
}

VOID ui_draw_text(UiContext *ctx, const CHAR16 *text, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    for (UINTN i = 0; text[i] != L'\0'; ++i) {
        for (UINTN row = 0; row < 7; ++row) {
            UINT8 bits = glyph_row(text[i], row);
            for (UINTN col = 0; col < 5; ++col) {
                if (bits & (1U << (4 - col))) {
                    ui_fill_rect(ctx, x + (i * 6 + col) * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

UINTN ui_text_width(const CHAR16 *text, UINTN scale) {
    if (text == NULL || text[0] == L'\0') {
        return 0;
    }
    return StrLen(text) * 6 * scale;
}

UINTN ui_number_width(UINTN value, UINTN scale) {
    UINTN digits = 1;
    while (value >= 10) {
        value /= 10;
        ++digits;
    }
    return digits * 6 * scale;
}

VOID ui_draw_text_number(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN scale, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    CHAR16 buf[24];
    UINTN i = 0;

    if (value == 0) {
        buf[i++] = L'0';
    } else {
        CHAR16 rev[24];
        UINTN n = 0;
        while (value > 0 && n < 24) {
            rev[n++] = (CHAR16)(L'0' + (value % 10));
            value /= 10;
        }
        while (n > 0) {
            buf[i++] = rev[--n];
        }
    }

    buf[i] = L'\0';
    ui_draw_text(ctx, buf, x, y, scale, color);
}

VOID ui_draw_number_centered(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    const UINTN base_digits = 4; /* Keep 2..2048 at one consistent visual size. */
    UINTN scale_from_height = h / 9;
    UINTN scale_from_width = w / (base_digits * 6 + 2);
    UINTN scale = (scale_from_height < scale_from_width) ? scale_from_height : scale_from_width;

    /* If value grows beyond 4 digits, shrink only as much as needed to fit. */
    UINTN value_fit_scale = w / (ui_number_width(value, 1) + 2);
    if (value_fit_scale < scale) {
        scale = value_fit_scale;
    }

    if (scale < 1) {
        scale = 1;
    }

    UINTN text_w = ui_number_width(value, scale);
    UINTN text_h = 7 * scale;
    UINTN ox = x + (w - text_w) / 2 + 2;
    UINTN oy = y + (h - text_h) / 2;
    ui_draw_text_number(ctx, value, ox, oy, scale, color);
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL ui_tile_color(UINT32 value) {
    switch (value) {
        case 0: return ui_rgb(205, 193, 180);
        case 2: return ui_rgb(238, 228, 218);
        case 4: return ui_rgb(237, 224, 200);
        case 8: return ui_rgb(242, 177, 121);
        case 16: return ui_rgb(245, 149, 99);
        case 32: return ui_rgb(246, 124, 95);
        case 64: return ui_rgb(246, 94, 59);
        case 128: return ui_rgb(237, 207, 114);
        case 256: return ui_rgb(237, 204, 97);
        case 512: return ui_rgb(237, 200, 80);
        case 1024: return ui_rgb(237, 197, 63);
        case 2048: return ui_rgb(237, 194, 46);
        default: return ui_rgb(144, 122, 102);
    }
}
