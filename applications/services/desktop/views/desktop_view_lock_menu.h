#pragma once

#include <gui/view.h>
#include <notification/notification_app.h>
#include "desktop_events.h"

#define HINT_TIMEOUT 2

typedef struct DesktopLockMenuView DesktopLockMenuView;

typedef void (*DesktopLockMenuViewCallback)(DesktopEvent event, void* context);

struct DesktopLockMenuView {
    View* view;
    DesktopLockMenuViewCallback callback;
    void* context;
    NotificationApp* notification;
    FuriTimer* brightness_hold_timer;
    bool usb_available;
    bool qflipper_on;
    bool bt_on;
    bool wifi_on;
    bool dark_mode;
};

typedef enum {
    DesktopLockMenuPopupIndexKeypad,
    DesktopLockMenuPopupIndexPinCode,
    DesktopLockMenuPopupIndexPinOff,
    DesktopLockMenuPopupIndexMAX,
} DesktopLockMenuPopupIndex;

typedef struct {
    uint8_t idx;
    bool editing_slider;
    bool slider_dirty;
    bool brightness_held;
    bool brightness_hold_fired;
    bool pin_is_set;
    bool stealth_mode;
    bool show_lock_popup;
    DesktopLockMenuPopupIndex lock_popup_index;
    DesktopLockMenuView* lock_menu;
} DesktopLockMenuViewModel;

void desktop_lock_menu_set_callback(
    DesktopLockMenuView* lock_menu,
    DesktopLockMenuViewCallback callback,
    void* context);

View* desktop_lock_menu_get_view(DesktopLockMenuView* lock_menu);
void desktop_lock_menu_set_idx(DesktopLockMenuView* lock_menu, uint8_t idx);
void desktop_lock_menu_set_pin_state(DesktopLockMenuView* lock_menu, bool pin_is_set);
void desktop_lock_menu_set_stealth_mode_state(DesktopLockMenuView* lock_menu, bool stealth_mode);

/** Rebuild the menu items from the current toggle states and reset the
 *  selection. `usb_available` gates the qFlipper / USB-Storage entries (USB-OTG
 *  is ESP32-S3/S2 only). "Mesh Clients" ist immer dabei (T-Embed ist immer
 *  Master). */
void desktop_lock_menu_set_states(
    DesktopLockMenuView* lock_menu,
    bool usb_available,
    bool qflipper_on,
    bool bt_on,
    bool wifi_on);

DesktopLockMenuView* desktop_lock_menu_alloc(void);
void desktop_lock_menu_free(DesktopLockMenuView* lock_menu);
