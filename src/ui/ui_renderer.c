/* UI facade: public UI API entrypoints and text-mode fallbacks. */
#include "ui_renderer.h"

#include <efilib.h>

#include "ui_context.h"
#include "ui_primitives.h"
#include "ui_board_scene.h"
#include "ui_tutorial_scene.h"

VOID ui_clear_screen(EFI_SYSTEM_TABLE *system_table) {
    UiContext ctx;
    if (!ui_ctx_begin(&ctx, system_table)) {
        uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
        return;
    }

    ui_fill_rect(&ctx, 0, 0, ctx.screen_w, ctx.screen_h, ui_rgb(248, 246, 240));
}

VOID ui_draw_board(EFI_SYSTEM_TABLE *system_table, const GameState *game, BOOLEAN is_game_over) {
    UiContext ctx;
    if (!ui_ctx_begin(&ctx, system_table)) {
        uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
        Print(L"UEFI 2048\nScore: %lu\n\n", (unsigned long)game->score);
        for (UINTN r = 0; r < BOARD_SIZE; ++r) {
            for (UINTN c = 0; c < BOARD_SIZE; ++c) {
                Print(L"%6u ", game->cells[r][c]);
            }
            Print(L"\n");
        }
        if (game->won) Print(L"\nYou reached 2048!\n");
        if (is_game_over) Print(L"\nGame Over! Press R to restart.\n");
        return;
    }

    ui_render_board_scene(&ctx, game, is_game_over);
}

VOID ui_draw_tutorial_frame(EFI_SYSTEM_TABLE *system_table, UINTN frame_index) {
    UiContext ctx;
    if (!ui_ctx_begin(&ctx, system_table)) {
        uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
        Print(L"2048 Tutorial %u/3\n", (unsigned)(frame_index + 1));
        Print(L"Press any key to continue...\n");
        return;
    }

    ui_render_tutorial_scene(&ctx, frame_index);
}
