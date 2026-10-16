"""Build this port's SD archive from the official release and local FAPs.

Never reads a physical SD or NVS. Does not publish a GitHub release.
"""
import argparse
import hashlib
import io
import json
import struct
import tarfile
import time
import zipfile
from pathlib import Path, PurePosixPath

PORT = Path(__file__).resolve().parents[1]
ROOT = PORT.parent
REMOVED = {'bt_trigger', 'pc_monitor', 'air_arkanoid', 'air_labyrinth'}

def safe_name(name):
    name = name.replace('\\', '/')
    if name == '/':
        return ''
    parts = PurePosixPath(name)
    if parts.is_absolute() or '..' in parts.parts or ':' in name:
        raise ValueError(f'Unsafe archive path: {name!r}')
    return str(parts).rstrip('/')

def excluded(name):
    path = PurePosixPath(name)
    return (path.name in {'.DS_Store', '.fap_icon_cache', 'files.txt', 'Manifest', 'version.txt'}
            or '__MACOSX' in path.parts
            or (path.suffix in {'.fap', '.fal'} and path.stem in REMOVED)
            or (len(path.parts) > 1 and path.parts[0] == 'apps_assets' and path.parts[1] in REMOVED)
            or path.parts[0] == 'u2f')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', type=Path, default=ROOT / '.diagnostics/sdcard-upstream-v2.0.0.zip')
    parser.add_argument('--output', type=Path, default=PORT / 'sdcard.zip')
    args = parser.parse_args()
    files = {}
    with zipfile.ZipFile(args.base) as archive:
        for item in archive.infolist():
            name = safe_name(item.filename)
            if name and not item.is_dir() and not excluded(name):
                files[name] = archive.read(item)
    # Local builds take precedence, also when upstream placed an app in Misc.
    overlays = {}
    for folder in ('sdcard', 'SD card files'):
        base = PORT / folder
        for source in sorted(base.rglob('*')):
            if source.is_file():
                name = safe_name(source.relative_to(base).as_posix())
                if not excluded(name):
                    overlays[name] = source.read_bytes()
    replacement_ids = {PurePosixPath(n).stem for n in overlays if n.endswith('.fap')}
    files = {n: data for n, data in files.items()
             if not (n.endswith('.fap') and PurePosixPath(n).stem in replacement_ids)}
    files.update(overlays)
    asset_sources = []
    catalogue = json.loads((ROOT / '.diagnostics/asset-packs-directory.json').read_text())['packs']
    for archive_name in ('swirl', 'psyduck', 'betterpet'):
        archive_path = ROOT / '.diagnostics' / (archive_name + '.tar.gz')
        raw = archive_path.read_bytes()
        matching = next((p for p in catalogue if any(
            f.get('sha256') == hashlib.sha256(raw).hexdigest() for f in p.get('files', []))), None)
        if not matching:
            raise ValueError(f'Asset pack hash not found in official catalogue: {archive_name}')
        with tarfile.open(fileobj=io.BytesIO(raw), mode='r:gz') as archive:
            for member in archive:
                name = safe_name(member.name)
                if name and member.isfile():
                    files['asset_packs/' + name] = archive.extractfile(member).read()
                elif member.issym() or member.islnk():
                    raise ValueError('Asset pack contains links')
        asset_sources.append({'name': matching['name'], 'author': matching['author'],
                              'source_url': matching['source_url'], 'sha256': hashlib.sha256(raw).hexdigest()})
    files['u2f/README.txt'] = (
        'U2F is built into this T-Embed Momentum firmware.\n'
        'Open U2F from the main menu and accept its warnings.\n'
        'The device generates and stores its own identity in NVS, not on SD.\n'
        'No shared certificate keys, device keys or counters are supplied here.\n'
        'Keep another login method; this experimental device can fail.\n'
    ).encode()
    files['asset_packs/SOURCES.json'] = json.dumps(asset_sources, indent=2).encode()
    files['version.txt'] = b'2.0.0-momentum-tembed\n'
    faps = []
    omitted = []
    for name, data in sorted(list(files.items())):
        if name.endswith(('.fap', '.fal')):
            if data[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', data, 18)[0] != 94:
                # Upstream ships some legacy ARM NFC plugins. They cannot run
                # on ESP32; retain the port's built-in protocol support instead.
                if name.endswith('.fal'):
                    omitted.append({'file': name, 'reason': 'Legacy non-Xtensa plugin'})
                    del files[name]
                    continue
                raise ValueError(f'Not an ESP32-S3 Xtensa app: {name}')
            faps.append(name)
    files['sdcard-package.json'] = json.dumps({
        'base_release': 'Sor3nt/Flipper-Zero-ESP32-Port v2.0.0',
        'firmware_sha256': hashlib.sha256((PORT / 'build_t_embed/furi_esp32.bin').read_bytes()).hexdigest(),
        'removed_apps': sorted(REMOVED), 'omitted_legacy_files': omitted,
        'fap_count': len(faps), 'physical_tests_of_all_apps': False,
    }, indent=2).encode()
    assert not any(PurePosixPath(n).stem in REMOVED for n in faps)
    assert not any(PurePosixPath(n).name in {'cert_key.u2f', 'key.u2f', 'cnt.u2f'} for n in files)
    assert 'apps_data/doom/doom1.wad' in files, 'Preserve existing Doom resources'
    directories = set()
    for name in files:
        directories.update(str(p) for p in PurePosixPath(name).parents if str(p) != '.')
    manifest = ['V:0', f'T:{int(time.time())}']
    manifest += ['D:' + directory for directory in sorted(directories)]
    manifest += [f'F:{hashlib.md5(data).hexdigest()}:{len(data)}:{name}' for name, data in sorted(files.items())]
    files['Manifest'] = ('\n'.join(manifest) + '\n').encode()
    files['files.txt'] = ''.join(f'{hashlib.sha256(data).hexdigest()} {len(data)} {name}\n'
                                 for name, data in sorted(files.items())).encode()
    # Build the ZIP first, so a failed validation cannot replace the previous ZIP.
    temporary = args.output.with_suffix('.zip.new')
    with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(temporary) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == set(files)
        for name, data in files.items():
            assert archive.read(name) == data, name
    if args.output.exists():
        backup = ROOT / '.diagnostics' / (args.output.name + '.before-' + str(time.time_ns()))
        args.output.replace(backup)
    temporary.replace(args.output)
    print(f'Created {args.output}: {len(files)} files, {len(faps)} FAP/FAL files, 3 asset packs')

if __name__ == '__main__':
    main()
