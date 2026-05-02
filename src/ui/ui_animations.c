#include "ui/ui_animations.h"

#include <efilib.h>

#include "ui/ui_board_transition.h"
#include "ui/ui_context.h"

static BOOLEAN ensure_full_buffers(UiAnimationState *state, UiContext *ctx) {
    if (state->full_base != NULL && state->full_w == ctx->screen_w && state->full_h == ctx->screen_h) {
        return TRUE;
    }

    if (state->full_base != NULL) {
        uefi_call_wrapper(ctx->system_table->BootServices->FreePool, 1, state->full_base);
        state->full_base = NULL;
    }
    if (state->full_frame != NULL) {
        uefi_call_wrapper(ctx->system_table->BootServices->FreePool, 1, state->full_frame);
        state->full_frame = NULL;
    }

    UINTN bytes = ctx->screen_w * ctx->screen_h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    EFI_STATUS st = uefi_call_wrapper(
        ctx->system_table->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        bytes,
        (VOID **)&state->full_base
    );
    if (EFI_ERROR(st) || state->full_base == NULL) {
        state->full_base = NULL;
        return FALSE;
    }

    st = uefi_call_wrapper(
        ctx->system_table->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        bytes,
        (VOID **)&state->full_frame
    );
    if (EFI_ERROR(st) || state->full_frame == NULL) {
        uefi_call_wrapper(ctx->system_table->BootServices->FreePool, 1, state->full_base);
        state->full_base = NULL;
        state->full_frame = NULL;
        return FALSE;
    }

    state->full_w = ctx->screen_w;
    state->full_h = ctx->screen_h;
    return TRUE;
}

static VOID overlay_board_into_full(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *full,
    UINTN full_w,
    UINTN full_h,
    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *board,
    UINTN board_x,
    UINTN board_y,
    UINTN board_w,
    UINTN board_h
) {
    if (board_x >= full_w || board_y >= full_h) {
        return;
    }

    UINTN copy_w = board_w;
    if (board_x + copy_w > full_w) {
        copy_w = full_w - board_x;
    }

    for (UINTN y = 0; y < board_h; ++y) {
        UINTN ty = board_y + y;
        if (ty >= full_h) continue;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dst = &full[ty * full_w + board_x];
        const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *src = &board[y * board_w];
        CopyMem(dst, src, copy_w * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }
}

VOID ui_animations_init(UiAnimationState *state) {
    state->full_base = NULL;
    state->full_frame = NULL;
    state->full_w = 0;
    state->full_h = 0;
    state->config.slide_enabled = TRUE;
    state->config.move_frames = 3;
    state->config.move_stall_us = 20000;
}

VOID ui_animations_prime(EFI_SYSTEM_TABLE *system_table) {
    UiContext ctx;
    if (!ui_ctx_begin(&ctx, system_table)) {
        return;
    }
    ui_prime_board_tile_sprites(&ctx);
}

VOID ui_animations_run_move(UiAnimationState *state, EFI_SYSTEM_TABLE *system_table, const GameState *before, const GameState *after, MoveDir dir) {
    UiContext ctx;
    BOOLEAN any_delta = FALSE;

    if (!state->config.slide_enabled || state->config.move_frames == 0) {
        return;
    }
    if (!ui_ctx_begin(&ctx, system_table)) {
        return;
    }

    for (UINTN r = 0; r < BOARD_SIZE && !any_delta; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            if (before->cells[r][c] != after->cells[r][c]) {
                any_delta = TRUE;
                break;
            }
        }
    }
    if (!any_delta) {
        return;
    }

    if (!ensure_full_buffers(state, &ctx)) {
        for (UINTN frame = 0; frame < state->config.move_frames; ++frame) {
            ui_render_board_transition_scene(&ctx, before, dir, frame + 1, state->config.move_frames);
            uefi_call_wrapper(system_table->BootServices->Stall, 1, state->config.move_stall_us);
        }
        return;
    }

    uefi_call_wrapper(
        ctx.gop->Blt,
        10,
        ctx.gop,
        state->full_base,
        EfiBltVideoToBltBuffer,
        0,
        0,
        0,
        0,
        ctx.screen_w,
        ctx.screen_h,
        0
    );

    ui_set_transition_auto_present(FALSE);
    for (UINTN frame = 0; frame < state->config.move_frames; ++frame) {
        ui_render_board_transition_scene(&ctx, before, dir, frame + 1, state->config.move_frames);

        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *board = NULL;
        UINTN bx = 0, by = 0, bw = 0, bh = 0;
        if (ui_get_last_transition_frame(&board, &bx, &by, &bw, &bh)) {
            CopyMem(state->full_frame, state->full_base, ctx.screen_w * ctx.screen_h * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            overlay_board_into_full(state->full_frame, ctx.screen_w, ctx.screen_h, board, bx, by, bw, bh);
            uefi_call_wrapper(
                ctx.gop->Blt,
                10,
                ctx.gop,
                state->full_frame,
                EfiBltBufferToVideo,
                0,
                0,
                0,
                0,
                ctx.screen_w,
                ctx.screen_h,
                0
            );
        }
        uefi_call_wrapper(system_table->BootServices->Stall, 1, state->config.move_stall_us);
    }
    ui_set_transition_auto_present(TRUE);
}
