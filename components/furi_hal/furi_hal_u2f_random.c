#include "furi_hal_u2f_random.h"
#include <furi.h>
#include <mbedtls/ctr_drbg.h>
#include <bootloader_random.h>
#include <esp_random.h>
#include <limits.h>

static mbedtls_ctr_drbg_context drbg;
static FuriMutex* mutex;
static bool ready;
static bool seeding;

static int boot_entropy(void* context, unsigned char* output, size_t length) {
    UNUSED(context);
    /* Only called before ADC/radio drivers start. Never reconfigure the ADC
     * while the device's battery monitor or radios are using it. */
    if(!seeding) return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    esp_fill_random(output, length);
    return 0;
}

void furi_hal_u2f_random_init(void) {
    mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    mbedtls_ctr_drbg_init(&drbg);
    bootloader_random_enable();
    seeding = true;
    const unsigned char purpose[] = "T-Embed U2F boot CSPRNG";
    ready = mbedtls_ctr_drbg_seed(&drbg, boot_entropy, NULL, purpose, sizeof(purpose)) == 0;
    seeding = false;
    bootloader_random_disable();
    mbedtls_ctr_drbg_set_reseed_interval(&drbg, INT_MAX);
}

int furi_hal_u2f_random_bytes(void* context, unsigned char* output, size_t length) {
    UNUSED(context);
    if(!ready || !mutex) return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    furi_mutex_acquire(mutex, FuriWaitForever);
    int status = mbedtls_ctr_drbg_random(&drbg, output, length);
    furi_mutex_release(mutex);
    return status;
}
