/* Audio module: low-level PC speaker tone generation and sound patterns. */
#include "audio.h"

static BOOLEAN g_audio_primed = FALSE;

static UINT8 inb(UINT16 port) {
    UINT8 value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

static VOID outb(UINT16 port, UINT8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "dN"(port));
}

static VOID pcspk_prime(EFI_SYSTEM_TABLE *system_table) {
    if (g_audio_primed) {
        return;
    }

    /* Prime speaker path once; some QEMU/OVMF setups drop the first audible tone. */
    UINT8 speaker_state = inb(0x61);
    outb(0x61, (UINT8)(speaker_state | 0x03));
    uefi_call_wrapper(system_table->BootServices->Stall, 1, 2000);
    outb(0x61, (UINT8)(speaker_state & (UINT8)~0x03));
    uefi_call_wrapper(system_table->BootServices->Stall, 1, 2000);

    g_audio_primed = TRUE;
}

static VOID pcspk_tone(EFI_SYSTEM_TABLE *system_table, UINTN frequency_hz, UINTN duration_ms) {
    if (frequency_hz == 0) {
        return;
    }

    /* PIT input clock is 1,193,182 Hz. */
    UINTN divisor = 1193182 / frequency_hz;
    if (divisor == 0 || divisor > 0xFFFF) {
        return;
    }

    outb(0x43, 0xB6);
    outb(0x42, (UINT8)(divisor & 0xFF));
    outb(0x42, (UINT8)((divisor >> 8) & 0xFF));

    UINT8 speaker_state = inb(0x61);
    outb(0x61, (UINT8)(speaker_state | 0x03));
    uefi_call_wrapper(system_table->BootServices->Stall, 1, (UINTN)(duration_ms * 1000));
    outb(0x61, (UINT8)(speaker_state & (UINT8)~0x03));
}

VOID audio_play_sound(EFI_SYSTEM_TABLE *system_table, AudioSound sound) {
    pcspk_prime(system_table);

    switch (sound) {
        case AUDIO_SOUND_MOVE:
            pcspk_tone(system_table, 880, 40);
            break;
        case AUDIO_SOUND_RESTART:
            pcspk_tone(system_table, 660, 40);
            pcspk_tone(system_table, 880, 40);
            break;
        case AUDIO_SOUND_WIN:
            pcspk_tone(system_table, 880, 50);
            pcspk_tone(system_table, 1046, 60);
            pcspk_tone(system_table, 1318, 90);
            break;
        case AUDIO_SOUND_GAME_OVER:
            pcspk_tone(system_table, 330, 90);
            pcspk_tone(system_table, 220, 120);
            break;
    }
}
