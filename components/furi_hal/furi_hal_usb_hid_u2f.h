#pragma once
#include <stdint.h>
#include <stdbool.h>

#define HID_U2F_PACKET_LEN 64
#define HID_U2F_REPORT_DESC_LEN 34
typedef enum {
    HidU2fConnected,
    HidU2fDisconnected,
    HidU2fRequest,
} HidU2fEvent;
typedef void (*U2fHidCallback)(HidU2fEvent event, void* context);

void furi_hal_hid_u2f_set_callback(U2fHidCallback callback, void* context);
uint32_t furi_hal_hid_u2f_get_request(uint8_t* packet);
bool furi_hal_hid_u2f_send_response(const uint8_t* packet, uint8_t length);
bool furi_hal_hid_u2f_start(void);
void furi_hal_hid_u2f_stop(void);
const uint8_t* furi_hal_hid_u2f_report_descriptor(void);
void furi_hal_hid_u2f_rx(const uint8_t* packet, uint16_t length);
void furi_hal_hid_u2f_mount(bool mounted);
