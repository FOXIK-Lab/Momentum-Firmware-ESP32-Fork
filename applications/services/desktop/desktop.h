#pragma once

#include <furi.h>

#include "desktop_settings.h"

#define RECORD_DESKTOP "desktop"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Desktop Desktop;

typedef struct {
    bool locked;
} DesktopStatus;

bool desktop_api_is_locked(Desktop* instance);

void desktop_api_unlock(Desktop* instance);

FuriPubSub* desktop_api_get_status_pubsub(Desktop* instance);

void desktop_api_get_settings(Desktop* instance, DesktopSettings* settings);

void desktop_api_set_settings(Desktop* instance, const DesktopSettings* settings);

/** Open device tools that were moved out of the Unleashed lock menu. */
void desktop_api_open_usb_storage(Desktop* instance);
void desktop_api_open_mesh_clients(Desktop* instance);

/** Runtime qFlipper bridge control and its persistent boot preference. */
bool desktop_api_qflipper_is_enabled(void);
void desktop_api_set_qflipper_enabled(Desktop* instance, bool enabled);
bool desktop_api_qflipper_on_boot(void);
void desktop_api_set_qflipper_on_boot(bool enabled);

#ifdef __cplusplus
}
#endif
