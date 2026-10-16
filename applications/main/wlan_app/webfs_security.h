#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* The SD backend accepts backslashes and redundant separators. Reject them
 * rather than comparing a non-canonical spelling with the protected root. */
static inline bool webfs_path_allowed(const char* path, bool allow_root) {
    if(!path || strncmp(path, "/ext", 4) != 0) return false;
    if(path[4] == '\0') return allow_root;
    if(path[4] != '/' || path[5] == '\0') return false;
    for(const char* segment = path + 5; *segment;) {
        const char* end = strchr(segment, '/');
        size_t length = end ? (size_t)(end - segment) : strlen(segment);
        if(length == 0 || (length == 1 && segment[0] == '.') ||
           (length == 2 && segment[0] == '.' && segment[1] == '.')) return false;
        for(size_t i = 0; i < length; i++) {
            unsigned char c = (unsigned char)segment[i];
            if(c < 0x20 || c == 0x7f || c == '\\' || c == ':' || c == '"') return false;
        }
        if(!end) return true;
        segment = end + 1;
        if(!*segment) return false;
    }
    return false;
}

static inline int webfs_hex(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Fail on malformed escapes, embedded NUL and truncation; never act on a
 * different prefix of the path the client actually supplied. */
static inline bool webfs_decode_path(const char* in, char* out, size_t capacity) {
    if(!in || !out || capacity == 0) return false;
    size_t written = 0;
    while(*in) {
        unsigned char c = (unsigned char)*in++;
        if(c == '%') {
            if(!in[0] || !in[1]) return false;
            int hi = webfs_hex(in[0]), lo = webfs_hex(in[1]);
            if(hi < 0 || lo < 0) return false;
            c = (unsigned char)((hi << 4) | lo);
            in += 2;
        }
        if(c == 0 || c < 0x20 || c == 0x7f || written + 1 >= capacity) return false;
        out[written++] = (char)c;
    }
    out[written] = '\0';
    return true;
}
