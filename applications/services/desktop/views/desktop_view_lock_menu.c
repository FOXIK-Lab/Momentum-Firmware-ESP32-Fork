#include <furi.h>
#include <gui/elements.h>
#include <gui/modules/menu.h>
#include <assets_icons.h>
#include <notification/notification_messages.h>
#include <furi_hal_rtc.h>
#include <furi_hal_display.h>

#include "../desktop_i.h"
#include "desktop_view_lock_menu.h"

#define LOCK_MENU_MAX_ITEMS 7

// Device actions formerly added by the ESP32 port live in their relevant
// modules now (WiFi, Bluetooth and Settings/Others).  The Unleashed lock menu
// keeps only the remaining Web-Filesystem shortcut.

typedef struct {
    const char* label;
    DesktopEvent event;
} LockMenuItem;

static LockMenuItem s_items[LOCK_MENU_MAX_ITEMS];
static uint8_t s_item_count = 0;

// Rows that fit on screen below the status bar (each item is 17px tall).
#define LOCK_MENU_VISIBLE 3
static uint8_t s_top = 0; // index of the first visible item

/* Index order and navigation are transplanted from Momentum-Firmware's
 * desktop_view_lock_menu.c. Hardware calls below are mapped to ESP32 APIs. */
typedef enum {
    DesktopLockMenuIndexLefthandedMode,
    DesktopLockMenuIndexSettings,
    DesktopLockMenuIndexDarkMode,
    DesktopLockMenuIndexLock,
    DesktopLockMenuIndexBluetooth,
    DesktopLockMenuIndexMomentum,
    DesktopLockMenuIndexBrightness,
    DesktopLockMenuIndexVolume,
    DesktopLockMenuIndexTotalCount,
} DesktopLockMenuIndex;

// Keep the selected item inside the visible window.
static void lock_menu_scroll_to(uint8_t idx) {
    if(idx < s_top) {
        s_top = idx;
    } else if(idx >= s_top + LOCK_MENU_VISIBLE) {
        s_top = idx - LOCK_MENU_VISIBLE + 1;
    }
}

static void lock_menu_build_items(bool usb_available, bool qflipper_on, bool bt_on, bool wifi_on) {
    s_item_count = 0;

    UNUSED(usb_available);
    UNUSED(qflipper_on);
    UNUSED(bt_on);
    UNUSED(wifi_on);

    /* Web-Filesystem: SoftAP + HTTP file server for the SD; board-independent
     * (WiFi is on every target). */
    s_items[s_item_count++] = (LockMenuItem){"Web-Filesystem", DesktopLockMenuEventWebFs};

}

void desktop_lock_menu_set_callback(
    DesktopLockMenuView* lock_menu,
    DesktopLockMenuViewCallback callback,
    void* context) {
    furi_assert(lock_menu);
    furi_assert(callback);
    lock_menu->callback = callback;
    lock_menu->context = context;
}

void desktop_lock_menu_set_idx(DesktopLockMenuView* lock_menu, uint8_t idx) {
    furi_assert(idx < DesktopLockMenuIndexTotalCount);
    with_view_model(
        lock_menu->view, DesktopLockMenuViewModel * model, { model->idx = idx; }, true);
}

void desktop_lock_menu_set_pin_state(DesktopLockMenuView* lock_menu, bool pin_is_set) {
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        {
            model->pin_is_set = pin_is_set;
            model->lock_popup_index = pin_is_set;
        },
        true);
}

void desktop_lock_menu_set_stealth_mode_state(DesktopLockMenuView* lock_menu, bool stealth_mode) {
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        { model->stealth_mode = stealth_mode; },
        true);
}

void desktop_lock_menu_set_states(
    DesktopLockMenuView* lock_menu,
    bool usb_available,
    bool qflipper_on,
    bool bt_on,
    bool wifi_on) {
    lock_menu->usb_available = usb_available;
    lock_menu->qflipper_on = qflipper_on;
    lock_menu->bt_on = bt_on;
    lock_menu->wifi_on = wifi_on;
    lock_menu_build_items(usb_available, qflipper_on, bt_on, wifi_on);
    /* Index nicht resetten — Caller (refresh nach Toggle) erwartet, dass die
     * Selektion stehen bleibt; bei out-of-range clampen wir, damit der Wechsel
     * vom Master- in den Off-Modus (verliert "Mesh Clients") nicht ins Leere
     * zeigt. */
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        {
            if(model->idx >= s_item_count) model->idx = s_item_count - 1;
            lock_menu_scroll_to(model->idx);
        },
        true);
}

static bool lock_menu_item_enabled(const DesktopLockMenuView* lock_menu, uint8_t idx) {
    if(idx >= s_item_count) return false;
    switch(s_items[idx].event) {
    case DesktopLockMenuEventBluetoothToggle:
        return lock_menu->bt_on;
    case DesktopLockMenuEventWifiToggle:
        return lock_menu->wifi_on;
    case DesktopLockMenuEventQflipperToggle:
        return lock_menu->qflipper_on;
    default:
        return false;
    }
}

static const char* lock_menu_item_short_label(uint8_t idx) {
    switch(s_items[idx].event) {
    case DesktopLockMenuEventBluetoothToggle:
        return "BT";
    case DesktopLockMenuEventWifiToggle:
        return "WiFi";
    case DesktopLockMenuEventQflipperToggle:
        return "qFlip";
    case DesktopLockMenuEventUsbStorage:
        return "USB";
    case DesktopLockMenuEventWebFs:
        return "WebFS";
    case DesktopLockMenuEventMeshClients:
        return "Mesh";
    default:
        return "?";
    }
}

static void desktop_lock_menu_draw_unleashed(Canvas* canvas, DesktopLockMenuViewModel* model) {
    canvas_draw_icon(canvas, -57, 0 + STATUS_BAR_Y_SHIFT, &I_DoorLeft_70x55);
    canvas_draw_icon(canvas, 116, 0 + STATUS_BAR_Y_SHIFT, &I_DoorRight_70x55);

    canvas_set_font(canvas, FontSecondary);
    for(uint8_t row = 0; row < LOCK_MENU_VISIBLE; ++row) {
        uint8_t i = s_top + row;
        if(i >= s_item_count) break;

        canvas_draw_str_aligned(
            canvas,
            64,
            9 + (row * 17) + STATUS_BAR_Y_SHIFT,
            AlignCenter,
            AlignCenter,
            s_items[i].label);

        if(model->idx == i) {
            elements_frame(canvas, 15, 1 + (row * 17) + STATUS_BAR_Y_SHIFT, 98, 15);
        }
    }
}

static void desktop_lock_menu_draw_momentum(
    Canvas* canvas, DesktopLockMenuViewModel* model, DesktopLockMenuView* lock_menu) {
    /* Directly ported layout from Momentum-Firmware: six 28x28 controls and
     * two 12x60 vertical sliders. */
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontBatteryPercent);

    for(size_t i = 0; i < DesktopLockMenuIndexTotalCount; ++i) {
        const bool selected = model->idx == i;
        const bool toggle = i < DesktopLockMenuIndexBrightness;
        int8_t x, y, w, h;
        bool enabled = false;
        uint8_t value = 0;
        const Icon* icon = NULL;

        if(toggle) {
            x = 2 + 32 * (i / 2);
            y = 2 + 32 * (i % 2);
            w = 28;
            h = 28;
        } else {
            x = 98 + 16 * (i % 2);
            y = 2;
            w = 12;
            h = 60;
        }

        switch(i) {
        case DesktopLockMenuIndexLefthandedMode:
            icon = &I_CC_LefthandedMode_16x16;
            enabled = furi_hal_rtc_is_flag_set(FuriHalRtcFlagHandOrient);
            break;
        case DesktopLockMenuIndexSettings:
            icon = &I_CC_Settings_16x16;
            break;
        case DesktopLockMenuIndexDarkMode:
            icon = &I_CC_DarkMode_16x16;
            enabled = lock_menu->dark_mode;
            break;
        case DesktopLockMenuIndexLock:
            icon = &I_CC_Lock_16x16;
            break;
        case DesktopLockMenuIndexBluetooth:
            icon = &I_CC_Bluetooth_16x16;
            enabled = lock_menu->bt_on;
            break;
        case DesktopLockMenuIndexMomentum:
            icon = &I_CC_Momentum_16x16;
            break;
        case DesktopLockMenuIndexBrightness:
            icon = &I_Pin_star_7x7;
            value = 58 - lock_menu->notification->settings.display_brightness * 58;
            break;
        case DesktopLockMenuIndexVolume:
            icon = (model->stealth_mode ||
                    lock_menu->notification->settings.speaker_volume <= 0.0001f) ?
                       &I_Muted_8x8 : &I_Volup_8x6;
            value = 58 - lock_menu->notification->settings.speaker_volume * 58;
            break;
        default:
            break;
        }

        if(selected) elements_bold_rounded_frame(canvas, x - 1, y - 1, w + 1, h + 1);
        else canvas_draw_rframe(canvas, x, y, w, h, 5);

        if(toggle) {
            if(enabled) {
                canvas_draw_rbox(canvas, x, y, w, h, 5);
                canvas_set_color(canvas, ColorWhite);
            }
            canvas_draw_icon(
                canvas,
                x + (w - icon_get_width(icon)) / 2,
                y + (h - icon_get_height(icon)) / 2,
                icon);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_draw_icon(
                canvas,
                x + (w - icon_get_width(icon)) / 2,
                y + (h - icon_get_height(icon)) / 2,
                icon);
            canvas_set_color(canvas, ColorXOR);
            canvas_draw_box(canvas, x + 1, y + 1 + value, w - 2, h - 2 - value);
            canvas_set_color(canvas, selected ? ColorBlack : ColorWhite);
            canvas_draw_dot(canvas, x + 1, y + 1);
            canvas_draw_dot(canvas, x + 1, y + h - 2);
            canvas_draw_dot(canvas, x + w - 2, y + 1);
            canvas_draw_dot(canvas, x + w - 2, y + h - 2);
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_rframe(canvas, x, y, w, h, 5);
        }
    }

    if(model->show_lock_popup) {
        canvas_set_font(canvas, FontSecondary);
        elements_bold_rounded_frame(canvas, 24, 4, 80, 56);
        canvas_draw_str_aligned(canvas, 64, 16, AlignCenter, AlignCenter, "Keypad Lock");
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "PIN Code Lock");
        canvas_draw_str_aligned(canvas, 64, 48, AlignCenter, AlignCenter, "PIN Lock + OFF");
        elements_frame(canvas, 28, 8 + model->lock_popup_index * 16, 72, 15);
    }
}

void desktop_lock_menu_draw_callback(Canvas* canvas, void* model) {
    DesktopLockMenuViewModel* m = model;

    canvas_set_color(canvas, ColorBlack);
    if(lock_screen_get_style() == LockScreenStyleMomentum) {
        desktop_lock_menu_draw_momentum(canvas, m, m->lock_menu);
    } else {
        desktop_lock_menu_draw_unleashed(canvas, m);
    }
}

View* desktop_lock_menu_get_view(DesktopLockMenuView* lock_menu) {
    furi_assert(lock_menu);
    return lock_menu->view;
}

static void desktop_lock_menu_brightness_hold(void* context) {
    DesktopLockMenuView* lock_menu = context;
    bool open_settings = false;
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        {
            if(model->brightness_held && !model->show_lock_popup &&
               model->idx == DesktopLockMenuIndexBrightness) {
                model->brightness_held = false;
                model->brightness_hold_fired = true;
                model->editing_slider = false;
                open_settings = true;
            }
        },
        false);
    if(open_settings) {
        lock_menu->callback(DesktopLockMenuEventScreenSettings, lock_menu->context);
    }
}

static void desktop_lock_menu_exit(void* context) {
    DesktopLockMenuView* lock_menu = context;
    furi_timer_stop(lock_menu->brightness_hold_timer);
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        { model->brightness_held = false; },
        false);
}

bool desktop_lock_menu_input_callback(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    DesktopLockMenuView* lock_menu = context;
    uint8_t idx = 0;
    bool show_lock_popup = false;
    DesktopLockMenuPopupIndex lock_popup_index = 0;
    bool editing_slider = false;
    bool started_slider_edit = false;
    bool finish_slider_edit = false;
    bool slider_dirty = false;
    bool consumed = true;
    const bool momentum_style = lock_screen_get_style() == LockScreenStyleMomentum;
    bool start_brightness_hold = false;
    if(event->type == InputTypePress && event->key == InputKeyOk && momentum_style) {
        with_view_model(
            lock_menu->view,
            DesktopLockMenuViewModel * model,
            {
                model->brightness_hold_fired = false;
                model->brightness_held = !model->show_lock_popup &&
                    model->idx == DesktopLockMenuIndexBrightness;
                start_brightness_hold = model->brightness_held;
            },
            false);
        if(start_brightness_hold)
            furi_timer_start(lock_menu->brightness_hold_timer, furi_ms_to_ticks(2500));
    } else if(event->type == InputTypeLong && event->key == InputKeyOk && momentum_style) {
        bool brightness_held = false;
        with_view_model(
            lock_menu->view,
            DesktopLockMenuViewModel * model,
            { brightness_held = model->brightness_held; },
            false);
        // T-Embed synthesizes Ok Press + Long after INPUT_LONG_PRESS_MS (500 ms),
        // not at physical button-down. Count only the remaining 2000 ms here.
        if(brightness_held)
            furi_timer_start(lock_menu->brightness_hold_timer, furi_ms_to_ticks(2000));
    } else if(event->type == InputTypeRelease || event->key != InputKeyOk) {
        furi_timer_stop(lock_menu->brightness_hold_timer);
        with_view_model(
            lock_menu->view,
            DesktopLockMenuViewModel * model,
            { model->brightness_held = false; },
            false);
    }

    if(!momentum_style) {
        bool update = false;
        with_view_model(
            lock_menu->view,
            DesktopLockMenuViewModel * model,
            {
                if((event->type == InputTypeShort) || (event->type == InputTypeRepeat)) {
                    if(event->key == InputKeyUp || event->key == InputKeyLeft) {
                        model->idx = model->idx == 0 ? s_item_count - 1 : model->idx - 1;
                        update = true;
                    } else if(event->key == InputKeyDown || event->key == InputKeyRight) {
                        model->idx = model->idx >= s_item_count - 1 ? 0 : model->idx + 1;
                        update = true;
                    }
                    if(update) lock_menu_scroll_to(model->idx);
                }
                idx = model->idx;
            },
            update);

        if(event->key == InputKeyOk && event->type == InputTypeShort && idx < s_item_count) {
            lock_menu->callback(s_items[idx].event, lock_menu->context);
        } else if(event->key == InputKeyBack) {
            consumed = false;
        }
        return consumed;
    }

    bool hold_fired = false;
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        { hold_fired = model->brightness_hold_fired; },
        false);
    if(event->key == InputKeyOk && hold_fired) return true;

    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        {
            show_lock_popup = model->show_lock_popup;
            editing_slider = model->editing_slider;
            slider_dirty = model->slider_dirty;
            // The ordinary Long event arrives before 2.5 s. Only our timer
            // opens settings; a Short event enters/exits brightness editing.
            if((event->type == InputTypeShort) || (event->type == InputTypeRepeat)) {
                if(model->show_lock_popup) {
                    if(event->key == InputKeyUp) {
                        model->lock_popup_index = model->lock_popup_index == 0 ?
                                                      DesktopLockMenuPopupIndexMAX - 1 :
                                                      model->lock_popup_index - 1;
                    } else if(event->key == InputKeyDown) {
                        model->lock_popup_index =
                            model->lock_popup_index == DesktopLockMenuPopupIndexMAX - 1 ?
                                0 :
                                model->lock_popup_index + 1;
                    } else if(event->key == InputKeyBack || event->key == InputKeyOk) {
                        model->show_lock_popup = false;
                    }
                } else if(model->editing_slider) {
                    if((event->key == InputKeyBack || event->key == InputKeyOk) &&
                       event->type == InputTypeShort) {
                        model->editing_slider = false;
                        finish_slider_edit = true;
                    }
                } else if(model->idx == DesktopLockMenuIndexLock && event->key == InputKeyOk) {
                    model->show_lock_popup = true;
                } else if(model->idx >= DesktopLockMenuIndexBrightness &&
                          event->key == InputKeyOk && event->type == InputTypeShort) {
                    model->editing_slider = true;
                    started_slider_edit = true;
                } else {
                    /* T-Embed's wheel emits Up/Down.  Walk linearly through
                       every tile and both sliders so no Left/Right buttons
                       are required to reach the second row. */
                    if(event->key == InputKeyUp) {
                        model->idx = model->idx == 0 ?
                                         DesktopLockMenuIndexTotalCount - 1 :
                                         model->idx - 1;
                    } else if(event->key == InputKeyDown) {
                        model->idx = model->idx == DesktopLockMenuIndexTotalCount - 1 ?
                                         0 :
                                         model->idx + 1;
                    } else if(event->key == InputKeyLeft &&
                              model->idx < DesktopLockMenuIndexBrightness) {
                        if(model->idx < DesktopLockMenuIndexBrightness) {
                            model->idx = model->idx < 2 ?
                                             DesktopLockMenuIndexVolume :
                                             model->idx - 2;
                        } else {
                            model->idx--;
                        }
                    } else if(event->key == InputKeyRight &&
                              model->idx < DesktopLockMenuIndexBrightness) {
                        if(model->idx < DesktopLockMenuIndexBrightness) {
                            model->idx = model->idx >= 4 ?
                                             DesktopLockMenuIndexBrightness :
                                             model->idx + 2;
                        } else {
                            model->idx = model->idx >= DesktopLockMenuIndexVolume ?
                                             DesktopLockMenuIndexSettings :
                                             model->idx + 1;
                        }
                    }
                }
            }
            idx = model->idx;
            lock_popup_index = model->lock_popup_index;
        },
        true);

    if(finish_slider_edit && slider_dirty) {
        notification_message_save_settings(lock_menu->notification);
        with_view_model(
            lock_menu->view,
            DesktopLockMenuViewModel * model,
            { model->slider_dirty = false; },
            false);
        slider_dirty = false;
    }

    DesktopEvent desktop_event = 0;
    if(show_lock_popup) {
        if(event->key == InputKeyOk && event->type == InputTypeShort) {
            switch(lock_popup_index) {
            case DesktopLockMenuPopupIndexKeypad:
                desktop_event = DesktopLockMenuEventLockKeypad;
                break;
            case DesktopLockMenuPopupIndexPinCode:
                desktop_event = DesktopLockMenuEventLockPinCode;
                break;
            case DesktopLockMenuPopupIndexPinOff:
                desktop_event = DesktopLockMenuEventLockPinOff;
                break;
            default:
                break;
            }
        }
    } else if(event->key == InputKeyBack && !editing_slider) {
        if(slider_dirty) {
            notification_message_save_settings(lock_menu->notification);
            with_view_model(
                lock_menu->view,
                DesktopLockMenuViewModel * model,
                { model->slider_dirty = false; },
                false);
        }
        consumed = false;
    } else if(event->key == InputKeyOk && event->type == InputTypeShort &&
              !editing_slider && !started_slider_edit) {
        switch(idx) {
        case DesktopLockMenuIndexLefthandedMode:
            if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagHandOrient)) {
                furi_hal_rtc_reset_flag(FuriHalRtcFlagHandOrient);
            } else {
                furi_hal_rtc_set_flag(FuriHalRtcFlagHandOrient);
            }
            break;
        case DesktopLockMenuIndexSettings:
            desktop_event = DesktopLockMenuEventSettings;
            break;
        case DesktopLockMenuIndexDarkMode: {
            uint16_t foreground = furi_hal_display_get_fg_color();
            furi_hal_display_set_fg_color(furi_hal_display_get_bg_color());
            furi_hal_display_set_bg_color(foreground);
            lock_menu->dark_mode = !lock_menu->dark_mode;
            break;
        }
        case DesktopLockMenuIndexBluetooth:
            desktop_event = DesktopLockMenuEventBluetoothToggle;
            break;
        case DesktopLockMenuIndexMomentum:
            desktop_event = DesktopLockMenuEventMomentum;
            break;
        case DesktopLockMenuIndexBrightness:
            break;
        case DesktopLockMenuIndexVolume:
            break;
        default:
            break;
        }
    } else if(idx >= DesktopLockMenuIndexBrightness &&
              (event->type == InputTypeShort || event->type == InputTypeRepeat)) {
        int8_t offset = 0;
        if(editing_slider && (event->key == InputKeyUp || event->key == InputKeyRight)) offset = 1;
        else if(editing_slider && (event->key == InputKeyDown || event->key == InputKeyLeft)) offset = -1;
        else if(!editing_slider && event->key == InputKeyRight) offset = 1;
        else if(!editing_slider && event->key == InputKeyLeft) offset = -1;

        if(offset) {
            float* value = idx == DesktopLockMenuIndexBrightness ?
                               &lock_menu->notification->settings.display_brightness :
                               &lock_menu->notification->settings.speaker_volume;
            *value += 0.05f * offset;
            if(*value < 0.0f) *value = 0.0f;
            if(*value > 1.0f) *value = 1.0f;
            with_view_model(
                lock_menu->view,
                DesktopLockMenuViewModel * model,
                { model->slider_dirty = true; },
                true);
            if(idx == DesktopLockMenuIndexBrightness) {
                /* Immediate hardware update; no storage or notification queue
                   round-trip on every encoder detent. */
                furi_hal_display_set_backlight((uint8_t)(*value * 255.0f));
            }
        }
    }

    if(desktop_event) {
        lock_menu->callback(desktop_event, lock_menu->context);
    }

    return consumed;
}

DesktopLockMenuView* desktop_lock_menu_alloc(void) {
    DesktopLockMenuView* lock_menu = malloc(sizeof(DesktopLockMenuView));
    memset(lock_menu, 0, sizeof(DesktopLockMenuView));
    lock_menu->notification = furi_record_open(RECORD_NOTIFICATION);
    lock_menu->brightness_hold_timer = furi_timer_alloc(
        desktop_lock_menu_brightness_hold, FuriTimerTypeOnce, lock_menu);
    lock_menu->view = view_alloc();
    view_allocate_model(lock_menu->view, ViewModelTypeLocking, sizeof(DesktopLockMenuViewModel));
    view_set_context(lock_menu->view, lock_menu);
    view_set_draw_callback(lock_menu->view, (ViewDrawCallback)desktop_lock_menu_draw_callback);
    view_set_input_callback(lock_menu->view, desktop_lock_menu_input_callback);
    view_set_exit_callback(lock_menu->view, desktop_lock_menu_exit);
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        { model->lock_menu = lock_menu; },
        false);

    // Default until the scene fills in real states on enter.
    lock_menu_build_items(false, false, false, false);

    return lock_menu;
}

void desktop_lock_menu_free(DesktopLockMenuView* lock_menu_view) {
    furi_assert(lock_menu_view);

    furi_timer_stop(lock_menu_view->brightness_hold_timer);
    furi_timer_free(lock_menu_view->brightness_hold_timer);
    view_free(lock_menu_view->view);
    furi_record_close(RECORD_NOTIFICATION);
    free(lock_menu_view);
}
