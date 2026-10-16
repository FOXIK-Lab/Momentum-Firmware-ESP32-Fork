"""Print lossless C icon data for an original Momentum lockscreen PNG."""
import sys
from PIL import Image
image = Image.open(sys.argv[1]).convert('RGBA')
assert image.size == (128, 64)
data = [0]
for y in range(64):
    for block in range(16):
        value = 0
        for bit in range(8):
            r, g, b, alpha = image.getpixel((block * 8 + bit, y))
            if alpha and r + g + b < 384:
                value |= 1 << bit
        data.append(value)
print('/* Lossless original Momentum Interface/Lockscreen.png (GPL-3.0). */')
print('#include "momentum_lock_icon.h"\n#include <gui/icon_i.h>')
print('static const uint8_t lock_data[] = {')
for offset in range(0, len(data), 16):
    print('    ' + ','.join(f'0x{v:02x}' for v in data[offset:offset+16]) + ',')
print('};\nstatic const uint8_t* const lock_frames[] = {lock_data};')
print('const Icon I_Lockscreen = {.width=128,.height=64,.frame_count=1,.frame_rate=0,.frames=lock_frames};')
