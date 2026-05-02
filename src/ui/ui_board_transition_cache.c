#include "ui/ui_board_transition_cache.h"

#include <efilib.h>

#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"

static UiBoardTransitionCache g_cache = {
    .anim_dir = DIR_LEFT,
    .auto_present = TRUE,
};

UiBoardTransitionCache *ui_board_transition_cache_get(VOID) {
    return &g_cache;
}

VOID ui_board_transition_cache_reset_anim(VOID) {
    g_cache.anim_tile_count = 0;
    g_cache.anim_valid = FALSE;
}

VOID ui_board_transition_cache_free_board_buffers(EFI_SYSTEM_TABLE *system_table) {
    if (g_cache.board_base_buffer != NULL) {
        uefi_call_wrapper(system_table->BootServices->FreePool, 1, g_cache.board_base_buffer);
        g_cache.board_base_buffer = NULL;
    }
    if (g_cache.board_frame_buffer != NULL) {
        uefi_call_wrapper(system_table->BootServices->FreePool, 1, g_cache.board_frame_buffer);
        g_cache.board_frame_buffer = NULL;
    }
    g_cache.board_buf_w = 0;
    g_cache.board_buf_h = 0;
}

static VOID fill_buf_rect(
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buf,
    UINTN bw,
    UINTN bh,
    UINTN x,
    UINTN y,
    UINTN w,
    UINTN h,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL color
) {
    if (x >= bw || y >= bh) return;
    if (x + w > bw) w = bw - x;
    if (y + h > bh) h = bh - y;

    for (UINTN yy = 0; yy < h; ++yy) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *row = &buf[(y + yy) * bw + x];
        for (UINTN xx = 0; xx < w; ++xx) {
            row[xx] = color;
        }
    }
}

BOOLEAN ui_board_transition_cache_ensure_board_buffers(UiContext *ctx, const UiGridLayout *layout) {
    if (g_cache.board_base_buffer != NULL
        && g_cache.board_buf_w == layout->board_rect.w
        && g_cache.board_buf_h == layout->board_rect.h) {
        return TRUE;
    }

    ui_board_transition_cache_free_board_buffers(ctx->system_table);
    UINTN pixels = layout->board_rect.w * layout->board_rect.h;
    UINTN bytes = pixels * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    EFI_STATUS st = uefi_call_wrapper(
        ctx->system_table->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        bytes,
        (VOID **)&g_cache.board_base_buffer
    );
    if (EFI_ERROR(st) || g_cache.board_base_buffer == NULL) {
        g_cache.board_base_buffer = NULL;
        return FALSE;
    }

    st = uefi_call_wrapper(
        ctx->system_table->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        bytes,
        (VOID **)&g_cache.board_frame_buffer
    );
    if (EFI_ERROR(st) || g_cache.board_frame_buffer == NULL) {
        ui_board_transition_cache_free_board_buffers(ctx->system_table);
        return FALSE;
    }

    g_cache.board_buf_w = layout->board_rect.w;
    g_cache.board_buf_h = layout->board_rect.h;

    fill_buf_rect(g_cache.board_base_buffer, g_cache.board_buf_w, g_cache.board_buf_h, 0, 0, g_cache.board_buf_w, g_cache.board_buf_h, UI_COLOR_BOARD);
    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            UiRect cell = ui_grid_cell_rect(layout, r, c);
            UINTN bx = cell.x - layout->board_rect.x;
            UINTN by = cell.y - layout->board_rect.y;
            fill_buf_rect(g_cache.board_base_buffer, g_cache.board_buf_w, g_cache.board_buf_h, bx, by, cell.w, cell.h, ui_tile_color(0));
        }
    }

    return TRUE;
}
