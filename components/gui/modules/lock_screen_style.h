#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Selects the T-Embed lock-screen control-center layout. */
typedef enum {
    LockScreenStyleMomentum,
    LockScreenStyleUnleashed,
    LockScreenStyleCount,
} LockScreenStyle;

void lock_screen_set_style(LockScreenStyle style);
LockScreenStyle lock_screen_get_style(void);

#ifdef __cplusplus
}
#endif
