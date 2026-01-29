#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""
Validates that PWA manifest files (*.webmanifest) use brave:// scheme
instead of chrome:// scheme for user-facing URLs.

Extension manifests (manifest.json) are explicitly excluded since they
legitimately use chrome:// for API permissions.
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path


def find_webmanifest_files(root_dir):
    """Find all *.webmanifest files in the brave directory."""
    webmanifest_files = []
    for root, dirs, files in os.walk(root_dir):
        # Skip vendor/third_party directories that are explicitly vendored code
        # Check if vendor or third_party is in the path components
        path_parts = os.path.normpath(root).split(os.sep)
        if 'vendor' in path_parts and 'crates' in path_parts:
            # Skip vendored Rust crates
            continue
        if 'third_party' in path_parts or 'third-party' in path_parts:
            continue

        for file in files:
            if file.endswith('.webmanifest'):
                webmanifest_files.append(os.path.join(root, file))
    return webmanifest_files


def validate_manifest(manifest_path):
    """
    Check if a webmanifest file contains chrome:// URLs.
    Returns tuple of (is_valid, error_messages)
    """
    try:
        with open(manifest_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        # Look for chrome:// URLs (but not chrome-untrusted://)
        chrome_url_pattern = r'chrome://'
        problematic_lines = []

        for line_num, line in enumerate(lines, start=1):
            if re.search(chrome_url_pattern, line):
                problematic_lines.append((line_num, line.rstrip()))

        if problematic_lines:
            lines_detail = "\n".join([
                f"      Line {num}: {line}"
                for num, line in problematic_lines
            ])

            error_msg = (
                f"\n[FAIL] VALIDATION FAILED\n"
                f"\n"
                f"   File: {manifest_path}\n"
                f"\n"
                f"   Error: PWA manifests must use 'brave://' scheme, not 'chrome://' scheme.\n"
                f"\n"
                f"   Lines that need fixing:\n"
                f"{lines_detail}\n"
                f"\n"
                f"   WHY: PWA manifest URLs are user-facing and displayed in the browser.\n"
                f"        Internal routing will convert brave:// to chrome:// automatically.\n"
                f"\n"
                f"   FIX: Replace 'chrome://' with 'brave://' on the lines above.\n"
            )
            return False, error_msg

        return True, None

    except Exception as e:
        error_msg = f"\n[ERROR]\n\n   File: {manifest_path}\n   Error: {e}\n"
        return False, error_msg


def main():
    parser = argparse.ArgumentParser(
        description='Validate PWA manifests for correct URL schemes')
    parser.add_argument('--stamp', help='Stamp file to create on success')
    parser.add_argument('--list-files', action='store_true',
                        help='List all webmanifest files in GN format and exit')
    parser.add_argument('--root-dir', help='Root directory to search (for --list-files)')
    parser.add_argument('files', nargs='*', help='Specific files to validate')
    args = parser.parse_args()

    # Get the brave directory (script is in brave/build/commands/scripts/)
    script_dir = Path(__file__).parent
    brave_dir = args.root_dir if args.root_dir else script_dir.parent.parent.parent

    # If --list-files, just output the file list and exit
    if args.list_files:
        webmanifest_files = find_webmanifest_files(brave_dir)
        print('[')
        for f in webmanifest_files:
            rel_path = os.path.relpath(f, brave_dir)
            print(f'  "{rel_path}",')
        print(']')
        return 0

    # Use provided files or discover them
    if args.files:
        webmanifest_files = args.files
        print(f"Validating {len(webmanifest_files)} PWA manifest(s)")
    else:
        print(f"Validating PWA manifests in: {brave_dir}")
        webmanifest_files = find_webmanifest_files(brave_dir)

    if not webmanifest_files:
        print("[OK] No *.webmanifest files found.")
        if args.stamp:
            Path(args.stamp).touch()
        return 0

    print(f"Found {len(webmanifest_files)} webmanifest file(s)")

    all_valid = True
    errors = []

    for manifest_path in webmanifest_files:
        is_valid, error_msg = validate_manifest(manifest_path)
        if not is_valid:
            all_valid = False
            errors.append(error_msg)

    if all_valid:
        print("[OK] All PWA manifests are valid (no chrome:// URLs found)")
        if args.stamp:
            Path(args.stamp).touch()
        return 0
    else:
        print("\n" + "="*80)
        print("PWA MANIFEST VALIDATION FAILED")
        print("="*80)
        for error in errors:
            print(error)
        print("="*80 + "\n")
        return 1


if __name__ == '__main__':
    sys.exit(main())
