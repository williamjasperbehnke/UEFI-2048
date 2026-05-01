/* UI primitives: bitmap font, 7-segment digits, and shared color palette. */
#include "ui_primitives.h"
#include <efilib.h>

#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40

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

static UINT8 digit_segments(UINTN d) {
    static const UINT8 map[10] = {
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
        SEG_B | SEG_C,
        SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
        SEG_B | SEG_C | SEG_F | SEG_G,
        SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
        SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G
    };
    return map[d % 10];
}

static VOID draw_digit7(UiContext *ctx, UINTN x, UINTN y, UINTN dw, UINTN dh, UINTN digit, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    UINT8 seg = digit_segments(digit);
    UINTN t = dw / 6;
    if (t < 2) t = 2;
    UINTN mid = y + dh / 2 - t / 2;

    if (seg & SEG_A) ui_fill_rect(ctx, x + t, y, dw - 2 * t, t, color);
    if (seg & SEG_D) ui_fill_rect(ctx, x + t, y + dh - t, dw - 2 * t, t, color);
    if (seg & SEG_G) ui_fill_rect(ctx, x + t, mid, dw - 2 * t, t, color);
    if (seg & SEG_F) ui_fill_rect(ctx, x, y + t, t, dh / 2 - t, color);
    if (seg & SEG_B) ui_fill_rect(ctx, x + dw - t, y + t, t, dh / 2 - t, color);
    if (seg & SEG_E) ui_fill_rect(ctx, x, mid + t, t, dh / 2 - t, color);
    if (seg & SEG_C) ui_fill_rect(ctx, x + dw - t, mid + t, t, dh / 2 - t, color);
}

VOID ui_draw_number_centered(UiContext *ctx, UINTN value, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    UINTN digits[10];
    UINTN count = 0;

    if (value == 0) {
        digits[count++] = 0;
    } else {
        while (value > 0 && count < 10) {
            digits[count++] = value % 10;
            value /= 10;
        }
    }

    UINTN digit_w = w / 6;
    UINTN digit_h = h * 2 / 3;
    UINTN spacing = digit_w / 4;
    UINTN total_w = count * digit_w + (count - 1) * spacing;

    UINTN ox = x + (w - total_w) / 2;
    UINTN oy = y + (h - digit_h) / 2;

    for (UINTN i = 0; i < count; ++i) {
        UINTN d = digits[count - 1 - i];
        draw_digit7(ctx, ox + i * (digit_w + spacing), oy, digit_w, digit_h, d, color);
    }
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
