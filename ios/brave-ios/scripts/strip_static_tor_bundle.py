#!/usr/bin/env python3
"""Remove Xcode's redundant static Tor bundle before iOS bundle validation."""

import argparse
import plistlib
import shutil
import subprocess
from pathlib import Path


def remove_static_tor_bundle(bundle):
    tor = bundle / "Frameworks" / "tor.framework"
    if not tor.exists():
        return []

    info = plistlib.loads((bundle / "Info.plist").read_bytes())
    executable = bundle / info["CFBundleExecutable"]
    loads = subprocess.check_output(
        ["xcrun", "otool", "-L", str(executable)], text=True
    )
    if "tor.framework/" in loads:
        raise RuntimeError("Refusing to remove a dynamically loaded Tor framework")

    # The pinned framework is static; Xcode copies its versioned bundle and
    # injects a stub anyway. Preserve unexpected resources by failing closed.
    removed = sorted(str(path.relative_to(tor)) for path in tor.rglob("*"))
    for path in tor.rglob("*"):
        if path.is_symlink() or not path.is_file():
            continue
        relative = path.relative_to(tor)
        allowed = (
            path.name in {"tor", "Info.plist"}
            or any(part in {"Headers", "PrivateHeaders", "Modules",
                            "_CodeSignature"} for part in relative.parts)
        )
        if not allowed:
            raise RuntimeError(f"Unexpected Tor resource: {relative}")
    shutil.rmtree(tor)
    return removed


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    args = parser.parse_args()
    removed = remove_static_tor_bundle(args.bundle)
    print(f"Removed {len(removed)} redundant static Tor entries from {args.bundle}")
