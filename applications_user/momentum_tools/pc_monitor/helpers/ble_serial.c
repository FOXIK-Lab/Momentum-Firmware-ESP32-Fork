/* ESP32 backend for the PC Monitor serial profile. Uses the existing
 * Flipper-compatible BLE serial service; no STM32 controller calls. */
#include "ble_serial.h"
#include <furi.h>
#include <furi_hal.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    FuriHalBleProfileBase base;
    BleSerial* serial;
} PcSerialProfile;

static void pc_serial_config(GapConfig* config, FuriHalBleProfileParams parameters) {
    BleProfileSerialParams* params = parameters;
    memset(config, 0, sizeof(*config));
    config->bonding_mode = true;
    config->pairing_method = GapPairingPinCodeDisplayOnly;
    const uint8_t* mac = furi_hal_version_get_ble_mac();
    memcpy(config->mac_address, mac, 6);
    config->mac_address[2]++;
    if(params) {
        config->mac_address[0] ^= params->mac_xor;
        config->mac_address[1] ^= params->mac_xor >> 8;
    }
    snprintf(config->adv_name, sizeof(config->adv_name), "%s %s",
        params && params->device_name_prefix ? params->device_name_prefix : "PC Mon",
        furi_hal_version_get_name_ptr());
}

static FuriHalBleProfileBase* pc_serial_start(FuriHalBleProfileParams params) {
    GapConfig gap;
    pc_serial_config(&gap, params);
    BleSerialConfig config = {0};
    strlcpy(config.device_name, gap.adv_name, sizeof(config.device_name));
    for(size_t i = 0; i < 6; i++) config.mac[i] = gap.mac_address[5 - i];
    config.bonding = gap.bonding_mode;
    config.pairing = BleSerialPairingDisplayOnly;
    PcSerialProfile* profile = calloc(1, sizeof(*profile));
    if(!profile) return NULL;
    profile->base.config = ble_profile_serial;
    profile->serial = ble_serial_alloc(&config);
    if(!profile->serial) { free(profile); return NULL; }
    return &profile->base;
}

static void pc_serial_stop(FuriHalBleProfileBase* base) {
    PcSerialProfile* profile = (PcSerialProfile*)base;
    if(!profile) return;
    ble_serial_free(profile->serial);
    free(profile);
}

static const FuriHalBleProfileTemplate pc_serial_template = {
    .start = pc_serial_start,
    .stop = pc_serial_stop,
    .get_gap_config = pc_serial_config,
};
const FuriHalBleProfileTemplate* const ble_profile_serial = &pc_serial_template;

void ble_profile_serial_set_event_callback(FuriHalBleProfileBase* base,
    uint16_t size, FuriHalBtSerialCallback callback, void* context) {
    furi_check(base && base->config == ble_profile_serial);
    ble_serial_set_event_callback(((PcSerialProfile*)base)->serial, size, callback, context);
}

bool ble_profile_serial_tx(FuriHalBleProfileBase* base, uint8_t* data, uint16_t size) {
    furi_check(base && base->config == ble_profile_serial);
    if(size > BLE_PROFILE_SERIAL_PACKET_SIZE_MAX) return false;
    return ble_serial_tx(((PcSerialProfile*)base)->serial, data, size);
}
