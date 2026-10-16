#include <furi.h>

#include <gui/elements.h>
#include <gui/icon.h>
#include <gui/view.h>

#include <assets_icons.h>
#include <momentum_lock_icon.h>
#include <locale/locale.h>
#include <momentum/settings.h>
#include <furi_hal_rtc.h>

#include "../desktop_i.h"
#include "desktop_view_locked.h"

#define DOOR_MOVING_INTERVAL_MS  (50)
#define LOCKED_HINT_TIMEOUT_MS   (3000)
#define UNLOCKED_HINT_TIMEOUT_MS (2000)

#define DOOR_OFFSET_START (-64)
#define DOOR_OFFSET_END   (0)

#define DOOR_L_FINAL_POS (0)
#define DOOR_R_FINAL_POS (60)

#define UNLOCK_CNT         (3)
#define UNLOCK_RST_TIMEOUT (600)

struct DesktopViewLocked {
    View* view;
    DesktopViewLockedCallback callback;
    void* context;

    FuriTimer* timer;
    uint8_t lock_count;
    uint32_t lock_lastpress;
};

typedef enum {
    DesktopViewLockedStateUnlocked,
    DesktopViewLockedStateLocked,
    DesktopViewLockedStateDoorsClosing,
    DesktopViewLockedStateCoverOpening,
    DesktopViewLockedStateLockedHintShown,
    DesktopViewLockedStateUnlockedHintShown
} DesktopViewLockedState;

typedef struct {
    bool pin_locked;
    int8_t door_offset;
    DesktopViewLockedState view_state;
} DesktopViewLockedModel;

void desktop_view_locked_set_callback(
    DesktopViewLocked* locked_view,
    DesktopViewLockedCallback callback,
    void* context) {
    furi_assert(locked_view);
    furi_assert(callback);
    locked_view->callback = callback;
    locked_view->context = context;
}

static void locked_view_timer_callback(void* context) {
    DesktopViewLocked* locked_view = context;
    locked_view->callback(DesktopLockedEventUpdate, locked_view->context);
}

static void desktop_view_locked_doors_draw(Canvas* canvas, DesktopViewLockedModel* model) {
    int32_t offset = model->door_offset;
    int32_t door_left_x = DOOR_L_FINAL_POS + offset;
    int32_t door_right_x = DOOR_R_FINAL_POS - offset;
    size_t height = icon_get_height(&I_DoorLeft_70x55);
    canvas_draw_icon(canvas, door_left_x, canvas_height(canvas) - height, &I_DoorLeft_70x55);
    canvas_draw_icon(canvas, door_right_x, canvas_height(canvas) - height, &I_DoorRight_70x55);
}

static bool desktop_view_locked_doors_move(DesktopViewLockedModel* model) {
    bool stop = false;
    if(model->door_offset < DOOR_OFFSET_END) {
        model->door_offset = CLAMP(model->door_offset + 8, DOOR_OFFSET_END, DOOR_OFFSET_START);
        stop = true;
    }

    return stop;
}

static void desktop_view_locked_update_hint_icon_timeout(DesktopViewLocked* locked_view) {
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    const bool change_state = (model->view_state == DesktopViewLockedStateLocked) &&
                              !model->pin_locked;
    if(change_state) {
        model->view_state = DesktopViewLockedStateLockedHintShown;
    }
    view_commit_model(locked_view->view, change_state);
    furi_timer_start(locked_view->timer, furi_ms_to_ticks(LOCKED_HINT_TIMEOUT_MS));
}

void desktop_view_locked_update(DesktopViewLocked* locked_view) {
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    DesktopViewLockedState view_state = model->view_state;

    if(view_state == DesktopViewLockedStateCoverOpening) {
        model->door_offset = CLAMP(model->door_offset - 8, DOOR_OFFSET_END, DOOR_OFFSET_START);
        if(model->door_offset == DOOR_OFFSET_START) {
            model->view_state = DesktopViewLockedStateUnlocked;
            furi_timer_stop(locked_view->timer);
        }
        view_commit_model(locked_view->view, true);
        return;
    }

    if(view_state == DesktopViewLockedStateDoorsClosing &&
       !desktop_view_locked_doors_move(model)) {
        locked_view->callback(DesktopLockedEventDoorsClosed, locked_view->context);
        model->view_state = DesktopViewLockedStateLocked;
    } else if(view_state == DesktopViewLockedStateLockedHintShown) {
        model->view_state = DesktopViewLockedStateLocked;
    } else if(view_state == DesktopViewLockedStateUnlockedHintShown) {
        model->view_state = DesktopViewLockedStateUnlocked;
    }

    view_commit_model(locked_view->view, true);

    if(view_state != DesktopViewLockedStateDoorsClosing) {
        if(view_state == DesktopViewLockedStateUnlockedHintShown ||
           view_state == DesktopViewLockedStateUnlocked) {
            furi_timer_stop(locked_view->timer);
        } else {
            furi_timer_start(locked_view->timer, furi_ms_to_ticks(1000));
        }
    }
}

static void desktop_view_locked_draw_lockscreen(Canvas* canvas, void* m) {
    DesktopViewLockedModel* model = m;
    int32_t y = model->door_offset;
    char time_str[9];
    char second_str[5];
    char date_str[14];
    char meridian_str[3];
    DateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);
    LocaleTimeFormat time_format = locale_get_time_format();
    LocaleDateFormat date_format = locale_get_date_format();

    bool pm;
    if(time_format == LocaleTimeFormat24h) {
        pm = false;
    } else {
        pm = datetime.hour > 12;
        snprintf(meridian_str, 3, datetime.hour >= 12 ? "PM" : "AM");
        if(datetime.hour == 0) {
            datetime.hour = momentum_settings.midnight_format_00 ? 0 : 12;
        }
    }
    snprintf(time_str, 9, "%.2d:%.2d", pm ? datetime.hour - 12 : datetime.hour, datetime.minute);
    snprintf(second_str, 5, ":%.2d", datetime.second);

    if(date_format == LocaleDateFormatYMD) {
        snprintf(date_str, 14, "%.4d-%.2d-%.2d", datetime.year, datetime.month, datetime.day);
    } else if(date_format == LocaleDateFormatMDY) {
        snprintf(date_str, 14, "%.2d-%.2d-%.4d", datetime.month, datetime.day, datetime.year);
    } else {
        snprintf(date_str, 14, "%.2d-%.2d-%.4d", datetime.day, datetime.month, datetime.year);
    }

    if(!momentum_settings.lockscreen_transparent) {
        canvas_draw_icon(canvas, 0, 0 + y, &I_Lockscreen);
    }
    if(momentum_settings.lockscreen_time) {
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str(canvas, 0, 64 + y, time_str);
        int offset = canvas_string_width(canvas, time_str) + 2;
        if(momentum_settings.lockscreen_seconds) {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 0 + offset, 64 + y, second_str);
            offset += canvas_string_width(canvas, ":00") + 2;
        }
        if(time_format == LocaleTimeFormat12h) {
            canvas_set_font(canvas, FontKeyboard);
            canvas_draw_str(canvas, 0 + offset, 64 + y, meridian_str);
        }
    }
    if(momentum_settings.lockscreen_date) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 48 + y + 16 * !momentum_settings.lockscreen_time, date_str);
    }
    if(model->view_state == DesktopViewLockedStateLockedHintShown &&
       momentum_settings.lockscreen_prompt) {
        canvas_set_font(canvas, FontSecondary);
        if(model->pin_locked) {
            elements_bubble_str(
                canvas, 12, 14 + y, "  Press   \nto unlock!", AlignRight, AlignBottom);
            canvas_draw_icon(canvas, 45, 16 + y, &I_Pin_arrow_up_7x9);
        } else {
            elements_bubble_str(
                canvas, 2, 14 + y, "Press 3x      \n  to unlock!", AlignRight, AlignBottom);
            canvas_draw_icon(canvas, 43, 17 + y, &I_Pin_back_arrow_10x8);
        }
    }
}

static void desktop_view_locked_draw(Canvas* canvas, void* model) {
    DesktopViewLockedModel* m = model;
    canvas_set_color(canvas, ColorBlack);
    if(m->view_state == DesktopViewLockedStateDoorsClosing ||
       m->view_state == DesktopViewLockedStateCoverOpening ||
       m->view_state == DesktopViewLockedStateLocked ||
       m->view_state == DesktopViewLockedStateLockedHintShown) {
        desktop_view_locked_draw_lockscreen(canvas, m);
    }
}

View* desktop_view_locked_get_view(DesktopViewLocked* locked_view) {
    furi_assert(locked_view);
    return locked_view->view;
}

static bool desktop_view_locked_input(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    bool is_changed = false;
    const uint32_t press_time = furi_get_tick();
    DesktopViewLocked* locked_view = context;
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    if(model->view_state == DesktopViewLockedStateUnlockedHintShown &&
       event->type == InputTypePress) {
        model->view_state = DesktopViewLockedStateUnlocked;
        is_changed = true;
    }
    const DesktopViewLockedState view_state = model->view_state;
    const bool pin_locked = model->pin_locked;
    view_commit_model(locked_view->view, is_changed);

    if(view_state == DesktopViewLockedStateUnlocked) {
        return false;
    } else if(event->key == InputKeyBack && event->type == InputTypeLong &&
              momentum_settings.lockscreen_poweroff) {
        Loader* loader = furi_record_open(RECORD_LOADER);
        loader_start_detached_with_gui_error(loader, "Power", "off");
        furi_record_close(RECORD_LOADER);
    } else if(pin_locked) {
        /* Never fall through to the no-PIN triple-Back unlock path. */
        if(view_state == DesktopViewLockedStateLocked && event->key == InputKeyOk &&
           event->type == InputTypeShort) {
            locked_view->callback(DesktopLockedEventShowPinInput, locked_view->context);
        }
    } else if(
        view_state == DesktopViewLockedStateLocked ||
        view_state == DesktopViewLockedStateLockedHintShown) {
        if(press_time - locked_view->lock_lastpress > UNLOCK_RST_TIMEOUT) {
            locked_view->lock_lastpress = press_time;
            locked_view->lock_count = 0;
        }

        desktop_view_locked_update_hint_icon_timeout(locked_view);

        if(event->key == InputKeyBack) {
            if(event->type == InputTypeShort) {
                locked_view->lock_lastpress = press_time;
                locked_view->lock_count++;
                if(locked_view->lock_count == UNLOCK_CNT) {
                    locked_view->callback(DesktopLockedEventUnlocked, locked_view->context);
                }
            }
        } else {
            locked_view->lock_count = 0;
        }

        locked_view->lock_lastpress = press_time;
    }

    return true;
}

DesktopViewLocked* desktop_view_locked_alloc(void) {
    DesktopViewLocked* locked_view = calloc(1, sizeof(DesktopViewLocked));
    locked_view->view = view_alloc();
    locked_view->timer =
        furi_timer_alloc(locked_view_timer_callback, FuriTimerTypePeriodic, locked_view);

    view_allocate_model(locked_view->view, ViewModelTypeLocking, sizeof(DesktopViewLockedModel));
    view_set_context(locked_view->view, locked_view);
    view_set_draw_callback(locked_view->view, desktop_view_locked_draw);
    view_set_input_callback(locked_view->view, desktop_view_locked_input);

    return locked_view;
}

void desktop_view_locked_free(DesktopViewLocked* locked_view) {
    furi_assert(locked_view);
    furi_timer_free(locked_view->timer);
    view_free(locked_view->view);
    free(locked_view);
}

void desktop_view_locked_close_doors(DesktopViewLocked* locked_view) {
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    furi_assert(model->view_state == DesktopViewLockedStateLocked);
    model->view_state = momentum_settings.lockscreen_skip_animation ?
                            DesktopViewLockedStateLocked : DesktopViewLockedStateDoorsClosing;
    model->door_offset = momentum_settings.lockscreen_skip_animation ?
                             DOOR_OFFSET_END : DOOR_OFFSET_START;
    view_commit_model(locked_view->view, true);
    furi_timer_start(locked_view->timer, momentum_settings.lockscreen_skip_animation ?
                                          furi_ms_to_ticks(1000) : furi_ms_to_ticks(DOOR_MOVING_INTERVAL_MS));
    if(momentum_settings.lockscreen_skip_animation)
        locked_view->callback(DesktopLockedEventDoorsClosed, locked_view->context);
}

void desktop_view_locked_lock(DesktopViewLocked* locked_view, bool pin_locked) {
    locked_view->lock_count = 0;
    locked_view->lock_lastpress = furi_get_tick();
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    furi_assert(
        model->view_state == DesktopViewLockedStateUnlocked ||
        model->view_state == DesktopViewLockedStateUnlockedHintShown);
    model->view_state = DesktopViewLockedStateLocked;
    model->pin_locked = pin_locked;
    view_commit_model(locked_view->view, true);
}

void desktop_view_locked_unlock(DesktopViewLocked* locked_view) {
    locked_view->lock_count = 0;
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    model->view_state = momentum_settings.lockscreen_skip_animation ?
                            DesktopViewLockedStateUnlocked : DesktopViewLockedStateCoverOpening;
    model->door_offset = momentum_settings.lockscreen_skip_animation ? DOOR_OFFSET_START : 0;
    model->pin_locked = false;
    view_commit_model(locked_view->view, true);
    if(momentum_settings.lockscreen_skip_animation) furi_timer_stop(locked_view->timer);
    else furi_timer_start(locked_view->timer, furi_ms_to_ticks(DOOR_MOVING_INTERVAL_MS));
}

bool desktop_view_locked_is_locked_hint_visible(DesktopViewLocked* locked_view) {
    DesktopViewLockedModel* model = view_get_model(locked_view->view);
    const DesktopViewLockedState view_state = model->view_state;
    view_commit_model(locked_view->view, false);
    return view_state == DesktopViewLockedStateLockedHintShown ||
           view_state == DesktopViewLockedStateLocked;
}
