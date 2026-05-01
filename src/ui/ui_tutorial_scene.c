/* Tutorial scene: data-driven frame model and frame-specific composition. */
#include "ui_tutorial_scene.h"

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
        UI_COLOR_BOARD
    );
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            fill_grid_tile(ctx, layout, r, c, ui_tile_color(0));
        }
    }
}

static VOID draw_arrow(UiContext *ctx, UINTN x, UINTN y, UINTN w, UINTN h, BOOLEAN points_left) {
    if (w < UI_TUTORIAL_ARROW_MIN_W || h < UI_TUTORIAL_ARROW_MIN_H) return;

    UINTN thickness = h / 3;
    if (thickness < 4) thickness = 4;
    UINTN head_w = UI_TUTORIAL_ARROW_HEAD_WIDTH;
    if (head_w > w / 2) head_w = w / 2;
    UINTN head_join_overlap = 4;
    UINTN shaft_w = w - head_w + head_join_overlap;

    UINTN sy = y + (h - thickness) / 2;
    UINTN shaft_x = points_left ? (x + head_w - head_join_overlap) : x;
    ui_fill_rect(ctx, shaft_x, sy, shaft_w, thickness, UI_COLOR_ARROW_OUTLINE);
    if (shaft_w > 4 && thickness > 4) {
        ui_fill_rect(
            ctx,
            shaft_x + UI_TUTORIAL_ARROW_INNER_INSET,
            sy + UI_TUTORIAL_ARROW_INNER_INSET,
            shaft_w - (2 * UI_TUTORIAL_ARROW_INNER_INSET),
            thickness - (2 * UI_TUTORIAL_ARROW_INNER_INSET),
            UI_COLOR_ARROW_FILL
        );
    }

    /* Build an isosceles triangle head by rasterizing per scanline. */
    UINTN half = h / 2;
    if (half == 0) half = 1;
    for (UINTN yy = 0; yy < h; ++yy) {
        UINTN dist = (yy > half) ? (yy - half) : (half - yy);
        UINTN row_w = head_w - (head_w * dist) / half;
        if (row_w == 0) row_w = 1;
        UINTN row_x = points_left ? (x + head_w - row_w) : (x + shaft_w - head_join_overlap);
        ui_fill_rect(ctx, row_x, y + yy, row_w, 1, UI_COLOR_ARROW_OUTLINE);
        if (row_w > 4) {
            ui_fill_rect(
                ctx,
                row_x + UI_TUTORIAL_ARROW_INNER_INSET,
                y + yy,
                row_w - (2 * UI_TUTORIAL_ARROW_INNER_INSET),
                1,
                UI_COLOR_ARROW_FILL
            );
        }
    }
}

static VOID render_frame_move(UiContext *ctx, const TutorialLayout *layout) {
    UINTN arrow_h = layout->tile / 2;
    if (arrow_h < UI_TUTORIAL_MOVE_ARROW_MIN_H) arrow_h = UI_TUTORIAL_MOVE_ARROW_MIN_H;

    for (UINTN r = 0; r < 3; ++r) {
        UINTN ty = grid_y(layout, r);
        fill_grid_tile(ctx, layout, r, 0, UI_COLOR_TILE_BLOCKED);

        UINTN value = (r == 1) ? 4 : 2;
        UINTN value_col = (r == 1) ? 2 : 3;
        UINTN vx = grid_x(layout, value_col);

        ui_fill_rect(ctx, vx, ty, layout->tile, layout->tile, ui_tile_color((UINT32)value));
        ui_draw_number_centered(ctx, value, vx, ty, layout->tile, layout->tile, UI_COLOR_TEXT_DARK);

        UINTN arrow_x = layout->board_rect.x + layout->gap + layout->tile + UI_TUTORIAL_MOVE_ARROW_START_PAD;
        UINTN arrow_w = value_col * (layout->tile + layout->gap) - layout->tile - UI_TUTORIAL_MOVE_ARROW_START_PAD;
        draw_arrow(ctx, arrow_x, ty + (layout->tile - arrow_h) / 2, arrow_w, arrow_h, TRUE);
    }
}

static VOID draw_plus(UiContext *ctx, UINTN cx, UINTN cy, UINTN size) {
    UINTN t = size / 4;
    if (t < 3) t = 3;

    ui_fill_rect(ctx, cx - t / 2, cy - size / 2, t, size, UI_COLOR_ARROW_OUTLINE);
    ui_fill_rect(ctx, cx - size / 2, cy - t / 2, size, t, UI_COLOR_ARROW_OUTLINE);

    if (t > 4 && size > 4) {
        ui_fill_rect(ctx, cx - t / 2 + 2, cy - size / 2 + 2, t - 4, size - 4, UI_COLOR_ARROW_FILL);
        ui_fill_rect(ctx, cx - size / 2 + 2, cy - t / 2 + 2, size - 4, t - 4, UI_COLOR_ARROW_FILL);
    }
}

static VOID draw_equals(UiContext *ctx, UINTN cx, UINTN cy, UINTN w, UINTN h) {
    UINTN bar_h = h / 3;
    if (bar_h < 4) bar_h = 4;
    UINTN gap = h / 5;
    if (gap < 3) gap = 3;

    UINTN top_y = cy - gap - bar_h;
    UINTN bot_y = cy + gap;
    ui_fill_rect(ctx, cx - w / 2, top_y, w, bar_h, UI_COLOR_ARROW_OUTLINE);
    ui_fill_rect(ctx, cx - w / 2, bot_y, w, bar_h, UI_COLOR_ARROW_OUTLINE);
    if (w > 4 && bar_h > 4) {
        ui_fill_rect(ctx, cx - w / 2 + 2, top_y + 2, w - 4, bar_h - 4, UI_COLOR_ARROW_FILL);
        ui_fill_rect(ctx, cx - w / 2 + 2, bot_y + 2, w - 4, bar_h - 4, UI_COLOR_ARROW_FILL);
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

    draw_merge_row(ctx, layout, y1, 2, 2, 4, UI_COLOR_TEXT_DARK, UI_COLOR_TEXT_DARK);
    draw_merge_row(ctx, layout, y2, 8, 8, 16, UI_COLOR_TEXT_LIGHT, UI_COLOR_TEXT_LIGHT);

    UINTN arrow_h = (layout->tile / 2) * 3 / 4;
    if (arrow_h < UI_TUTORIAL_MERGE_ARROW_MIN_H) arrow_h = UI_TUTORIAL_MERGE_ARROW_MIN_H;
    UINTN arrow_w = ((3 * (layout->tile + layout->gap) - layout->tile / 3) * 2) / 3;
    if (arrow_w < UI_TUTORIAL_MERGE_ARROW_MIN_W) arrow_w = UI_TUTORIAL_MERGE_ARROW_MIN_W;
    UINTN arrow_x = layout->board_rect.x + layout->gap + layout->tile + UI_TUTORIAL_MERGE_ARROW_X_OFFSET;

    draw_arrow(
        ctx,
        arrow_x,
        y1 + UI_TUTORIAL_MERGE_ARROW_Y_OFFSET + (layout->tile - arrow_h) / 2 + layout->tile / 3,
        arrow_w,
        arrow_h,
        FALSE
    );
    draw_arrow(
        ctx,
        arrow_x,
        y2 + UI_TUTORIAL_MERGE_ARROW_Y_OFFSET + (layout->tile - arrow_h) / 2 + layout->tile / 3,
        arrow_w,
        arrow_h,
        FALSE
    );
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
                ui_draw_number_centered(ctx, v, tx, ty, layout->tile, layout->tile, (v <= 4) ? UI_COLOR_TEXT_DARK : UI_COLOR_TEXT_LIGHT);
            }
        }
    }
}

VOID ui_render_tutorial_scene(UiContext *ctx, UINTN frame_index) {
    TutorialLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, 4);

    static const TutorialFrame frames[] = {
        {L"ARROW KEYS OR WASD TO MOVE ALL TILES", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_MOVE, render_frame_move},
        {L"WHEN TWO TILES WITH SAME NUMBER TOUCH THEY MERGE", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_MERGE, render_frame_merge},
        {L"JOIN THE NUMBERS AND GET TO THE 2048 TILE!", 0, UI_TUTORIAL_TITLE_Y, UI_TUTORIAL_TITLE_SCALE_GOAL, render_frame_goal}
    };

    const TutorialFrame *frame = &frames[frame_index % 3];

    ui_fill_rect(ctx, 0, 0, ctx->screen_w, ctx->screen_h, UI_COLOR_BG);
    draw_tutorial_grid(ctx, &layout);

    UINTN title_x = frame->title_x;
    if (title_x == 0) {
        title_x = (ctx->screen_w - ui_text_width(frame->title, frame->title_scale)) / 2;
    }
    ui_draw_text(ctx, frame->title, title_x, frame->title_y, frame->title_scale, UI_COLOR_TEXT_ACCENT);

    frame->render_fn(ctx, &layout);

    {
        const CHAR16 *footer = L"PRESS ANY KEY";
        UINTN footer_x = (ctx->screen_w - ui_text_width(footer, UI_TUTORIAL_FOOTER_SCALE)) / 2;
        ui_draw_text(
            ctx,
            footer,
            footer_x,
            ctx->screen_h - UI_TUTORIAL_FOOTER_Y_OFFSET,
            UI_TUTORIAL_FOOTER_SCALE,
            UI_COLOR_TEXT_TITLE
        );
    }
}
