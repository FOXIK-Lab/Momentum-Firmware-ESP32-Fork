# Pre-publication security review — 2026-10-16

## Scope

Targeted source review of this fork's file-server, updater and trust boundaries;
local secret scanning of source and upstream history; archive/package checks;
publication-tree cleanup; regression checks and T-Embed compilation. No physical
device penetration testing or destructive tests were performed. This is **not**
a claim that every application, dependency or protocol is vulnerability-free.

## Findings addressed

| Finding | Change |
| --- | --- |
| WebFS had no HTTP authentication, including on existing Wi-Fi networks | Every route now requires `admin` plus a fresh random 80-bit session password displayed on T-Embed. Credentials are not persisted or logged. |
| Authenticated browser cross-origin writes | Origin is checked against the request host; cross-origin requests are rejected. Locked devices reject HTTP requests. Responses disable caching and framing. |
| Noncanonical root spellings could bypass the SD root-delete check | Shared path validation rejects empty/dot segments, redundant or trailing separators, backslashes and control characters. Delete and rename explicitly refuse root. |
| URL decoding silently truncated paths or accepted embedded NUL | Decoder fails on truncation, malformed escapes, NUL and control characters. Invalid list queries no longer silently become root queries. |
| Upload timeout loop could retry forever | At most two timeout retries, 180-second transfer deadline and 64 MB upload cap. Larger files should use USB storage. This is not protection against all authenticated resource exhaustion. |
| Wi-Fi firmware and SD updaters skipped certificate/hostname verification | Attached the ESP certificate bundle and enabled hostname checking; disabled global insecure TLS fallback. A valid device clock and trusted server are required. |
| Update feed pointed at upstream Sor3nt binaries, not this port | Dedicated port release policy; network updates are disabled until this fork has its own supported release process. No GitHub credentials are embedded. |
| Update object could be freed after a fixed cancellation wait while worker still used it | Atomic worker-running flag and a cooperative lifetime wait; task handle cleared before the final release. Slow network cancellation may wait for the network timeout rather than freeing live memory. |

## Remaining limitations / public release gates

- ESP-IDF remains **5.4.1**. This review did not migrate the SDK or backport every
  vendor advisory. Do not describe this build as fully patched. Review current
  advisories and migrate/test the SDK before a public release, including SoftAP
  DHCP code. See [Espressif security advisories](https://github.com/espressif/esp-idf/security/advisories)
  and [DHCP parser advisory GHSA-g764-gwc3-75m5](https://github.com/espressif/esp-idf/security/advisories/GHSA-g764-gwc3-75m5).
  The advisory describes affected newer 5.4 branch releases; absence of 5.4.1 in
  that listed version set is not evidence that this older SDK is safe.
- WebFS Basic authentication over HTTP is visible to a network eavesdropper.
  The AP still supports the existing default/configured Wi-Fi password and open
  AP behavior; use a strong Wi-Fi password and a trusted network. No TLS server
  or complete online rate limiting was added.
- Interrupted authenticated uploads can leave partial files. There is no
  transactional filesystem, file sandbox per application or FAP isolation.
- Software U2F/NVS, physical extraction, paired-phone unlock, unsigned local
  firmware/FAPs and lack of anti-rollback are described in SECURITY.md.
- No irreversible eFuse or device security settings were changed.
- Current source contains upstream cryptographic format markers/test vectors
  and a legacy U2F demo resource. They are not this device's account identity.
  The ESP32 U2F implementation generates and reads its own NVS identity instead.

## Secret scanning

Gitleaks **8.30.1** was downloaded from its official GitHub release and checked
against the release SHA-256 checksum. Redacted source/history reports are stored
outside this repository. The initial source scan flagged 34 findings: cache-key
names, protocol/crypto constants, test vectors, and PEM format marker strings.
They were reviewed as false positives, not silently suppressed globally. The
history scan also produced 34 findings in upstream library/example code.
No personal access token, account credential or device NVS identity was identified
in the current publication tree. This is not a guarantee that arbitrary compiled
third-party apps or every historical blob contains no secrets.

The staged-change scan flagged another 12 occurrences in newly added crypto
sources: published AES vectors and function declarations taking secret-key
parameters. These were reviewed as false positives. A separate publication
check found an Android companion signing keystore; the entire unrelated Android
companion project was moved to a recoverable local backup and is not published.
The SD ZIP was also checked for unsafe paths and device U2F identity filenames.

The private repository is prepared from a fresh snapshot of the current source,
not by uploading upstream history, local caches or diagnostic backups. Original
upstream history remains in the local checkout. Base provenance: Sor3nt upstream
commit `0e367fb`; source credits and license notices remain intact.

## Cleanup

- Web flasher moved to a separate sibling project, outside the firmware tree.
- Unpacked `sdcard` and `SD card files` removed from firmware; `sdcard.zip` kept.
- Build caches, IDE state, downloaded managed components, Android Gradle cache
  and local machine properties moved outside the publication tree.
- Local backups remain recoverable outside Git. No files on physical SD removed.
- GitHub Actions configuration is excluded from this publication because the
  publishing connection lacks workflow permission. A local backup is retained;
  manual firmware builds are unaffected.

## Verification

- Production C path/decoder tests cover SD-root aliases, traversal, control
  characters, encoded NUL, malformed escapes and truncation.
- Source wiring regression checks cover all seven HTTP routes, origin/lock
  checks, TLS configuration, update-feed policy and worker lifetime ordering.
- Clean T-Embed ESP32-S3 build passed after the final hardening changes.
  The generated app image is 3,652,080 bytes. Build artifacts are outside Git.
- Native C path/decoder tests, security wiring checks and final publication-tree
  checks passed. The staged snapshot contains 7,588 files (about 171.6 MiB).
- Browser/phone login and actual file transfers with the new session password
  still require a physical-device check after flashing. No hardware flash was
  requested or performed as part of repository preparation.
