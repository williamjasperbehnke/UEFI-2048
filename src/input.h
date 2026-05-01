#ifndef INPUT_H
#define INPUT_H

#include <efi.h>
#include "game.h"

EFI_STATUS input_wait_key(EFI_SYSTEM_TABLE *system_table, EFI_INPUT_KEY *key);
BOOLEAN input_is_quit_key(const EFI_INPUT_KEY *key);
BOOLEAN input_is_restart_key(const EFI_INPUT_KEY *key);
BOOLEAN input_key_to_move(const EFI_INPUT_KEY *key, MoveDir *dir);

#endif
