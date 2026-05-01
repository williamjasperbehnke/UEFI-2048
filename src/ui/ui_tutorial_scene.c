/* Tutorial scene: data-driven frame model and frame-specific composition. */
#include "ui_tutorial_scene.h"
#include <efilib.h>

#include "ui_primitives.h"
#include "ui_theme.h"

typedef UiGridLayout TutorialLayout;

typedef struct {
    const CHAR16 *title;
    UINTN title_x;
    UINTN title_y;
    UINTN title_scale;
    VOID (*render_fn)(UiContext *ctx, const TutorialLayout *layout);
} TutorialFrame;

static UINTN grid_x(const TutorialLayout *layout, UINTN col) {
    return layout->board_rect.x + layout->gap + col * (layout->tile + layout->gap);
}

static UINTN grid_y(const TutorialLayout *layout, UINTN row) {
    return layout->board_rect.y + layout->gap + row * (layout->tile + layout->gap);
}

static VOID fill_grid_tile(
    UiContext *ctx,
    const TutorialLayout *layout,
    UINTN row,
    UINTN col,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color
) {
    ui_fill_rect(ctx, grid_x(layout, col), grid_y(layout, row), layout->tile, layout->tile, color);
}

static VOID draw_tutorial_grid(UiContext *ctx, const TutorialLayout *layout) {
    ui_fill_rect(
        ctx,
        layout->board_rect.x,
        layout->board_rect.y,
        layout->board_rect.w,
        layout->board_rect.h,
        ui_rgb(187, 173, 160)
    );
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            fill_grid_tile(ctx, layout, r, c, ui_tile_color(0));
        }
    }
}

static VOID draw_arrow(UiContext *ctx, UINTN x, UINTN y, UINTN w, UINTN h, BOOLEAN points_left) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color = ui_rgb(235, 170, 10);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL outline = ui_rgb(162, 122, 16);
    if (w < UI_TUTORIAL_ARROW_MIN_W || h < UI_TUTORIAL_ARROW_MIN_H) return;

    UINTN thickness = h / 3;
    if (thickness < 4) thickness = 4;
    UINTN head_w = UI_TUTORIAL_ARROW_HEAD_WIDTH;
    if (head_w > w / 2) head_w = w / 2;
    UINTN shaft_w = w - head_w;

    UINTN sy = y + (h - thickness) / 2;
    UINTN shaft_x = points_left ? (x + head_w) : x;
    ui_fill_rect(ctx, shaft_x, sy, shaft_w, thickness, outline);
    if (shaft_w > 4 && thickness > 4) {
        ui_fill_rect(ctx, shaft_x + 2, sy + 2, shaft_w - 4, thickness - 4, color);
    }

    /* Build an isosceles triangle head by rasterizing per scanline. */
    UINTN half = h / 2;
    if (half == 0) half = 1;
    for (UINTN yy = 0; yy < h; ++yy) {
        UINTN dist = (yy > half) ? (yy - half) : (half - yy);
        UINTN row_w = head_w - (head_w * dist) / half;
        if (row_w == 0) row_w = 1;
        UINTN row_x = points_left ? (x + head_w - row_w) : (x + shaft_w);
        ui_fill_rect(ctx, row_x, y + yy, row_w, 1, outline);
        if (row_w > 4) {
            ui_fill_rect(ctx, row_x + 2, y + yy, row_w - 4, 1, color);
        }
    }
}

static VOID render_frame_move(UiContext *ctx, const TutorialLayout *layout) {
    UINTN arrow_h = layout->tile / 2;
    if (arrow_h < 18) arrow_h = 18;

    for (UINTN r = 0; r < 3; ++r) {
        UINTN ty = grid_y(layout, r);
        fill_grid_tile(ctx, layout, r, 0, ui_rgb(236, 196, 74));

        UINTN value = (r == 1) ? 4 : 2;
        UINTN value_col = (r == 1) ? 2 : 3;
        UINTN vx = grid_x(layout, value_col);

        ui_fill_rect(ctx, vx, ty, layout->tile, layout->tile, ui_tile_color((UINT32)value));
        ui_draw_number_centered(ctx, value, vx, ty, layout->tile, layout->tile, ui_rgb(90, 80, 70));

        UINTN arrow_x = layout->board_rect.x + layout->gap + layout->tile + 8;
        UINTN arrow_w = value_col * (layout->tile + layout->gap) - layout->tile - 8;
        draw_arrow(ctx, arrow_x, ty + (layout->tile - arrow_h) / 2, arrow_w, arrow_h, TRUE);
    }
}

static VOID draw_plus(UiContext *ctx, UINTN cx, UINTN cy, UINTN size) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color = ui_rgb(235, 170, 10);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL outline = ui_rgb(162, 122, 16);
    UINTN t = size / 4;
    if (t < 3) t = 3;

    ui_fill_rect(ctx, cx - t / 2, cy - size / 2, t, size, outline);
    ui_fill_rect(ctx, cx - size / 2, cy - t / 2, size, t, outline);

    if (t > 4 && size > 4) {
        ui_fill_rect(ctx, cx - t / 2 + 2, cy - size / 2 + 2, t - 4, size - 4, color);
        ui_fill_rect(ctx, cx - size / 2 + 2, cy - t / 2 + 2, size - 4, t - 4, color);
    }
}

static VOID draw_equals(UiContext *ctx, UINTN cx, UINTN cy, UINTN w, UINTN h) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color = ui_rgb(235, 170, 10);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL outline = ui_rgb(162, 122, 16);
    UINTN bar_h = h / 3;
    if (bar_h < 4) bar_h = 4;
    UINTN gap = h / 5;
    if (gap < 3) gap = 3;

    UINTN top_y = cy - gap - bar_h;
    UINTN bot_y = cy + gap;
    ui_fill_rect(ctx, cx - w / 2, top_y, w, bar_h, outline);
    ui_fill_rect(ctx, cx - w / 2, bot_y, w, bar_h, outline);
    if (w > 4 && bar_h > 4) {
        ui_fill_rect(ctx, cx - w / 2 + 2, top_y + 2, w - 4, bar_h - 4, color);
        ui_fill_rect(ctx, cx - w / 2 + 2, bot_y + 2, w - 4, bar_h - 4, color);
    }
}

static VOID draw_merge_row(UiContext *ctx, const TutorialLayout *layout, UINTN y, UINTN a, UINTN b, UINTN out, EFI_GRAPHICS_OUTPUT_BLT_PIXEL in_text, EFI_GRAPHICS_OUTPUT_BLT_PIXEL out_text) {
    UINTN x1 = grid_x(layout, 1);
    UINTN x2 = grid_x(layout, 2);
    UINTN x3 = grid_x(layout, 3);

    ui_fill_rect(ctx, x1, y, layout->tile, layout->tile, ui_tile_color((UINT32)a));
    ui_fill_rect(ctx, x2, y, layout->tile, layout->tile, ui_tile_color((UINT32)b));
    ui_fill_rect(ctx, x3, y, layout->tile, layout->tile, ui_tile_color((UINT32)out));

    ui_draw_number_centered(ctx, a, x1, y, layout->tile, layout->tile, in_text);
    ui_draw_number_centered(ctx, b, x2, y, layout->tile, layout->tile, in_text);
    ui_draw_number_centered(ctx, out, x3, y, layout->tile, layout->tile, out_text);

    UINTN cy = y + layout->tile / 2;
    draw_plus(ctx, (x1 + x2 + layout->tile) / 2, cy, layout->tile / 2);
    draw_equals(ctx, (x2 + x3 + layout->tile) / 2, cy, layout->tile / 2, layout->tile / 3);
}

static VOID render_frame_merge(UiContext *ctx, const TutorialLayout *layout) {
    UINTN y1 = grid_y(layout, 1);
    UINTN y2 = grid_y(layout, 2);

    draw_merge_row(ctx, layout, y1, 2, 2, 4, ui_rgb(90, 80, 70), ui_rgb(90, 80, 70));
    draw_merge_row(ctx, layout, y2, 8, 8, 16, ui_rgb(248, 246, 242), ui_rgb(248, 246, 242));

    UINTN arrow_h = (layout->tile / 2) * 3 / 4;
    if (arrow_h < 12) arrow_h = 12;
    UINTN arrow_w = ((3 * (layout->tile + layout->gap) - layout->tile / 3) * 2) / 3;
    if (arrow_w < 64) arrow_w = 64;
    UINTN arrow_x = layout->board_rect.x + layout->gap + layout->tile + 104;

    draw_arrow(ctx, arrow_x, y1 + 32 + (layout->tile - arrow_h) / 2 + layout->tile / 3, arrow_w, arrow_h, FALSE);
    draw_arrow(ctx, arrow_x, y2 + 32 + (layout->tile - arrow_h) / 2 + layout->tile / 3, arrow_w, arrow_h, FALSE);
}

static VOID render_frame_goal(UiContext *ctx, const TutorialLayout *layout) {
    static const UINT32 sample[4][4] = {
        {0, 2, 4, 0},
        {8, 2048, 32, 2},
        {4, 32, 128, 4},
        {16, 2, 4, 2}
    };

    for (UINTN r = 0; r < 4; ++r) {
        for (UINTN c = 0; c < 4; ++c) {
            UINTN tx = grid_x(layout, c);
            UINTN ty = grid_y(layout, r);
            UINT32 v = sample[r][c];
            if (v != 0) {
                fill_grid_tile(ctx, layout, r, c, ui_tile_color(v));
                ui_draw_number_centered(ctx, v, tx, ty, layout->tile, layout->tile, (v <= 4) ? ui_rgb(90, 80, 70) : ui_rgb(248, 246, 242));
            }
        }
    }

    UINTN btn_w = layout->board_rect.w * 62 / 100;
    UINTN btn_h = layout->tile * 85 / 100;
    UINTN bx = layout->board_rect.x + (layout->board_rect.w - btn_w) / 2;
    UINTN by = layout->board_rect.y + layout->board_rect.h + layout->tile / 2;

    ui_fill_rect(ctx, bx, by, btn_w, btn_h, ui_rgb(187, 173, 160));
    ui_draw_text(ctx, L"LETS PLAY!", bx + btn_w / 2 - 90, by + btn_h / 2 - 14, 4, ui_rgb(248, 246, 242));
}

VOID ui_render_tutorial_scene(UiContext *ctx, UINTN frame_index) {
    TutorialLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, 4);

    static const TutorialFrame frames[] = {
        {L"SWIPE TO MOVE ALL TILES", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_MOVE, render_frame_move},
        {L"WHEN TWO TILES WITH SAME NUMBER TOUCH THEY MERGE", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_MERGE, render_frame_merge},
        {L"JOIN THE NUMBERS AND GET TO THE 2048 TILE!", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_GOAL, render_frame_goal}
    };

    const TutorialFrame *frame = &frames[frame_index % 3];

    ui_fill_rect(ctx, 0, 0, ctx->screen_w, ctx->screen_h, ui_rgb(248, 246, 240));
    draw_tutorial_grid(ctx, &layout);

    UINTN title_x = frame->title_x;
    if (title_x == 0) {
        title_x = (ctx->screen_w / 2) - (StrLen(frame->title) * 6 * frame->title_scale) / 2;
    }
    ui_draw_text(ctx, frame->title, title_x, frame->title_y, frame->title_scale, ui_rgb(240, 170, 20));

    frame->render_fn(ctx, &layout);

    ui_draw_text(ctx, L"PRESS ANY KEY", ctx->screen_w / 2 - 108, ctx->screen_h - 52, 3, ui_rgb(120, 110, 101));
}
