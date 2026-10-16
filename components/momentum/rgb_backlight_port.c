#include "rgb_backlight.h"

/* ESP32/T-Embed implementation of Momentum's RGB-backlight state. The
 * original Flipper driver talks to an SK6805 hardware modification, which the
 * T-Embed does not contain; keeping the same API preserves the original
 * Momentum scenes without pretending that unavailable LEDs exist. */
static RgbColor colors[3] = {{{255, 69, 0}}, {{255, 69, 0}}, {{255, 69, 0}}};
static RGBBacklightRainbowMode rainbow_mode = RGBBacklightRainbowModeOff;
static uint8_t rainbow_speed = 1;
static uint32_t rainbow_interval = 500;
static uint8_t rainbow_saturation = 255;

void rgb_backlight_load_settings(bool enabled) { UNUSED(enabled); }
void rgb_backlight_save_settings(void) {}
void rgb_backlight_set_color(uint8_t index, const RgbColor* color) {
    if(index < COUNT_OF(colors)) colors[index] = *color;
}
void rgb_backlight_get_color(uint8_t index, RgbColor* color) {
    if(index < COUNT_OF(colors)) *color = colors[index];
}
void rgb_backlight_set_rainbow_mode(RGBBacklightRainbowMode mode) { rainbow_mode = mode; }
RGBBacklightRainbowMode rgb_backlight_get_rainbow_mode(void) { return rainbow_mode; }
void rgb_backlight_set_rainbow_speed(uint8_t speed) { rainbow_speed = speed; }
uint8_t rgb_backlight_get_rainbow_speed(void) { return rainbow_speed; }
void rgb_backlight_set_rainbow_interval(uint32_t interval) { rainbow_interval = interval; }
uint32_t rgb_backlight_get_rainbow_interval(void) { return rainbow_interval; }
void rgb_backlight_set_rainbow_saturation(uint8_t saturation) { rainbow_saturation = saturation; }
uint8_t rgb_backlight_get_rainbow_saturation(void) { return rainbow_saturation; }
void rgb_backlight_reconfigure(bool enabled) { UNUSED(enabled); }
void rgb_backlight_update(uint8_t brightness, bool forced) {
    UNUSED(brightness);
    UNUSED(forced);
}
