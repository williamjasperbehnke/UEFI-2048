/* Board transition: animated move composition and board-frame caching. */
#include "ui/ui_board_transition.h"

#include <efilib.h>

#include "ui/ui_primitives.h"
#include "ui/ui_tile_atlas.h"
#include "ui/ui_theme.h"
#include "ui/ui_board_transition_cache.h"

static UiBoardTransitionCache *cache(VOID) {
    return ui_board_transition_cache_get();
}

static VOID map_line_index_to_cell(MoveDir dir, UINTN line, UINTN k, UINTN *out_r, UINTN *out_c) {
    if (dir == DIR_LEFT) {
        *out_r = line;
        *out_c = k;
    } else if (dir == DIR_RIGHT) {
        *out_r = line;
        *out_c = BOARD_SIZE - 1 - k;
    } else if (dir == DIR_UP) {
        *out_r = k;
        *out_c = line;
    } else {
        *out_r = BOARD_SIZE - 1 - k;
        *out_c = line;
    }
}

static BOOLEAN same_board(const GameState *a, const UINT32 b[BOARD_SIZE][BOARD_SIZE]) {
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            if (a->cells[r][c] != b[r][c]) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static VOID draw_board_background(UiContext *ctx, const UiGridLayout *layout) {
    ui_fill_rect(
        ctx,
        layout->board_rect.x,
        layout->board_rect.y,
        layout->board_rect.w,
        layout->board_rect.h,
        UI_COLOR_BOARD
    );
}

static VOID blit_sprite_to_buf(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dst,
    UINTN dw,
    UINTN dh,
    INTN dx,
    INTN dy,
    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *src,
    UINTN sw,
    UINTN sh
) {
    if (src == NULL) return;
    for (UINTN y = 0; y < sh; ++y) {
        INTN ty = dy + (INTN)y;
        if (ty < 0 || (UINTN)ty >= dh) continue;
        for (UINTN x = 0; x < sw; ++x) {
            INTN tx = dx + (INTN)x;
            if (tx < 0 || (UINTN)tx >= dw) continue;
            dst[(UINTN)ty * dw + (UINTN)tx] = src[y * sw + x];
        }
    }
}

static VOID blit_sprite_scaled_to_buf(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dst,
    UINTN dw,
    UINTN dh,
    INTN dx,
    INTN dy,
    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *src,
    UINTN sw,
    UINTN sh,
    UINTN scaled_w,
    UINTN scaled_h
) {
    if (src == NULL || scaled_w == 0 || scaled_h == 0) return;
    for (UINTN y = 0; y < scaled_h; ++y) {
        INTN ty = dy + (INTN)y;
        if (ty < 0 || (UINTN)ty >= dh) continue;
        UINTN sy = (y * sh) / scaled_h;
        for (UINTN x = 0; x < scaled_w; ++x) {
            INTN tx = dx + (INTN)x;
            if (tx < 0 || (UINTN)tx >= dw) continue;
            UINTN sx = (x * sw) / scaled_w;
            dst[(UINTN)ty * dw + (UINTN)tx] = src[sy * sw + sx];
        }
    }
}

static VOID learn_visible_tile_sprites(UiContext *ctx, const UiGridLayout *layout, const GameState *game) {
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            ui_tile_atlas_learn_from_cell(ctx, layout, r, c, game->cells[r][c]);
        }
    }
}

static BOOLEAN prepare_board_frame(
    UiContext *ctx,
    const UiGridLayout *layout,
    const GameState *game
) {
    UiBoardTransitionCache *state = cache();
    ui_tile_atlas_ensure_size(ctx->system_table, layout->tile);
    if (!ui_board_transition_cache_ensure_board_buffers(ctx, layout)) {
        draw_board_background(ctx, layout);
        return FALSE;
    }

    learn_visible_tile_sprites(ctx, layout, game);
    CopyMem(
        state->board_frame_buffer,
        state->board_base_buffer,
        state->board_buf_w * state->board_buf_h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    );
    return TRUE;
}

static VOID rebuild_anim_cache(const GameState *from_game, MoveDir dir) {
    UiBoardTransitionCache *state = cache();
    ui_board_transition_cache_reset_anim();

    for (UINTN line = 0; line < BOARD_SIZE; ++line) {
        UINT32 vals[BOARD_SIZE] = {0, 0, 0, 0};
        UINTN src_r[BOARD_SIZE] = {0, 0, 0, 0};
        UINTN src_c[BOARD_SIZE] = {0, 0, 0, 0};
        UINTN count = 0;
        UINTN write = 0;
        UINTN i = 0;

        for (UINTN k = 0; k < BOARD_SIZE; ++k) {
            UINTN r = 0;
            UINTN c = 0;
            map_line_index_to_cell(dir, line, k, &r, &c);

            if (from_game->cells[r][c] != 0) {
                vals[count] = from_game->cells[r][c];
                src_r[count] = r;
                src_c[count] = c;
                count++;
            }
        }

        while (i < count) {
            UINTN dst_r = 0;
            UINTN dst_c = 0;
            map_line_index_to_cell(dir, line, write, &dst_r, &dst_c);

            if (i + 1 < count && vals[i] == vals[i + 1]) {
                state->anim_tiles[state->anim_tile_count++] = (UiAnimTile){vals[i], src_r[i], src_c[i], dst_r, dst_c, TRUE};
                state->anim_tiles[state->anim_tile_count++] = (UiAnimTile){vals[i + 1], src_r[i + 1], src_c[i + 1], dst_r, dst_c, TRUE};
                i += 2;
            } else {
                BOOLEAN moved = (src_r[i] != dst_r) || (src_c[i] != dst_c);
                state->anim_tiles[state->anim_tile_count++] = (UiAnimTile){vals[i], src_r[i], src_c[i], dst_r, dst_c, moved};
                i += 1;
            }
            write++;
        }
    }

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            state->anim_board[r][c] = from_game->cells[r][c];
        }
    }
    state->anim_dir = dir;
    state->anim_valid = TRUE;
}

static VOID draw_tile_fallback(UiContext *ctx, UINT32 value, UINTN x, UINTN y, UINTN w, UINTN h) {
    ui_fill_rect(ctx, x, y, w, h, ui_tile_color(value));
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL text = (value <= 4) ? UI_COLOR_TEXT_TILE_DARK : UI_COLOR_TEXT_LIGHT;
    ui_draw_number_centered(ctx, (UINTN)value, x, y, w, h, text);
}

static VOID draw_tile_to_board_buffer(
    UiContext *ctx,
    const UiGridLayout *layout,
    UINT32 value,
    UINTN x,
    UINTN y,
    UINTN w,
    UINTN h
) {
    UiBoardTransitionCache *state = cache();
    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *sprite = ui_tile_atlas_sprite(value);
    if (sprite != NULL) {
        blit_sprite_to_buf(
            state->board_frame_buffer,
            state->board_buf_w,
            state->board_buf_h,
            (INTN)x - (INTN)layout->board_rect.x,
            (INTN)y - (INTN)layout->board_rect.y,
            sprite,
            layout->tile,
            layout->tile
        );
        return;
    }

    draw_tile_fallback(ctx, value, x, y, w, h);
}

static VOID draw_tile_scaled_to_board_buffer(
    UiContext *ctx,
    const UiGridLayout *layout,
    UINT32 value,
    UINTN cell_x,
    UINTN cell_y,
    UINTN cell_w,
    UINTN cell_h,
    UINTN scale_percent
) {
    UiBoardTransitionCache *state = cache();
    UINTN draw_w = (cell_w * scale_percent) / 100;
    UINTN draw_h = (cell_h * scale_percent) / 100;
    if (draw_w == 0) draw_w = 1;
    if (draw_h == 0) draw_h = 1;
    INTN draw_x = (INTN)cell_x + ((INTN)cell_w - (INTN)draw_w) / 2;
    INTN draw_y = (INTN)cell_y + ((INTN)cell_h - (INTN)draw_h) / 2;

    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *sprite = ui_tile_atlas_sprite(value);
    if (sprite != NULL) {
        blit_sprite_scaled_to_buf(
            state->board_frame_buffer,
            state->board_buf_w,
            state->board_buf_h,
            draw_x - (INTN)layout->board_rect.x,
            draw_y - (INTN)layout->board_rect.y,
            sprite,
            layout->tile,
            layout->tile,
            draw_w,
            draw_h
        );
        return;
    }

    draw_tile_fallback(ctx, value, (UINTN)draw_x, (UINTN)draw_y, draw_w, draw_h);
}

static VOID compose_stationary_tiles(UiContext *ctx, const UiGridLayout *layout) {
    UiBoardTransitionCache *state = cache();
    for (UINTN idx = 0; idx < state->anim_tile_count; ++idx) {
        if (state->anim_tiles[idx].moved) {
            continue;
        }

        UiRect cell = ui_grid_cell_rect(layout, state->anim_tiles[idx].to_r, state->anim_tiles[idx].to_c);
        draw_tile_to_board_buffer(ctx, layout, state->anim_tiles[idx].value, cell.x, cell.y, cell.w, cell.h);
    }
}

static VOID compose_moving_tiles(UiContext *ctx, const UiGridLayout *layout, UINTN progress_numer, UINTN progress_denom) {
    UiBoardTransitionCache *state = cache();
    for (UINTN idx = 0; idx < state->anim_tile_count; ++idx) {
        if (!state->anim_tiles[idx].moved) {
            continue;
        }

        UiRect from_cell = ui_grid_cell_rect(layout, state->anim_tiles[idx].from_r, state->anim_tiles[idx].from_c);
        UiRect to_cell = ui_grid_cell_rect(layout, state->anim_tiles[idx].to_r, state->anim_tiles[idx].to_c);
        INTN x = (INTN)from_cell.x + (((INTN)to_cell.x - (INTN)from_cell.x) * (INTN)progress_numer) / (INTN)progress_denom;
        INTN y = (INTN)from_cell.y + (((INTN)to_cell.y - (INTN)from_cell.y) * (INTN)progress_numer) / (INTN)progress_denom;

        draw_tile_to_board_buffer(
            ctx,
            layout,
            state->anim_tiles[idx].value,
            (UINTN)x,
            (UINTN)y,
            from_cell.w,
            from_cell.h
        );
    }
}

static VOID present_transition_frame(UiContext *ctx, const UiGridLayout *layout) {
    UiBoardTransitionCache *state = cache();
    state->last_board_x = layout->board_rect.x;
    state->last_board_y = layout->board_rect.y;
    state->last_board_w = layout->board_rect.w;
    state->last_board_h = layout->board_rect.h;

    if (!state->auto_present) {
        return;
    }

    uefi_call_wrapper(
        ctx->gop->Blt,
        10,
        ctx->gop,
        state->board_frame_buffer,
        EfiBltBufferToVideo,
        0,
        0,
        layout->board_rect.x,
        layout->board_rect.y,
        layout->board_rect.w,
        layout->board_rect.h,
        0
    );
}

VOID ui_prime_board_tile_sprites(UiContext *ctx) {
    static const UINT32 kPrimeValues[] = {
        2, 4, 8, 16, 32, 64, 128, 256,
        512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
        131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608
    };

    UiGridLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, UI_BOARD_MIN_GAP);
    ui_tile_atlas_ensure_size(ctx->system_table, layout.tile);

    UiRect scratch = ui_grid_cell_rect(&layout, 0, 0);
    for (UINTN i = 0; i < sizeof(kPrimeValues) / sizeof(kPrimeValues[0]); ++i) {
        UINT32 value = kPrimeValues[i];
        if (ui_tile_atlas_sprite(value) != NULL) {
            continue;
        }

        ui_fill_rect(ctx, scratch.x, scratch.y, scratch.w, scratch.h, ui_tile_color(value));
        ui_draw_number_centered(
            ctx,
            (UINTN)value,
            scratch.x,
            scratch.y,
            scratch.w,
            scratch.h,
            (value <= 4) ? UI_COLOR_TEXT_TILE_DARK : UI_COLOR_TEXT_LIGHT
        );

        ui_tile_atlas_learn_from_cell(ctx, &layout, 0, 0, value);
    }
}

VOID ui_render_board_transition_scene(
    UiContext *ctx,
    const GameState *from_game,
    MoveDir dir,
    UINTN progress_numer,
    UINTN progress_denom
) {
    UiBoardTransitionCache *state = cache();
    if (progress_denom == 0) {
        progress_denom = 1;
    }
    if (progress_numer > progress_denom) {
        progress_numer = progress_denom;
    }

    UiGridLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, UI_BOARD_MIN_GAP);
    if (!prepare_board_frame(ctx, &layout, from_game)) {
        return;
    }

    BOOLEAN need_rebuild = !state->anim_valid
        || (state->anim_dir != dir)
        || !same_board(from_game, state->anim_board);

    if (need_rebuild) {
        rebuild_anim_cache(from_game, dir);
    }

    compose_stationary_tiles(ctx, &layout);
    compose_moving_tiles(ctx, &layout, progress_numer, progress_denom);
    present_transition_frame(ctx, &layout);
}

VOID ui_render_board_pop_scene(
    UiContext *ctx,
    const GameState *game,
    const BOOLEAN pop_mask[BOARD_SIZE][BOARD_SIZE],
    UINTN scale_percent
) {
    UiGridLayout layout = ui_make_grid_layout(ctx, UI_BOARD_SCALE_PERCENT, UI_BOARD_MIN_GAP);
    if (!prepare_board_frame(ctx, &layout, game)) {
        return;
    }

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            UINT32 value = game->cells[r][c];
            if (value == 0) {
                continue;
            }

            UiRect cell = ui_grid_cell_rect(&layout, r, c);
            if (pop_mask[r][c]) {
                draw_tile_scaled_to_board_buffer(ctx, &layout, value, cell.x, cell.y, cell.w, cell.h, scale_percent);
            } else {
                draw_tile_to_board_buffer(ctx, &layout, value, cell.x, cell.y, cell.w, cell.h);
            }
        }
    }

    present_transition_frame(ctx, &layout);
}

VOID ui_set_transition_auto_present(BOOLEAN enabled) {
    UiBoardTransitionCache *state = cache();
    state->auto_present = enabled;
}

BOOLEAN ui_get_last_transition_frame(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL **out_buf,
    UINTN *out_x,
    UINTN *out_y,
    UINTN *out_w,
    UINTN *out_h
) {
    UiBoardTransitionCache *state = cache();
    if (state->board_frame_buffer == NULL || state->last_board_w == 0 || state->last_board_h == 0) {
        return FALSE;
    }

    *out_buf = state->board_frame_buffer;
    *out_x = state->last_board_x;
    *out_y = state->last_board_y;
    *out_w = state->last_board_w;
    *out_h = state->last_board_h;
    return TRUE;
}
