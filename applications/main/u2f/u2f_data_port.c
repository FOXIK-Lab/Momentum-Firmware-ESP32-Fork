/* ESP32 software U2F identity. Deliberately bypasses this port's no-op
 * crypto-enclave compatibility layer. NVS is persistent but NOT a secure
 * element: the app warns before use, and must never be the only login key. */
#include "u2f_data.h"
#include <furi_hal_random.h>
#include <furi_hal_u2f_random.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/platform_util.h>
#include <nvs.h>
#include <string.h>

#define IDENTITY_MAGIC 0x55324631UL
#define CERT_CAPACITY 768
typedef struct {
    uint32_t magic;
    uint32_t counter;
    uint32_t cert_length;
    uint8_t device_key[32];
    uint8_t cert_key[32];
    uint8_t certificate[CERT_CAPACITY];
} U2fIdentity;
static U2fIdentity identity;
static bool loaded;

static int random_bytes(void* ctx, unsigned char* output, size_t length) {
    UNUSED(ctx);
    return furi_hal_u2f_random_bytes(NULL, output, length);
}

static bool identity_generate(void) {
    bool ok = false;
    mbedtls_pk_context key;
    mbedtls_x509write_cert cert;
    uint8_t serial_bytes[16];
    uint8_t der[CERT_CAPACITY];
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&cert);
    memset(&identity, 0, sizeof(identity));
    do {
        if(mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) break;
        if(mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), random_bytes, NULL)) break;
        if(mbedtls_mpi_write_binary(&mbedtls_pk_ec(key)->MBEDTLS_PRIVATE(d), identity.cert_key, 32)) break;
        if(random_bytes(NULL, identity.device_key, 32)) break;
        if(random_bytes(NULL, serial_bytes, sizeof(serial_bytes))) break;
        serial_bytes[0] &= 0x7f;
        serial_bytes[0] |= 1;
        mbedtls_x509write_crt_set_version(&cert, MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&cert, MBEDTLS_MD_SHA256);
        mbedtls_x509write_crt_set_subject_key(&cert, &key);
        mbedtls_x509write_crt_set_issuer_key(&cert, &key);
        if(mbedtls_x509write_crt_set_serial_raw(&cert, serial_bytes, sizeof(serial_bytes))) break;
        const char* name = "CN=T-Embed Experimental U2F,O=Software Authenticator";
        if(mbedtls_x509write_crt_set_subject_name(&cert, name)) break;
        if(mbedtls_x509write_crt_set_issuer_name(&cert, name)) break;
        if(mbedtls_x509write_crt_set_validity(&cert, "20200101000000", "20991231235959")) break;
        if(mbedtls_x509write_crt_set_basic_constraints(&cert, 0, -1)) break;
        if(mbedtls_x509write_crt_set_key_usage(&cert, MBEDTLS_X509_KU_DIGITAL_SIGNATURE)) break;
        int length = mbedtls_x509write_crt_der(&cert, der, sizeof(der), random_bytes, NULL);
        if(length <= 0 || length > CERT_CAPACITY) break;
        identity.cert_length = length;
        memcpy(identity.certificate, der + sizeof(der) - length, length);
        identity.magic = IDENTITY_MAGIC;
        ok = true;
    } while(false);
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&cert);
    mbedtls_platform_zeroize(der, sizeof(der));
    return ok;
}

static bool identity_open(void) {
    if(loaded) return true;
    nvs_handle_t handle;
    if(nvs_open("tembed_u2f", NVS_READWRITE, &handle) != ESP_OK) return false;
    size_t length = sizeof(identity);
    esp_err_t err = nvs_get_blob(handle, "identity", &identity, &length);
    bool ok = false;
    if(err == ESP_ERR_NVS_NOT_FOUND) {
        /* Only a genuinely new identity may be generated. Corrupt/partial
         * stored data must fail closed, never silently replace account keys. */
        ok = identity_generate() &&
             nvs_set_blob(handle, "identity", &identity, sizeof(identity)) == ESP_OK &&
             nvs_commit(handle) == ESP_OK;
    } else if(err == ESP_OK && length == sizeof(identity) &&
              identity.magic == IDENTITY_MAGIC && identity.cert_length > 0 &&
              identity.cert_length <= CERT_CAPACITY) {
        mbedtls_x509_crt cert;
        mbedtls_x509_crt_init(&cert);
        ok = mbedtls_x509_crt_parse_der(&cert, identity.certificate, identity.cert_length) == 0;
        mbedtls_x509_crt_free(&cert);
    }
    nvs_close(handle);
    loaded = ok;
    if(!ok) mbedtls_platform_zeroize(&identity, sizeof(identity));
    return ok;
}

bool u2f_data_check(bool cert_only) {
    UNUSED(cert_only);
    return identity_open();
}
bool u2f_data_cert_check(void) { return identity_open(); }
uint32_t u2f_data_cert_load(uint8_t* cert) {
    if(!cert || !identity_open()) return 0;
    memcpy(cert, identity.certificate, identity.cert_length);
    return identity.cert_length;
}
bool u2f_data_cert_key_load(uint8_t* key) {
    if(!key || !identity_open()) return false;
    memcpy(key, identity.cert_key, 32);
    return true;
}
bool u2f_data_key_load(uint8_t* key) {
    if(!key || !identity_open()) return false;
    memcpy(key, identity.device_key, 32);
    return true;
}
bool u2f_data_key_generate(uint8_t* key) {
    /* The old application's load-error fallback must not rotate an identity. */
    return u2f_data_key_load(key);
}
bool u2f_data_cnt_read(uint32_t* counter) {
    if(!counter || !identity_open()) return false;
    *counter = identity.counter;
    return true;
}
bool u2f_data_cnt_write(uint32_t counter) {
    if(!identity_open() || counter < identity.counter) return false;
    nvs_handle_t handle;
    if(nvs_open("tembed_u2f", NVS_READWRITE, &handle) != ESP_OK) return false;
    uint32_t previous = identity.counter;
    identity.counter = counter;
    bool ok = nvs_set_blob(handle, "identity", &identity, sizeof(identity)) == ESP_OK &&
              nvs_commit(handle) == ESP_OK;
    nvs_close(handle);
    if(!ok) identity.counter = previous;
    return ok;
}

void u2f_data_close(void) {
    mbedtls_platform_zeroize(&identity, sizeof(identity));
    loaded = false;
}
