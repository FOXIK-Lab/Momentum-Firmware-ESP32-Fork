#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <furi.h>

/* App-local compatibility: no change to the existing FAP ABI. */
float fabsf(float x) { return x < 0.0f ? -x : x; }
float roundf(float x) {
    if(!isfinite(x) || fabsf(x) >= 8388608.0f) return x;
    return (float)(int)(x < 0.0f ? x - 0.5f : x + 0.5f);
}
void bzero(void* ptr, size_t length) { memset(ptr, 0, length); }

/* C++ allocation helpers for embedded games, built without exceptions. */
void* games_new(size_t size) __asm__("_Znwj");
void* games_new(size_t size) {
    void* result = malloc(size ? size : 1);
    furi_check(result);
    return result;
}
void games_delete(void* ptr, size_t size) __asm__("_ZdlPvj");
void games_delete(void* ptr, size_t size) { (void)size; free(ptr); }
void games_length_error(const char* reason) __asm__("_ZSt20__throw_length_errorPKc");
void games_length_error(const char* reason) { furi_crash(reason); }
void games_array_error(void) __asm__("_ZSt28__throw_bad_array_new_lengthv");
void games_array_error(void) { furi_crash("Invalid game array length"); }
void __cxa_pure_virtual(void) { furi_crash("Pure virtual game call"); }
