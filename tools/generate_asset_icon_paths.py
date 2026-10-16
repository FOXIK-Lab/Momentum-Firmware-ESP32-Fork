"""Generate the runtime icon/path table used by Momentum asset packs."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ICONS = ROOT.parent / "Momentum-Firmware" / "assets" / "icons"
HEADER = ROOT / "components" / "assets" / "assets_icons.h"
OUT_H = ROOT / "components" / "assets" / "assets_icon_paths.h"
OUT_C = ROOT / "components" / "assets" / "assets_icon_paths.c"

symbols = set(re.findall(r"extern const Icon ([A-Za-z0-9_]+);", HEADER.read_text()))
paths = {}
for png in SOURCE_ICONS.rglob("*.png"):
    rel = png.relative_to(SOURCE_ICONS).with_suffix("").as_posix()
    stem = png.stem
    for prefix in ("I_", "A_"):
        symbol = prefix + stem
        if symbol in symbols:
            paths.setdefault(symbol, rel)

# The ESP32 Passport still uses stock, level-specific portraits. Momentum packs
# have one portrait per mood instead. Apply that portrait at every stock level,
# with the old asset filename retained as a fallback for older packs.
passport_fallbacks = {}
for mood in ("happy", "okay", "bad"):
    for level in range(1, 4):
        symbol = f"I_passport_{mood}{level}_46x49"
        if symbol in symbols:
            paths[symbol] = f"Passport/passport_{mood}_46x49"
            passport_fallbacks[symbol] = f"Passport/passport_{mood}{level}_46x49"

OUT_H.write_text(
    "#pragma once\n\n#include <stddef.h>\n#include <icon.h>\n\n"
    "typedef struct { const Icon* icon; const char* path; } AssetIconPath;\n"
    "extern const AssetIconPath ASSET_ICON_PATHS[];\n"
    "extern const size_t ASSET_ICON_PATHS_COUNT;\n"
)

rows = "\n".join(
    f'    {{&{symbol}, "{path}"}},' for symbol, path in sorted(paths.items())
)
rows += "\n" + "\n".join(
    f'    {{&{symbol}, "{path}"}},' for symbol, path in sorted(passport_fallbacks.items())
)
OUT_C.write_text(
    '#include "assets_icon_paths.h"\n#include "assets_icons.h"\n\n'
    f"const AssetIconPath ASSET_ICON_PATHS[] = {{\n{rows}\n}};\n"
    "const size_t ASSET_ICON_PATHS_COUNT = "
    "sizeof(ASSET_ICON_PATHS) / sizeof(ASSET_ICON_PATHS[0]);\n"
)
print(f"generated {len(paths)} icon paths")
