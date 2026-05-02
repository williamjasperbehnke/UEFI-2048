/* UI context: GOP setup and low-level rectangle blitting. */
#include "ui/ui_context.h"

#include <efilib.h>

static EFI_GRAPHICS_OUTPUT_PROTOCOL *g_shared_gop = NULL;

BOOLEAN ui_ctx_begin(UiContext *ctx, EFI_SYSTEM_TABLE *system_table) {
    ctx->system_table = system_table;

    if (g_shared_gop == NULL) {
        EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        EFI_STATUS st = uefi_call_wrapper(
            system_table->BootServices->LocateProtocol,
            3,
            &gop_guid,
            NULL,
            (VOID **)&g_shared_gop
        );
        if (EFI_ERROR(st) || g_shared_gop == NULL) {
            ctx->gop = NULL;
            ctx->screen_w = 0;
            ctx->screen_h = 0;
            return FALSE;
        }
    }

    ctx->gop = g_shared_gop;
    ctx->screen_w = g_shared_gop->Mode->Info->HorizontalResolution;
    ctx->screen_h = g_shared_gop->Mode->Info->VerticalResolution;
    return TRUE;
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL ui_rgb(UINT8 r, UINT8 g, UINT8 b) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL p;
    p.Red = r;
    p.Green = g;
    p.Blue = b;
    p.Reserved = 0;
    return p;
}

VOID ui_fill_rect(UiContext *ctx, UINTN x, UINTN y, UINTN w, UINTN h, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
    if (ctx->gop == NULL || w == 0 || h == 0) {
        return;
    }

    uefi_call_wrapper(
        ctx->gop->Blt,
        10,
        ctx->gop,
        &color,
        EfiBltVideoFill,
        0,
        0,
        x,
        y,
        w,
        h,
        0
    );
}

UiGridLayout ui_make_grid_layout(const UiContext *ctx, UINTN board_scale_percent, UINTN min_gap) {
    UiGridLayout layout;
    UINTN board = (ctx->screen_w < ctx->screen_h ? ctx->screen_w : ctx->screen_h) * board_scale_percent / 100;
    layout.board_rect.w = board;
    layout.board_rect.h = board;
    layout.board_rect.x = (ctx->screen_w - board) / 2;
    layout.board_rect.y = (ctx->screen_h - board) / 2;

    layout.gap = board / 30;
    if (layout.gap < min_gap) layout.gap = min_gap;
    layout.tile = (board - layout.gap * (BOARD_SIZE + 1)) / BOARD_SIZE;
    return layout;
}

UiRect ui_grid_cell_rect(const UiGridLayout *layout, UINTN row, UINTN col) {
    UiRect cell;
    cell.x = layout->board_rect.x + layout->gap + col * (layout->tile + layout->gap);
    cell.y = layout->board_rect.y + layout->gap + row * (layout->tile + layout->gap);
    cell.w = layout->tile;
    cell.h = layout->tile;
    return cell;
}
