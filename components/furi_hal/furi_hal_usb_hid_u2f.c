#include "furi_hal_usb_hid_u2f.h"
#include "furi_hal_usb_tinyusb_composite.h"
#include <furi.h>
#include <tusb.h>
#include <string.h>
#include <stdatomic.h>

/* FIDO U2F usage page, 64-byte input/output, no report ID. The foreground
 * app temporarily selects U2F on HID instance 0 instead of keyboard HID. */
static const uint8_t report_descriptor[] = {
    0x06, 0xd0, 0xf1, 0x09, 0x01, 0xa1, 0x01,
    0x09, 0x20, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, 0x40, 0x81, 0x02,
    0x09, 0x21, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, 0x40, 0x91, 0x02, 0xc0,
};
_Static_assert(sizeof(report_descriptor) == HID_U2F_REPORT_DESC_LEN, "U2F descriptor size");
static FuriMessageQueue* requests;
static FuriMutex* callback_mutex;
static U2fHidCallback callback;
static void* callback_context;
static atomic_bool active;
static bool owns_composite;

const uint8_t* furi_hal_hid_u2f_report_descriptor(void) {
    return report_descriptor;
}

bool furi_hal_hid_u2f_start(void) {
    if(active) return true;
    if(!requests) requests = furi_message_queue_alloc(32, HID_U2F_PACKET_LEN);
    if(!callback_mutex) callback_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_message_queue_reset(requests);
    owns_composite = !furi_hal_usb_composite_is_installed();
    if(!owns_composite && !furi_hal_usb_composite_uninstall()) return false;
    if(!furi_hal_usb_composite_select_u2f(true) ||
       !furi_hal_usb_composite_install(0, 0, NULL, NULL)) {
        furi_hal_usb_composite_select_u2f(false);
        if(!owns_composite) furi_hal_usb_composite_install(0, 0, NULL, NULL);
        return false;
    }
    active = true;
    return true;
}

void furi_hal_hid_u2f_stop(void) {
    furi_hal_hid_u2f_set_callback(NULL, NULL);
    active = false;
    furi_hal_usb_composite_uninstall();
    furi_hal_usb_composite_select_u2f(false);
    if(!owns_composite) furi_hal_usb_composite_install(0, 0, NULL, NULL);
    owns_composite = false;
}

void furi_hal_hid_u2f_set_callback(U2fHidCallback cb, void* context) {
    if(!callback_mutex) return;
    furi_mutex_acquire(callback_mutex, FuriWaitForever);
    callback = cb;
    callback_context = context;
    if(cb && active && tud_mounted()) cb(HidU2fConnected, context);
    furi_mutex_release(callback_mutex);
}

void furi_hal_hid_u2f_mount(bool mounted) {
    if(!callback_mutex) return;
    furi_mutex_acquire(callback_mutex, FuriWaitForever);
    if(active && callback) callback(mounted ? HidU2fConnected : HidU2fDisconnected,
                                    callback_context);
    furi_mutex_release(callback_mutex);
}

void furi_hal_hid_u2f_rx(const uint8_t* packet, uint16_t length) {
    if(!callback_mutex || length != HID_U2F_PACKET_LEN) return;
    furi_mutex_acquire(callback_mutex, FuriWaitForever);
    if(active && callback && furi_message_queue_put(requests, packet, 0) == FuriStatusOk) {
        callback(HidU2fRequest, callback_context);
    }
    furi_mutex_release(callback_mutex);
}

uint32_t furi_hal_hid_u2f_get_request(uint8_t* packet) {
    return requests && furi_message_queue_get(requests, packet, 0) == FuriStatusOk ?
               HID_U2F_PACKET_LEN : 0;
}

bool furi_hal_hid_u2f_send_response(const uint8_t* packet, uint8_t length) {
    if(length != HID_U2F_PACKET_LEN) return false;
    uint32_t deadline = furi_get_tick() + furi_ms_to_ticks(200);
    while(active && tud_mounted() && !tud_hid_n_ready(0)) {
        if((int32_t)(furi_get_tick() - deadline) >= 0) return false;
        furi_delay_ms(1);
    }
    return active && tud_mounted() && tud_hid_n_report(0, 0, packet, length);
}
