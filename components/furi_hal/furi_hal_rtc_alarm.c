#include "furi_hal_rtc.h"
#include <furi.h>
#include <nvs.h>

/* ESP32 software alarm. No claim of waking from battery ship mode/deep sleep. */
static FuriMutex* alarm_mutex;
static FuriTimer* alarm_timer;
static uint16_t alarm_minutes;
static bool alarm_enabled;
static uint32_t alarm_last_minute = UINT32_MAX;
static FuriHalRtcAlarmCallback alarm_callback;
static void* alarm_context;

static void rtc_alarm_init(void) {
    if(alarm_mutex) return;
    alarm_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    nvs_handle_t handle;
    if(nvs_open("rtc_alarm", NVS_READONLY, &handle) == ESP_OK) {
        uint8_t enabled = 0;
        nvs_get_u16(handle, "minutes", &alarm_minutes);
        nvs_get_u8(handle, "enabled", &enabled);
        alarm_enabled = enabled && alarm_minutes < 24 * 60;
        if(alarm_minutes >= 24 * 60) alarm_minutes = 0;
        nvs_close(handle);
    }
}

bool furi_hal_rtc_get_alarm(DateTime* alarm) {
    rtc_alarm_init();
    furi_mutex_acquire(alarm_mutex, FuriWaitForever);
    if(alarm) {
        *alarm = (DateTime){.year = 2000, .month = 1, .day = 1, .weekday = 6,
                            .hour = alarm_minutes / 60, .minute = alarm_minutes % 60};
    }
    bool enabled = alarm_enabled;
    furi_mutex_release(alarm_mutex);
    return enabled;
}

void furi_hal_rtc_set_alarm(const DateTime* alarm, bool enabled) {
    rtc_alarm_init();
    if(alarm && (alarm->hour >= 24 || alarm->minute >= 60)) return;
    furi_mutex_acquire(alarm_mutex, FuriWaitForever);
    if(alarm) alarm_minutes = alarm->hour * 60 + alarm->minute;
    alarm_enabled = enabled;
    alarm_last_minute = UINT32_MAX;
    nvs_handle_t handle;
    if(nvs_open("rtc_alarm", NVS_READWRITE, &handle) == ESP_OK) {
        esp_err_t status = nvs_set_u16(handle, "minutes", alarm_minutes);
        if(status == ESP_OK) status = nvs_set_u8(handle, "enabled", alarm_enabled);
        if(status == ESP_OK) status = nvs_commit(handle);
        if(status != ESP_OK) FURI_LOG_E("RtcAlarm", "Could not save alarm: %d", status);
        nvs_close(handle);
    } else {
        FURI_LOG_E("RtcAlarm", "Alarm storage unavailable");
    }
    furi_mutex_release(alarm_mutex);
}

static void rtc_alarm_poll(void* context) {
    UNUSED(context);
    DateTime now;
    furi_hal_rtc_get_datetime(&now);
    uint32_t minute = furi_hal_rtc_get_timestamp() / 60;
    FuriHalRtcAlarmCallback callback = NULL;
    void* callback_context = NULL;
    furi_mutex_acquire(alarm_mutex, FuriWaitForever);
    if(alarm_enabled && alarm_callback && alarm_minutes == now.hour * 60 + now.minute &&
       minute != alarm_last_minute) {
        alarm_last_minute = minute;
        callback = alarm_callback;
        callback_context = alarm_context;
    }
    furi_mutex_release(alarm_mutex);
    if(callback) callback(callback_context);
}

void furi_hal_rtc_set_alarm_callback(FuriHalRtcAlarmCallback callback, void* context) {
    rtc_alarm_init();
    furi_mutex_acquire(alarm_mutex, FuriWaitForever);
    alarm_callback = callback;
    alarm_context = context;
    furi_mutex_release(alarm_mutex);
    if(!alarm_timer) alarm_timer = furi_timer_alloc(rtc_alarm_poll, FuriTimerTypePeriodic, NULL);
    if(callback) furi_timer_start(alarm_timer, furi_ms_to_ticks(1000));
    else furi_timer_stop(alarm_timer);
}
