#include <momentum/asset_packs_i.h>
#include <momentum/settings.h>

#include <assets_icon_paths.h>
#include <gui/icon_i.h>
#include <storage/storage.h>

#define TAG "AssetPacks"
#define ICONS_FMT ASSET_PACKS_PATH "/%s/Icons/%s"
#define FONTS_FMT ASSET_PACKS_PATH "/%s/Fonts/%s.u8f"
#define U8G2_FONT_DATA_STRUCT_SIZE 23

AssetPacks* asset_packs = NULL;

typedef struct {
    Icon icon;
    uint8_t* frames[];
} AnimatedIconSwap;

typedef struct {
    int32_t width;
    int32_t height;
    int32_t frame_rate;
    int32_t frame_count;
} __attribute__((packed)) AnimatedIconMetaFile;

typedef struct {
    Icon icon;
    uint8_t* frames[1];
    uint8_t frame[];
} StaticIconSwap;

typedef struct {
    int32_t width;
    int32_t height;
} __attribute__((packed)) StaticIconBmxHeader;

static void load_icon_animated(
    const Icon* original,
    const char* name,
    FuriString* path,
    File* file) {
    furi_string_printf(path, ICONS_FMT "/meta", momentum_settings.asset_pack, name);
    if(!storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_close(file);
        return;
    }

    AnimatedIconMetaFile meta;
    bool ok = storage_file_read(file, &meta, sizeof(meta)) == sizeof(meta);
    storage_file_close(file);
    if(!ok || meta.width <= 0 || meta.height <= 0 || meta.frame_count <= 0 ||
       meta.frame_count > 64 || meta.frame_rate <= 0) {
        return;
    }

    AnimatedIconSwap* swap =
        calloc(1, sizeof(AnimatedIconSwap) + sizeof(uint8_t*) * meta.frame_count);
    int32_t loaded = 0;
    for(; loaded < meta.frame_count; ++loaded) {
        furi_string_printf(
            path, ICONS_FMT "/frame_%02ld.bm", momentum_settings.asset_pack, name, loaded);
        if(!storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
            storage_file_close(file);
            break;
        }
        size_t frame_size = (size_t)storage_file_size(file);
        swap->frames[loaded] = malloc(frame_size);
        ok = swap->frames[loaded] &&
             storage_file_read(file, swap->frames[loaded], frame_size) == frame_size;
        storage_file_close(file);
        if(!ok) break;
    }

    if(loaded == meta.frame_count) {
        *((uint16_t*)&swap->icon.width) = (uint16_t)meta.width;
        *((uint16_t*)&swap->icon.height) = (uint16_t)meta.height;
        *((uint8_t*)&swap->icon.frame_count) = (uint8_t)meta.frame_count;
        *((uint8_t*)&swap->icon.frame_rate) = (uint8_t)meta.frame_rate;
        *((const uint8_t* const**)&swap->icon.frames) =
            (const uint8_t* const*)swap->frames;
        IconSwapList_push_back(
            asset_packs->icons, (IconSwap){.original = original, .replaced = &swap->icon});
    } else {
        for(int32_t i = 0; i <= loaded && i < meta.frame_count; ++i) free(swap->frames[i]);
        free(swap);
    }
}

static void load_icon_static(
    const Icon* original,
    const char* name,
    FuriString* path,
    File* file) {
    furi_string_printf(path, ICONS_FMT ".bmx", momentum_settings.asset_pack, name);
    if(!storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_close(file);
        return;
    }

    uint64_t file_size = storage_file_size(file);
    if(file_size <= sizeof(StaticIconBmxHeader)) {
        storage_file_close(file);
        return;
    }

    StaticIconBmxHeader header;
    size_t frame_size = (size_t)(file_size - sizeof(header));
    StaticIconSwap* swap = malloc(sizeof(StaticIconSwap) + frame_size);
    bool ok = swap && storage_file_read(file, &header, sizeof(header)) == sizeof(header) &&
              header.width > 0 && header.height > 0 && header.width <= UINT16_MAX &&
              header.height <= UINT16_MAX &&
              storage_file_read(file, swap->frame, frame_size) == frame_size;
    storage_file_close(file);

    if(ok) {
        *((uint16_t*)&swap->icon.width) = (uint16_t)header.width;
        *((uint16_t*)&swap->icon.height) = (uint16_t)header.height;
        *((uint8_t*)&swap->icon.frame_count) = 1;
        *((uint8_t*)&swap->icon.frame_rate) = 0;
        swap->frames[0] = swap->frame;
        *((const uint8_t* const**)&swap->icon.frames) =
            (const uint8_t* const*)swap->frames;
        IconSwapList_push_back(
            asset_packs->icons, (IconSwap){.original = original, .replaced = &swap->icon});
    } else {
        free(swap);
    }
}

static void free_icon(const Icon* icon) {
    StaticIconSwap* swap = (void*)icon;
    if(swap->frames[0] != swap->frame) {
        for(size_t i = 0; i < swap->icon.frame_count; ++i) free(swap->frames[i]);
    }
    free(swap);
}

static const char* const font_names[FontTotalNumber] = {
    [FontPrimary] = "Primary",
    [FontSecondary] = "Secondary",
    [FontKeyboard] = "Keyboard",
    [FontBigNumbers] = "BigNumbers",
    [FontBatteryPercent] = "BatteryPercent",
};

static void load_font(Font font, FuriString* path, File* file) {
    furi_string_printf(
        path, FONTS_FMT, momentum_settings.asset_pack, font_names[font]);
    if(!storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_close(file);
        return;
    }

    size_t size = (size_t)storage_file_size(file);
    uint8_t* replacement = size > U8G2_FONT_DATA_STRUCT_SIZE ? malloc(size) : NULL;
    bool ok = replacement && storage_file_read(file, replacement, size) == size;
    storage_file_close(file);
    if(!ok) {
        free(replacement);
        return;
    }

    CanvasFontParameters* params = malloc(sizeof(CanvasFontParameters));
    if(!params) {
        free(replacement);
        return;
    }
    params->leading_default = replacement[10];
    params->leading_min = params->leading_default > 2 ? params->leading_default - 2 : 1;
    params->height = MAX((int8_t)replacement[15], 0);
    params->descender = MAX((int8_t)replacement[16], 0);
    asset_packs->fonts[font] = replacement;
    asset_packs->font_params[font] = params;
}

void asset_packs_init(void) {
    if(asset_packs || momentum_settings.asset_pack[0] == '\0') return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* path = furi_string_alloc_printf(
        ASSET_PACKS_PATH "/%s/Icons", momentum_settings.asset_pack);
    FileInfo info;
    if(storage_common_stat(storage, furi_string_get_cstr(path), &info) == FSE_OK &&
       (info.flags & FSF_DIRECTORY)) {
        asset_packs = calloc(1, sizeof(AssetPacks));
        IconSwapList_init(asset_packs->icons);
        File* file = storage_file_alloc(storage);
        for(size_t i = 0; i < ASSET_ICON_PATHS_COUNT; ++i) {
            const AssetIconPath* entry = &ASSET_ICON_PATHS[i];
            /* Alias fallback rows must not replace an already loaded primary
               icon or allocate duplicate replacements for the same original. */
            if(asset_packs_swap_icon(entry->icon) != entry->icon) continue;
            if(entry->icon->frame_count > 1) {
                load_icon_animated(entry->icon, entry->path, path, file);
            } else {
                load_icon_static(entry->icon, entry->path, path, file);
            }
        }
        for(Font font = 0; font < FontTotalNumber; ++font) {
            load_font(font, path, file);
        }
        storage_file_free(file);
        FURI_LOG_I(
            TAG,
            "Loaded '%s' (%u icons)",
            momentum_settings.asset_pack,
            (unsigned)IconSwapList_size(asset_packs->icons));
    }
    furi_string_free(path);
    furi_record_close(RECORD_STORAGE);
}

void asset_packs_free(void) {
    if(!asset_packs) return;
    for M_EACH(icon_swap, asset_packs->icons, IconSwapList_t) {
        free_icon(icon_swap->replaced);
    }
    IconSwapList_clear(asset_packs->icons);
    for(Font font = 0; font < FontTotalNumber; ++font) {
        free(asset_packs->fonts[font]);
        free(asset_packs->font_params[font]);
    }
    free(asset_packs);
    asset_packs = NULL;
}

const Icon* asset_packs_swap_icon(const Icon* requested) {
    if(!asset_packs || !requested) return requested;
    for M_EACH(icon_swap, asset_packs->icons, IconSwapList_t) {
        if(icon_swap->original == requested) return icon_swap->replaced;
    }
    return requested;
}

const uint8_t* asset_packs_get_font(Font font) {
    return asset_packs && font < FontTotalNumber ? asset_packs->fonts[font] : NULL;
}

const CanvasFontParameters* asset_packs_get_font_params(Font font) {
    return asset_packs && font < FontTotalNumber ? asset_packs->font_params[font] : NULL;
}
