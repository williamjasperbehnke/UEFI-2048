/* Main module: orchestrates game, input, rendering, and audio flow. */
#include <efi.h>
#include <efilib.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "ui.h"

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    game_seed_rng(system_table);

    GameState game;
    game_start_new(&game);
    BOOLEAN win_announced = FALSE;
    BOOLEAN game_over_announced = FALSE;
    ui_draw_board(system_table, &game);

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
            game_over_announced = FALSE;
            ui_draw_board(system_table, &game);
            continue;
        }
        if (input_is_quit_key(&key)) {
            break;
        }
        if (!game_has_moves(&game)) {
            continue;
        }

        MoveDir dir;
        if (input_key_to_move(&key, &dir)) {
            if (game_apply_move(&game, dir)) {
                audio_play_sound(system_table, AUDIO_SOUND_MOVE);

                if (game.won && !win_announced) {
                    audio_play_sound(system_table, AUDIO_SOUND_WIN);
                    win_announced = TRUE;
                }
                if (!game_has_moves(&game) && !game_over_announced) {
                    audio_play_sound(system_table, AUDIO_SOUND_GAME_OVER);
                    game_over_announced = TRUE;
                }

                ui_draw_board(system_table, &game);
            }
        }
    }

    ui_clear_screen(system_table);
    Print(L"Thanks for playing UEFI 2048.\n");
    return EFI_SUCCESS;
}
