#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MOMENTUM_BAD_PIN_FORMAT_THRESHOLD 10U

/* Pure predicates are also tested on the host, without formatting any media. */
static inline bool momentum_bad_pin_format_due(bool enabled, bool confirmed, uint32_t failures) {
    return enabled && confirmed && failures >= MOMENTUM_BAD_PIN_FORMAT_THRESHOLD;
}

static inline bool momentum_locked_rpc_allowed(bool locked, bool allowed_while_locked) {
    return !locked || allowed_while_locked;
}

static inline size_t momentum_scroll_offset(size_t scroll, size_t span, bool marquee) {
    if(span == 0) return 0;
    if(!marquee) return scroll % (span + 3);
    const size_t delay = 3;
    size_t position = scroll % (span * 2 + delay * 2);
    if(position < span) return position;
    if(position < span + delay) return span;
    if(position < span * 2 + delay) return span - (position - (span + delay));
    return 0;
}
