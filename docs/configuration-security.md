# Configuration hardening (work in progress)

Based on upstream `folloup-waveshare` at
`f0b56e969ba3cf7a9d1bed0a77472337abd07ee4`.

## Setup

1. Run `python3 scripts/provision_setup.py` locally before configuring ESP-IDF.
   This generates independent random 32-character passwords for the WPA2 setup
   network and the HTTP panel. No shared/default password is provided.
2. Keep `sdkconfig` and `setup-credentials.local.txt` private. Both are ignored
   by Git. Back up the credentials separately; firmware binaries contain them.
3. Build only when explicitly requested. Join the device's Followup Wi-Fi using
   the generated network password. Open `http://192.168.4.1/` in a full browser.
   Sign in as `admin` with the separate panel password. A captive mini-browser
   might not support HTTP Basic authentication reliably.
4. Turn off AP setup mode using the device after saving settings. Never forward
   its HTTP port on your router.

The build intentionally fails without the required password lengths. Existing
local sdkconfig settings take precedence over defaults; the provisioning script
edits that ignored local file and refuses to overwrite existing setup secrets.

## Implemented protections

- WPA2-PSK setup network, at most one associated client.
- Separate HTTP Basic password on every registered Wi-Fi, Gemini and timezone
  route, including the frontend assets.
- Accepted HTTP connections must target the AP interface's local IP; station
  interface connections are rejected even when AP and station run together.
- Exact Host allowlist, Origin validation, cross-site fetch rejection and
  framing prevention reduce browser-based cross-site and DNS-rebinding attacks.
- Secrets/settings responses must not be cached. AP and panel passwords are not
  exposed by the status API or application logs.

HTTP Basic is not TLS. Protection on the setup radio relies on WPA2 and strong,
private credentials. Anyone who knows the AP password is outside that protection
boundary and could attack/intercept HTTP; the separate panel password is an
additional access check, not encryption. This is not end-to-end HTTPS.

## Validation and remaining work

Host policy tests cover accepted/rejected hosts, origins and credentials.
Provisioning tests use temporary directories and verify overwrite refusal and
file permissions. Firmware has NOT been compiled or tested on a board. Required
hardware tests before use:

- Check Wi-Fi password rejection and authenticated panel access in a full browser.
- Check every API rejects missing/wrong panel credentials.
- While AP+station are active, verify the panel is unreachable at the station IP.
- Verify cross-site requests are denied; verify Wi-Fi and Gemini setup succeeds.
- Switch AP off, reboot, exercise sleep/wake and confirm setup still works.

Not implemented: HTTPS, physical-presence setup expiry, brute-force throttling,
credential rotation UI, flash/NVS encryption, secure boot, SD encryption, or
dependency/binary supply-chain verification. Do not describe this as a complete
security audit or production-ready secure firmware.

The `portal_security` component owns request guards. Service route registration
must use its wrapper; do not register new sensitive routes directly with HTTPD.
The guard assumes the project's default AP address 192.168.4.1. If that address
changes, update the Host/Origin policy together.
