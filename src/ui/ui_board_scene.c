/* Board scene: in-game board, score HUD, and game-over overlay. */
#include "ui_board_scene.h"
#include "ui_primitives.h"
#include "ui_theme.h"

static UINTN ui_count_digits(UINTN value) {
    UINTN digits = 1;
    while (value >= 10) {
        value /= 10;
        ++digits;
    }
    return digits;
}

static VOID draw_board_tiles(UiContext *ctx, const GameState *game) {
    UiGridLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, UI_BOARD_MIN_GAP);
    ui_fill_rect(
        ctx,
        layout.board_rect.x,
        layout.board_rect.y,
        layout.board_rect.w,
        layout.board_rect.h,
        ui_rgb(187, 173, 160)
    );

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            UiRect cell = ui_grid_cell_rect(&layout, r, c);
            UINT32 value = game->cells[r][c];
            ui_fill_rect(ctx, cell.x, cell.y, cell.w, cell.h, ui_tile_color(value));
            if (value != 0) {
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL text = (value <= 4) ? ui_rgb(100, 95, 90) : ui_rgb(248, 246, 242);
                ui_draw_number_centered(ctx, (UINTN)value, cell.x, cell.y, cell.w, cell.h, text);
            }
        }
    }
}

static VOID draw_hud(UiContext *ctx, const GameState *game) {
    const UINTN score_label_scale = 2;
    const UINTN score_value_scale = UI_HUD_SCORE_VALUE_SCALE;
    const UINTN score_value_digits = ui_count_digits(game->score);
    const UINTN score_value_w = score_value_digits * 6 * score_value_scale;
    const UINTN score_label_w = 5 * 6 * score_label_scale; /* "SCORE" is 5 glyphs at 5x7 + 1px spacing. */
    const UINTN score_label_x = UI_HUD_SCORE_BOX_X + (UI_HUD_SCORE_BOX_W - score_label_w) / 2;
    const UINTN score_value_x = UI_HUD_SCORE_BOX_X + (UI_HUD_SCORE_BOX_W - score_value_w) / 2;
    const UINTN score_label_y = UI_HUD_SCORE_BOX_Y + 8;
    const UINTN score_value_y = UI_HUD_SCORE_BOX_Y + 36;

    ui_draw_text(ctx, L"2048", UI_HUD_TITLE_X, UI_HUD_TITLE_Y, 4, ui_rgb(120, 110, 101));
    ui_fill_rect(ctx, UI_HUD_SCORE_BOX_X, UI_HUD_SCORE_BOX_Y, UI_HUD_SCORE_BOX_W, UI_HUD_SCORE_BOX_H, ui_rgb(187, 173, 160));
    ui_draw_text(ctx, L"SCORE", score_label_x, score_label_y, score_label_scale, ui_rgb(238, 228, 218));
    ui_draw_text_number(ctx, game->score, score_value_x, score_value_y, score_value_scale, ui_rgb(248, 246, 242));

    if (game->won) {
        ui_draw_text(ctx, L"YOU WON", ctx->screen_w - 220, 20, 3, ui_rgb(237, 194, 46));
    }
}

static VOID draw_game_over_overlay(UiContext *ctx) {
    UINTN pw = ctx->screen_w * 70 / 100;
    UINTN ph = ctx->screen_h * 36 / 100;
    UINTN px = (ctx->screen_w - pw) / 2;
    UINTN py = (ctx->screen_h - ph) / 2;

    ui_fill_rect(ctx, px, py, pw, ph, ui_rgb(120, 90, 80));
    ui_fill_rect(ctx, px + 6, py + 6, pw - 12, ph - 12, ui_rgb(145, 104, 90));
    ui_draw_text(ctx, L"GAME OVER", px + 40, py + 40, 6, ui_rgb(255, 246, 235));
    ui_draw_text(ctx, L"PRESS R TO RESTART", px + 44, py + ph - 80, 3, ui_rgb(255, 232, 208));
}

VOID ui_render_board_scene(UiContext *ctx, const GameState *game, BOOLEAN is_game_over) {
    ui_fill_rect(ctx, 0, 0, ctx->screen_w, ctx->screen_h, ui_rgb(248, 246, 240));
    draw_board_tiles(ctx, game);
    draw_hud(ctx, game);

    if (is_game_over) {
        draw_game_over_overlay(ctx);
    }
}
