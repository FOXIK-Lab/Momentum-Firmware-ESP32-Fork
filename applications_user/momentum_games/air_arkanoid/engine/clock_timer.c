#include "clock_timer.h"
#include <furi.h>

/* ESP32 scheduler timer: period is the requested frames per second. */
static FuriTimer* clock_timer;
void clock_timer_start(ClockTimerCallback callback, void* context, float period) {
    furi_check(callback);
    furi_check(period > 0.0f);
    clock_timer = furi_timer_alloc(callback, FuriTimerTypePeriodic, context);
    uint32_t ticks = (uint32_t)((float)furi_kernel_get_tick_frequency() / period + 0.5f);
    furi_check(furi_timer_start(clock_timer, ticks ? ticks : 1) == FuriStatusOk);
}
void clock_timer_stop(void) {
    if(clock_timer) {
        furi_timer_stop(clock_timer);
        furi_timer_free(clock_timer);
        clock_timer = NULL;
    }
}
