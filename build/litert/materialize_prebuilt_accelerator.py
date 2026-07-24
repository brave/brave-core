#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Materializes a prebuilt LiteRT accelerator from its Git LFS pointer.

The chromium.googlesource mirror of LiteRT strips LFS content, leaving the
prebuilt accelerators as `<name>.lfs` pointer files. This fetches the real
binary from the upstream GitHub LFS store, keyed by the oid pinned in the
pointer, so the accelerator we bundle stays in lockstep with the checked-out
LiteRT source. Run as a gclient hook. Idempotent: skips when the binary is
already present and matches the pinned hash.
"""

import argparse
import hashlib
import json
import os
import sys
import urllib.request

# LFS store of the upstream repo that //third_party/litert mirrors.
_LFS_BATCH_URL = (
    "https://github.com/google-ai-edge/LiteRT.git/info/lfs/objects/batch")
_CHUNK = 1 << 20


def _parse_pointer(path):
    oid = None
    size = None
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("oid sha256:"):
                oid = line[len("oid sha256:"):].strip()
            elif line.startswith("size "):
                size = int(line[len("size "):].strip())
    if not oid or size is None:
        sys.exit(f"Not a valid LFS pointer: {path}")
    return oid, size


def _sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(_CHUNK), b""):
            h.update(chunk)
    return h.hexdigest()


def _matches(path, oid, size):
    return (os.path.exists(path) and os.path.getsize(path) == size
            and _sha256(path) == oid)


def _download_href(oid, size):
    body = json.dumps({
        "operation": "download",
        "transfers": ["basic"],
        "objects": [{
            "oid": oid,
            "size": size
        }],
    }).encode("utf-8")
    request = urllib.request.Request(
        _LFS_BATCH_URL,
        data=body,
        headers={
            "Accept": "application/vnd.git-lfs+json",
            "Content-Type": "application/vnd.git-lfs+json",
        })
    with urllib.request.urlopen(request) as response:
        obj = json.load(response)["objects"][0]
    if "actions" not in obj:
        sys.exit(f"LFS server declined oid {oid}: {obj.get('error')}")
    return obj["actions"]["download"]["href"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--pointer",
        required=True,
        help="Path to the <name>.lfs pointer; the binary is written alongside "
        "it without the .lfs suffix.")
    args = parser.parse_args()

    pointer = args.pointer
    if not pointer.endswith(".lfs"):
        sys.exit("--pointer must end with .lfs")
    # A checkout that still carries real LFS content has no pointer file;
    # nothing to do in that case.
    if not os.path.exists(pointer):
        return
    out = pointer[:-len(".lfs")]
    oid, size = _parse_pointer(pointer)
    if _matches(out, oid, size):
        return

    tmp = out + ".tmp"
    urllib.request.urlretrieve(_download_href(oid, size), tmp)
    if not _matches(tmp, oid, size):
        os.remove(tmp)
        sys.exit(f"Downloaded blob for {out} failed its integrity check")
    os.replace(tmp, out)
    print(f"Materialized {out} ({size} bytes)")


if __name__ == "__main__":
    main()
