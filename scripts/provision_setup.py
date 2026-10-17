#!/usr/bin/env python3
"""Generate local setup secrets without printing them or adding them to Git."""
from pathlib import Path
import re
import os
import secrets

ROOT = Path(__file__).resolve().parents[1]


def main():
    config = ROOT / "sdkconfig"
    text = config.read_text() if config.exists() else ""
    keys = ("FOLLOWUP_WIFI_AP_PASSWORD", "FOLLOWUP_PORTAL_PASSWORD")
    if any(re.search(rf'^CONFIG_{key}=".+"$', text, re.M) for key in keys):
        raise SystemExit("Setup secrets already exist; refusing to rotate or overwrite them.")
    generated = {key: secrets.token_hex(16) for key in keys}
    for key, value in generated.items():
        text = re.sub(rf"^(?:CONFIG_{key}=.*|# CONFIG_{key} is not set)\n?", "", text, flags=re.M)
        text += f'\nCONFIG_{key}="{value}"\n'
    # Exclusive creation avoids accidentally overwriting a saved credentials file.
    credentials = ROOT / "setup-credentials.local.txt"
    with open(credentials, "x", opener=lambda path, flags: os.open(path, flags, 0o600)) as f:
        f.write(f"Wi-Fi password: {generated[keys[0]]}\nPanel user: admin\n"
                f"Panel password: {generated[keys[1]]}\nURL: http://192.168.4.1/\n")
    fd = os.open(config, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(text)
    os.chmod(config, 0o600)
    print("Local setup secrets saved. Read setup-credentials.local.txt privately; do not share it.")


if __name__ == "__main__":
    main()
