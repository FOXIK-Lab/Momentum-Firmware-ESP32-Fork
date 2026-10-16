/* Original Momentum assets/icons/Archive/search_10px.png; GPL-3.0.
 * Lossless conversion to the port's uncompressed monochrome icon format. */
#include "archive_search_icon.h"
#include <gui/icon_i.h>
static const uint8_t search_frame[] = {
    0x00,0x1c,0x00,0x22,0x00,0x41,0x00,0x41,0x00,0x41,0x00,
    0x22,0x00,0x5c,0x00,0x80,0x00,0x00,0x01,0x00,0x02,
};
static const uint8_t* const search_frames[] = {search_frame};
const Icon I_search_10px = {.width=10, .height=10, .frame_count=1, .frame_rate=0, .frames=search_frames};
