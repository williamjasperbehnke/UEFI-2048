#include "ui/ui_tile_atlas.h"

#include <efilib.h>

typedef struct {
    BOOLEAN valid;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *sprite;
} TileAtlasEntry;

#define UI_TILE_ATLAS_MIN_EXP 1   /* 2^1 = 2 */
#define UI_TILE_ATLAS_MAX_EXP 23  /* 2^23 = 8388608 */
#define UI_TILE_ATLAS_SLOTS (UI_TILE_ATLAS_MAX_EXP + 1)

static TileAtlasEntry g_tile_atlas[UI_TILE_ATLAS_SLOTS];
static UINTN g_tile_atlas_tile_size = 0;

static VOID free_tile_atlas(EFI_SYSTEM_TABLE *system_table) {
    for (UINTN i = 0; i < UI_TILE_ATLAS_SLOTS; ++i) {
        if (g_tile_atlas[i].valid && g_tile_atlas[i].sprite != NULL) {
            uefi_call_wrapper(system_table->BootServices->FreePool, 1, g_tile_atlas[i].sprite);
        }
        g_tile_atlas[i].valid = FALSE;
        g_tile_atlas[i].sprite = NULL;
    }
}

static INTN tile_exp(UINT32 value) {
    if (value < 2 || (value & (value - 1)) != 0) {
        return -1;
    }

    UINTN exp = 0;
    while (value > 1) {
        value >>= 1;
        exp++;
    }

    if (exp < UI_TILE_ATLAS_MIN_EXP || exp > UI_TILE_ATLAS_MAX_EXP) {
        return -1;
    }

    return (INTN)exp;
}

static TileAtlasEntry *entry_for_value(UINT32 value) {
    INTN exp = tile_exp(value);
    if (exp < 0) {
        return NULL;
    }
    return &g_tile_atlas[(UINTN)exp];
}

VOID ui_tile_atlas_ensure_size(EFI_SYSTEM_TABLE *system_table, UINTN tile_size) {
    if (g_tile_atlas_tile_size == tile_size && tile_size != 0) {
        return;
    }
    free_tile_atlas(system_table);
    g_tile_atlas_tile_size = tile_size;
}

VOID ui_tile_atlas_learn_from_cell(UiContext *ctx, const UiGridLayout *layout, UINTN row, UINTN col, UINT32 value) {
    TileAtlasEntry *entry;

    if (value == 0) {
        return;
    }

    entry = entry_for_value(value);
    if (entry == NULL || entry->valid) {
        return;
    }

    UINTN sprite_bytes = layout->tile * layout->tile * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    EFI_STATUS st = uefi_call_wrapper(
        ctx->system_table->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        sprite_bytes,
        (VOID **)&entry->sprite
    );
    if (EFI_ERROR(st) || entry->sprite == NULL) {
        entry->valid = FALSE;
        entry->sprite = NULL;
        return;
    }

    UiRect cell = ui_grid_cell_rect(layout, row, col);
    st = uefi_call_wrapper(
        ctx->gop->Blt,
        10,
        ctx->gop,
        entry->sprite,
        EfiBltVideoToBltBuffer,
        cell.x,
        cell.y,
        0,
        0,
        layout->tile,
        layout->tile,
        0
    );
    if (EFI_ERROR(st)) {
        uefi_call_wrapper(ctx->system_table->BootServices->FreePool, 1, entry->sprite);
        entry->valid = FALSE;
        entry->sprite = NULL;
        return;
    }

    entry->valid = TRUE;
}

const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *ui_tile_atlas_sprite(UINT32 value) {
    TileAtlasEntry *entry = entry_for_value(value);
    if (entry == NULL || !entry->valid || entry->sprite == NULL) {
        return NULL;
    }
    return entry->sprite;
}

BOOLEAN ui_tile_atlas_blit_video(UiContext *ctx, const UiGridLayout *layout, UINT32 value, UINTN x, UINTN y) {
    const EFI_GRAPHICS_OUTPUT_BLT_PIXEL *sprite = ui_tile_atlas_sprite(value);
    if (sprite == NULL) {
        return FALSE;
    }

    uefi_call_wrapper(
        ctx->gop->Blt,
        10,
        ctx->gop,
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)sprite,
        EfiBltBufferToVideo,
        0,
        0,
        x,
        y,
        layout->tile,
        layout->tile,
        0
    );
    return TRUE;
}
