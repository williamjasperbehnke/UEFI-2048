#ifndef AUDIO_H
#define AUDIO_H

#include <efi.h>

typedef enum {
    AUDIO_SOUND_MOVE = 0,
    AUDIO_SOUND_RESTART,
    AUDIO_SOUND_WIN,
    AUDIO_SOUND_GAME_OVER
} AudioSound;

VOID audio_play_sound(EFI_SYSTEM_TABLE *system_table, AudioSound sound);

#endif
