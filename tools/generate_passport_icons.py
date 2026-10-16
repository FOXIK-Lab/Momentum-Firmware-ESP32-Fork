"""Emit the original Momentum Passport PNGs as lossless Furi icon data."""
from pathlib import Path
from PIL import Image

source = Path(__file__).resolve().parents[2] / 'Momentum-Firmware/assets/icons/Passport'
lines = [
    '/* Generated from Momentum-Firmware/assets/icons/Passport (GPL-3.0).',
    ' * Pixel data is copied unchanged; regenerate with tools/generate_passport_icons.py. */',
    '#include "assets_icons.h"',
    '#include <gui/icon_i.h>',
    '',
]
for name in ['passport_128x64', 'passport_happy_46x49', 'passport_okay_46x49', 'passport_bad_46x49']:
    image = Image.open(source / (name + '.png')).convert('RGBA')
    width, height = image.size
    data = bytearray([0])  # Furi uncompressed bitmap marker.
    for y in range(height):
        for block in range((width + 7) // 8):
            value = 0
            for bit in range(8):
                x = block * 8 + bit
                if x < width:
                    r, g, b, alpha = image.getpixel((x, y))
                    if alpha and (r + g + b) < 384:
                        value |= 1 << bit
            data.append(value)
    lines.append(f'static const uint8_t {name}_data[] = {{')
    for offset in range(0, len(data), 16):
        lines.append('    ' + ', '.join(f'0x{b:02x}' for b in data[offset:offset+16]) + ',')
    lines.append('};')
    lines.append(f'static const uint8_t* const {name}_frames[] = {{{name}_data}};')
    lines.append(f'const Icon I_{name} = {{.width={width}, .height={height}, .frame_count=1, .frame_rate=0, .frames={name}_frames}};')
    lines.append('')
print('\n'.join(lines))
