"""Validate the exact staged snapshot before publishing. No network access."""
import subprocess
import zipfile
from pathlib import Path, PurePosixPath

root = Path(__file__).resolve().parents[1]
raw = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root)
files = [name.decode('utf-8') for name in raw.split(b'\0') if name]
blocked_roots = {'web-flasher', 'sdcard', 'SD card files', 'managed_components', '.diagnostics'}
blocked_names = {'local.properties', '.env', 'nvs.bin', 'flash_dump.bin', 'sdkconfig.old'}
bad = []
total = 0
for name in files:
    parts = PurePosixPath(name).parts
    file = root / name
    if not file.exists():
        bad.append(f'Missing staged file: {name}')
        continue
    size = file.stat().st_size
    total += size
    if parts[0] in blocked_roots or parts[0].startswith('build') and len(parts) > 1:
        bad.append(f'Non-source directory: {name}')
    if '.gradle' in parts or '.idea' in parts or file.name in blocked_names:
        bad.append(f'Local/private file: {name}')
    if file.suffix.lower() in {'.jks', '.keystore', '.p12', '.pfx'}:
        bad.append(f'Private key container: {name}')
    if size >= 100 * 1024 * 1024:
        bad.append(f'Exceeds GitHub file limit: {name}')
assert 'sdcard.zip' in files, 'Keep sdcard.zip in the publication'
with zipfile.ZipFile(root / 'sdcard.zip') as archive:
    assert archive.testzip() is None, 'Corrupt SD archive'
    for member in archive.infolist():
        path = PurePosixPath(member.filename.replace('\\', '/'))
        assert not path.is_absolute() and '..' not in path.parts, f'Unsafe ZIP entry: {path}'
        assert path.name not in {'key.u2f', 'cert_key.u2f', 'cnt.u2f', 'nvs.bin'}, 'Device identity in SD ZIP'
assert not bad, '\n'.join(bad)
print(f'PASS: {len(files)} staged files, {total / 1024 / 1024:.1f} MiB; firmware-only tree, SD ZIP intact, no caches/local credentials/device key containers')
