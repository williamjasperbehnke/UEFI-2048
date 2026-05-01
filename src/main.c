/* Main module: orchestrates game, input, rendering, and audio flow. */
#include <efi.h>
#include <efilib.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "ui/ui_renderer.h"

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    game_seed_rng(system_table);

    for (UINTN i = 0; i < 3; ++i) {
        ui_draw_tutorial_frame(system_table, i);

        /* Advance on key press; if input is flaky, auto-advance after ~4 seconds. */
        UINTN ticks = 0;
        while (1) {
            EFI_INPUT_KEY tutorial_key;
            EFI_STATUS st = input_wait_key(system_table, &tutorial_key);
            if (!EFI_ERROR(st)) {
                break;
            }

            uefi_call_wrapper(system_table->BootServices->Stall, 1, 50000);
            ticks++;
            if (ticks >= 80) {
                break;
            }
        }
    }

    GameState game;
    game_start_new(&game);

    BOOLEAN win_announced = FALSE;
    BOOLEAN show_win_overlay = FALSE;
    BOOLEAN game_over_announced = FALSE;
    BOOLEAN is_game_over = !game_has_moves(&game);
    ui_clear_screen(system_table);
    ui_draw_board(system_table, &game, is_game_over, show_win_overlay);

    while (1) {
        EFI_INPUT_KEY key;
        EFI_STATUS st = input_wait_key(system_table, &key);
        if (EFI_ERROR(st)) {
            continue;
        }

        if (input_is_restart_key(&key)) {
            game_start_new(&game);
            audio_play_sound(system_table, AUDIO_SOUND_RESTART);
            win_announced = FALSE;
            show_win_overlay = FALSE;
            game_over_announced = FALSE;
            is_game_over = !game_has_moves(&game);
            ui_clear_screen(system_table);
            ui_draw_board(system_table, &game, is_game_over, show_win_overlay);
            continue;
        }
        if (show_win_overlay && !input_is_restart_key(&key) && !input_is_quit_key(&key)) {
            show_win_overlay = FALSE;
            ui_clear_screen(system_table);
            ui_draw_board(system_table, &game, is_game_over, show_win_overlay);
            continue;
        }
        if (input_is_quit_key(&key)) {
            break;
        }
        if (is_game_over) {
            continue;
        }

        MoveDir dir;
        if (input_key_to_move(&key, &dir)) {
            if (game_apply_move(&game, dir)) {
                audio_play_sound(system_table, AUDIO_SOUND_MOVE);

                if (game.won && !win_announced) {
                    audio_play_sound(system_table, AUDIO_SOUND_WIN);
                    win_announced = TRUE;
                    show_win_overlay = TRUE;
                }
                is_game_over = !game_has_moves(&game);
                if (is_game_over && !game_over_announced) {
                    audio_play_sound(system_table, AUDIO_SOUND_GAME_OVER);
                    game_over_announced = TRUE;
                }

                ui_draw_board(system_table, &game, is_game_over, show_win_overlay);
            }
        }
    }

    ui_clear_screen(system_table);
    Print(L"Thanks for playing UEFI 2048.\n");
    return EFI_SUCCESS;
}
