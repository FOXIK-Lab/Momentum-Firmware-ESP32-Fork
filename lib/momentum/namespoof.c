#include "namespoof.h"

#include <flipper_format/flipper_format.h>
#include <furi_hal_version.h>

#define TAG "NameSpoof"

void namespoof_init(void) {
    FuriString* str = furi_string_alloc();
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* file = flipper_format_file_alloc(storage);

    do {
        uint32_t version;
        if(!flipper_format_file_open_existing(file, NAMESPOOF_PATH)) break;
        if(!flipper_format_read_header(file, str, &version)) break;
        if(furi_string_cmp_str(str, NAMESPOOF_HEADER)) break;
        if(version != NAMESPOOF_VERSION) break;

        if(!flipper_format_read_string(file, "Name", str)) break;
        /* On Flipper, custom_name is heap-owned and Momentum replaces/frees
           it.  The ESP32 port keeps the active name in a static HAL buffer,
           so freeing version_get_custom_name() corrupts memory and causes a
           boot loop.  The HAL setter copies the spoofed name safely. */
        furi_hal_version_set_name(furi_string_get_cstr(str));

        uint32_t color;
        if(flipper_format_read_uint32(file, "Color", &color, 1) &&
           color < FuriHalVersionColorCount) {
            furi_hal_version_set_hw_color((FuriHalVersionColor)color);
        }
    } while(false);

    flipper_format_free(file);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(str);
}
