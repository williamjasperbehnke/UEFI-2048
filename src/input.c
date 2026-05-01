/* Input module: keyboard wait/read and key-to-action mapping. */
#include "input.h"

EFI_STATUS input_wait_key(EFI_SYSTEM_TABLE *system_table, EFI_INPUT_KEY *key) {
    UINTN index = 0;
    EFI_STATUS st = uefi_call_wrapper(system_table->BootServices->WaitForEvent, 3, 1, &system_table->ConIn->WaitForKey, &index);

    if (EFI_ERROR(st)) {
        return st;
    }

    return uefi_call_wrapper(system_table->ConIn->ReadKeyStroke, 2, system_table->ConIn, key);
}

BOOLEAN input_is_quit_key(const EFI_INPUT_KEY *key) {
    return key->UnicodeChar == L'q' || key->UnicodeChar == L'Q' || key->ScanCode == SCAN_ESC;
}

BOOLEAN input_is_restart_key(const EFI_INPUT_KEY *key) {
    return key->UnicodeChar == L'r' || key->UnicodeChar == L'R';
}

BOOLEAN input_key_to_move(const EFI_INPUT_KEY *key, MoveDir *dir) {
    if (key->ScanCode == SCAN_LEFT || key->UnicodeChar == L'a' || key->UnicodeChar == L'A') {
        *dir = DIR_LEFT;
        return TRUE;
    }
    if (key->ScanCode == SCAN_RIGHT || key->UnicodeChar == L'd' || key->UnicodeChar == L'D') {
        *dir = DIR_RIGHT;
        return TRUE;
    }
    if (key->ScanCode == SCAN_UP || key->UnicodeChar == L'w' || key->UnicodeChar == L'W') {
        *dir = DIR_UP;
        return TRUE;
    }
    if (key->ScanCode == SCAN_DOWN || key->UnicodeChar == L's' || key->UnicodeChar == L'S') {
        *dir = DIR_DOWN;
        return TRUE;
    }
    return FALSE;
}
