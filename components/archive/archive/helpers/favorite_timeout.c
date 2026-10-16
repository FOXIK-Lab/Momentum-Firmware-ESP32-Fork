#include "favorite_timeout.h"
#include <furi.h>
#include <furi/core/event_loop_timer.h>
#include <momentum/settings.h>
#include <string.h>

typedef struct {
    ViewDispatcher* dispatcher;
    SceneManager* scene_manager;
} FavoriteTimeoutContext;

bool archive_process_favorite_launch(const char** args) {
    if(args && *args && strncmp(*args, "fav/", 4) == 0 && strlen(*args) > 4) {
        *args += 3;
        return true;
    }
    return false;
}

static void archive_favorite_timeout_expired(void* context) {
    FavoriteTimeoutContext* timeout = context;
    // Event-loop timer: scene teardown runs on the application's UI thread,
    // not on the shared timer service. Stop emulation before freeing the app.
    scene_manager_stop(timeout->scene_manager);
    view_dispatcher_stop(timeout->dispatcher);
}

void archive_favorite_timeout_run(
    ViewDispatcher* dispatcher, SceneManager* scene_manager, bool favorite) {
    uint32_t seconds = momentum_settings.favorite_timeout;
    if(!favorite || !seconds) {
        view_dispatcher_run(dispatcher);
        return;
    }
    FavoriteTimeoutContext context = {.dispatcher = dispatcher, .scene_manager = scene_manager};
    FuriEventLoopTimer* timer = furi_event_loop_timer_alloc(
        view_dispatcher_get_event_loop(dispatcher), archive_favorite_timeout_expired,
        FuriEventLoopTimerTypeOnce, &context);
    furi_event_loop_timer_start(timer, furi_ms_to_ticks(seconds * 1000));
    view_dispatcher_run(dispatcher);
    furi_event_loop_timer_stop(timer);
    furi_event_loop_timer_free(timer);
}
