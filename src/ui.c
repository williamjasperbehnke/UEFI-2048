/* UI module: board rendering and text styling for the UEFI console. */
#include "ui.h"

#include <efilib.h>

static VOID set_color(EFI_SYSTEM_TABLE *system_table, UINTN fg, UINTN bg) {
    uefi_call_wrapper(system_table->ConOut->SetAttribute, 2, system_table->ConOut, EFI_TEXT_ATTR(fg, bg));
}

static VOID reset_color(EFI_SYSTEM_TABLE *system_table) {
    set_color(system_table, EFI_LIGHTGRAY, EFI_BLACK);
}

static UINTN tile_fg_color(UINT32 value) {
    switch (value) {
        case 0: return EFI_DARKGRAY;
        case 2: return EFI_LIGHTGRAY;
        case 4: return EFI_YELLOW;
        case 8: return EFI_LIGHTRED;
        case 16: return EFI_RED;
        case 32: return EFI_LIGHTMAGENTA;
        case 64: return EFI_MAGENTA;
        case 128: return EFI_LIGHTCYAN;
        case 256: return EFI_CYAN;
        case 512: return EFI_LIGHTGREEN;
        case 1024: return EFI_GREEN;
        case 2048: return EFI_WHITE;
        default: return EFI_WHITE;
    }
}

VOID ui_clear_screen(EFI_SYSTEM_TABLE *system_table) {
    uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
}

static VOID print_separator(void) {
    Print(L"+------+------+------+------+\n");
}

VOID ui_draw_board(EFI_SYSTEM_TABLE *system_table, const GameState *game) {
    ui_clear_screen(system_table);
    reset_color(system_table);

    Print(L"UEFI 2048\n");
    Print(L"Score: %lu\n", (unsigned long)game->score);
    Print(L"Use Arrow Keys (or WASD), R to restart, Q to quit\n\n");

    for (UINTN r = 0; r < BOARD_SIZE; ++r) {
        print_separator();
        for (UINTN c = 0; c < BOARD_SIZE; ++c) {
            UINT32 value = game->cells[r][c];
            set_color(system_table, tile_fg_color(value), EFI_BLACK);
            if (value == 0) {
                Print(L"|%6s", L".");
            } else {
                Print(L"|%6u", value);
            }
            reset_color(system_table);
        }
        Print(L"|\n");
    }
    print_separator();

    if (game->won) {
        set_color(system_table, EFI_LIGHTGREEN, EFI_BLACK);
        Print(L"\nYou reached %d! Keep going or press R to restart.\n", TARGET_TILE);
        reset_color(system_table);
    }
    if (!game_has_moves(game)) {
        set_color(system_table, EFI_LIGHTRED, EFI_BLACK);
        Print(L"\nGame Over! Press R to restart or Q to quit.\n");
        reset_color(system_table);
    }
}
