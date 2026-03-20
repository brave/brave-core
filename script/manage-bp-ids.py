#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Manage stable IDs for best practice rules.

Assigns, validates, and checks unique IDs on ## and ### headings in
docs/best-practices/*.md so review-bot links always resolve.

Usage:
  manage-bp-ids.py --assign     Add IDs to headings that lack them
  manage-bp-ids.py --validate   Check all headings have unique IDs (read-only)
  manage-bp-ids.py --check-link <id> [--doc <doc.md>]
                                Check if an ID exists
                                (exit 0 = found, 1 = not found)
"""

import argparse
import re
import sys
from pathlib import Path

DOCS_DIR = Path(__file__).resolve().parent.parent / "docs" / "best-practices"

# Document filename stem → ID prefix
DOC_PREFIXES = {
    "coding-standards": "CS",
    "coding-standards-memory": "CSM",
    "coding-standards-apis": "CSA",
    "architecture": "ARCH",
    "build-system": "BS",
    "chromium-src-overrides": "CSRC",
    "documentation": "DOC",
    "frontend": "FE",
    "localization": "L10N",
    "testing-async": "TA",
    "testing-isolation": "TI",
    "testing-upstream-failures": "TUF",
    "testing-javascript": "TJ",
    "testing-navigation": "TN",
    "android": "AND",
    "ios": "IOS",
    "nala": "NA",
    "patches": "PATCH",
    "plaster": "PLSTR",
    "ui-views": "UV",
}

ANCHOR_RE = re.compile(r'^<a\s+id="([^"]+)"\s*></a>\s*$')
HEADING_RE = re.compile(r"^(#{2,3})\s+(.+)$")


def parse_doc(path: Path):
    """Yield (line_index, heading_text, existing_id_or_None)."""
    lines = path.read_text().splitlines()
    for i, line in enumerate(lines):
        m = HEADING_RE.match(line)
        if not m:
            continue
        heading_text = m.group(2)
        existing_id = None
        # Check if the preceding non-blank line is an anchor tag
        j = i - 1
        while j >= 0 and lines[j].strip() == "":
            j -= 1
        if j >= 0:
            am = ANCHOR_RE.match(lines[j])
            if am:
                existing_id = am.group(1)
        yield i, heading_text, existing_id


def collect_all_ids():
    """Return {id: (file, heading_text)} for every ID across all docs."""
    all_ids = {}
    for md in sorted(DOCS_DIR.glob("*.md")):
        for _, heading, eid in parse_doc(md):
            if eid:
                all_ids[eid] = (md.name, heading)
    return all_ids


def cmd_validate():
    errors = []
    all_ids = {}
    for md in sorted(DOCS_DIR.glob("*.md")):
        for line_idx, heading, eid in parse_doc(md):
            if not eid:
                errors.append(
                    f"  MISSING ID: {md.name}:{line_idx + 1}  {heading}")
            else:
                if eid in all_ids:
                    prev_file, prev_heading = all_ids[eid]
                    errors.append(
                        f"  DUPLICATE ID '{eid}': {md.name} ({heading}) "
                        f"vs {prev_file} ({prev_heading})")
                all_ids[eid] = (md.name, heading)

    if errors:
        print("Validation FAILED:")
        for e in errors:
            print(e)
        return 1
    print(f"OK: {len(all_ids)} headings, all have unique IDs.")
    return 0


def cmd_assign():
    existing_ids = collect_all_ids()
    total_added = 0

    for md in sorted(DOCS_DIR.glob("*.md")):
        stem = md.stem
        prefix = DOC_PREFIXES.get(stem)
        if prefix is None:
            print(f"WARNING: no prefix mapping for {md.name}, skipping")
            continue

        # Find the highest existing sequence number for this prefix
        max_seq = 0
        for eid in existing_ids:
            if eid.startswith(prefix + "-"):
                try:
                    seq = int(eid[len(prefix) + 1:])
                    max_seq = max(max_seq, seq)
                except ValueError:
                    pass

        lines = md.read_text().splitlines()
        new_lines = []
        i = 0
        added = 0
        while i < len(lines):
            m = HEADING_RE.match(lines[i])
            if m:
                # Check if preceding line is already an anchor
                has_anchor = False
                j = len(new_lines) - 1
                while j >= 0 and new_lines[j].strip() == "":
                    j -= 1
                if j >= 0:
                    am = ANCHOR_RE.match(new_lines[j])
                    if am:
                        has_anchor = True

                if not has_anchor:
                    max_seq += 1
                    new_id = f"{prefix}-{max_seq:03d}"
                    # Ensure blank line before anchor for readability
                    if new_lines and new_lines[-1].strip() != "":
                        new_lines.append("")
                    new_lines.append(f'<a id="{new_id}"></a>')
                    new_lines.append("")
                    existing_ids[new_id] = (md.name, m.group(2))
                    added += 1

            new_lines.append(lines[i])
            i += 1

        if added > 0:
            md.write_text("\n".join(new_lines) + "\n")
            print(f"  {md.name}: added {added} IDs"
                  f" ({prefix}-001..{prefix}-{max_seq:03d})")
            total_added += added

    print(f"\nTotal: {total_added} IDs added.")
    return 0


def cmd_check_link(fragment, doc=None):
    """Check if a fragment ID exists. Returns 0 if found, 1 if not."""
    if doc:
        target = DOCS_DIR / doc
        if not target.exists():
            print(f"NOT FOUND: doc '{doc}' does not exist")
            return 1
        for _, _, eid in parse_doc(target):
            if eid == fragment:
                print(f"OK: {fragment} found in {doc}")
                return 0
        print(f"NOT FOUND: {fragment} not in {doc}")
        return 1

    # Search all docs
    all_ids = collect_all_ids()
    if fragment in all_ids:
        fname, heading = all_ids[fragment]
        print(f"OK: {fragment} found in {fname} ({heading})")
        return 0
    print(f"NOT FOUND: {fragment}")
    return 1


def main():
    parser = argparse.ArgumentParser(description="Manage best-practice IDs")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--assign",
                       action="store_true",
                       help="Add IDs to headings missing them")
    group.add_argument("--validate",
                       action="store_true",
                       help="Check all IDs are unique")
    group.add_argument("--check-link",
                       metavar="ID",
                       help="Check if an ID exists")
    parser.add_argument("--doc",
                        help="Limit --check-link to a specific doc file")
    args = parser.parse_args()

    if args.assign:
        sys.exit(cmd_assign())
    elif args.validate:
        sys.exit(cmd_validate())
    elif args.check_link:
        sys.exit(cmd_check_link(args.check_link, args.doc))


if __name__ == "__main__":
    main()
