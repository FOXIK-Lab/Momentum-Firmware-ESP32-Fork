#pragma once

#include <stddef.h>
#include <icon.h>

typedef struct { const Icon* icon; const char* path; } AssetIconPath;
extern const AssetIconPath ASSET_ICON_PATHS[];
extern const size_t ASSET_ICON_PATHS_COUNT;
