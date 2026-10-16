# Security

This is experimental development firmware, not a hardened security appliance.
Only use its radio, USB and network features on systems you own or have permission
to test. Repository privacy does not make firmware safe.

Before any public release, review [SECURITY-REVIEW.md](SECURITY-REVIEW.md), update
the ESP-IDF/dependency security assessment, and physically test affected features.
Do not post passwords, account keys, NVS/flash dumps or device captures in issues.
Report a suspected vulnerability privately to the maintainer before disclosure.

## Important boundaries

- U2F identities are generated per device and stored in software NVS, without a
  secure element. Flash encryption and secure boot are not enabled. A desktop PIN
  does not protect against physical flash extraction. Keep another login method.
- Web-Filesystem is manually started and uses a new random HTTP Basic password
  for each server session. User: `admin`; password: shown on the device. Locked
  devices refuse requests. HTTP itself is not encrypted. Use only trusted local
  networks; do not expose the service to the Internet or forward its port.
- Pairing/authenticated BLE is a trust boundary. When locked RPC BLE is allowed,
  a trusted paired phone may unlock the device, including PIN locks, as configured
  by the owner. Do not trust an unknown phone or enable this unnecessarily.
- Network updates are disabled while this fork has no release feed. Local file
  OTA validates image compatibility, not an author's signature. Install only
  trusted binaries and applications. Hashes alone are not authentication.
- FAP applications are not isolated untrusted code. Running a third-party FAP
  grants substantial access to the device and SD.
- No task here enables eFuses, secure boot, flash encryption, or formats a real SD.

## Local regression checks

```sh
python tools/check_security_wiring.py
cc -std=c11 -Wall -Wextra -Werror tests/webfs_security_test.c -o webfs_security_test
./webfs_security_test
```

These checks are not a full penetration test, fuzzing campaign, RF evaluation,
cryptographic audit, or certification.
