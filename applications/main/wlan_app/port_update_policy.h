#pragma once

/* No anonymous release feed exists while this repository is private. Never
 * install upstream Sor3nt binaries over this custom Momentum port. Enable only
 * after a dedicated, verified release process has been configured. */
#define MOMENTUM_PORT_NETWORK_UPDATES_ENABLED 0
#define MOMENTUM_PORT_RELEASE_URL \
    "https://github.com/Filip1x3/Momentum-Firmware-ESP32-Port/releases/latest/download"
