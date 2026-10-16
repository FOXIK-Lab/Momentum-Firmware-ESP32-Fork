"""Read-only regression checks for the pre-publication security hardening.

Not a penetration test or proof that the full firmware is free of defects.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
web = (root / 'applications/main/wlan_app/wlan_webfs.c').read_text(encoding='utf-8')
for handler in ('root', 'list', 'download', 'upload', 'rename', 'delete', 'mkdir'):
    body = web.split(f'static esp_err_t handler_{handler}(httpd_req_t* req) {{', 1)[1]
    assert body.lstrip().startswith('if(!webfs_authorize(req)) return ESP_OK;'), handler
assert 'esp_fill_random(random, sizeof(random))' in web
assert 'WWW-Authenticate' in web and 'Origin' in web
assert 'FuriHalRtcFlagLock' in web
assert '++timeouts < 3' in web and 'furi_ms_to_ticks(180000)' in web
assert 'webfs_path_allowed(path, false)' in web
for filename in ('wlan_fw_update.c', 'wlan_sd_update.c'):
    source = (root / 'applications/main/wlan_app' / filename).read_text(encoding='utf-8')
    assert 'cfg->crt_bundle_attach = esp_crt_bundle_attach;' in source
    assert 'cfg->skip_cert_common_name_check = false;' in source
    assert 'sor3nt.github.io' not in source
    assert 'MOMENTUM_PORT_NETWORK_UPDATES_ENABLED' in source
    assert '_Atomic bool running;' in source
    assert 'while(u->running)' in source
    finish = re.search(r'static void \w+_finish\([^)]*\) \{(.*?)\n\}', source, re.S).group(1)
    assert finish.index('u->task = NULL;') < finish.index('u->running = false;')
assert '#define MOMENTUM_PORT_NETWORK_UPDATES_ENABLED 0' in (
    root / 'applications/main/wlan_app/port_update_policy.h').read_text()
rpc = (root / 'components/rpc/rpc.c').read_text()
assert 'rpc_owner_allowed' in rpc
unlock = (root / 'applications/services/rpc/rpc_desktop.c').read_text()
assert 'ble_serial_is_authenticated' in unlock
print('PASS: WebFS auth/lock/origin/path/timeout guards, TLS verification, private OTA policy, worker lifetime, BLE unlock authentication')
