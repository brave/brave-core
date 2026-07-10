#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Chunk best-practice documents into groups of ~N rules for parallel review.

Each best-practice document contains rules marked with <a id="XX-NNN"></a>
anchors followed by ## headings. This script splits documents at ## heading
boundaries and groups them into chunks of approximately CHUNK_SIZE rules.

Rules with ### sub-headings are kept together with their parent ## section.

Usage:
    python3 chunk-best-practices.py <doc_path> [--chunk-size N]

Output (JSON):
    [
      {
        "doc": "coding-standards.md",
        "chunk_index": 0,
        "total_chunks": 3,
        "rule_count": 5,
        "headings": ["Always Include What You Use (IWYU)", ...],
        "content": "# C++ Coding Standards\\n\\n<a id=\\"CS-001\\">..."
      },
      ...
    ]
"""

import argparse
import json
import os
import re
import sys

DEFAULT_CHUNK_SIZE = 3


def split_into_rules(text):
    """Split a document into its header and individual ## rule sections.

    Returns (header_text, list_of_rule_dicts) where each rule dict has:
      - text: the full text of the rule section
      - heading: the ## heading text (stripped of # and emoji)
    """
    lines = text.split("\n")

    # Find positions of top-level ## headings (not ### or deeper).
    # Each rule boundary starts at the <a id> tag preceding the ## heading.
    rule_starts = []
    for i, line in enumerate(lines):
        if not re.match(r"^## [^#]", line):
            continue

        # Walk backwards to find the start of this rule's block:
        # the <a id> tag, and optionally a --- separator before it.
        start = i
        for j in range(max(0, i - 4), i):
            if re.match(r'^<a id="', lines[j]):
                start = j
                break

        # Include a preceding --- separator and blank lines if present.
        while start > 0 and lines[start - 1].strip() in ("---", ""):
            start -= 1

        rule_starts.append(start)

    if not rule_starts:
        return text, []

    header = "\n".join(lines[:rule_starts[0]])
    rules = []
    for i, start in enumerate(rule_starts):
        end = rule_starts[i + 1] if i + 1 < len(rule_starts) else len(lines)
        rule_text = "\n".join(lines[start:end])

        # Extract the heading text for metadata.
        heading = ""
        for line in lines[start:end]:
            if re.match(r"^## [^#]", line):
                heading = re.sub(r"^#+\s*", "", line)
                # Strip common emoji prefixes.
                heading = re.sub(r"^[✅❌🔧]\s*", "", heading).strip()
                break

        rules.append({"text": rule_text, "heading": heading})

    return header, rules


def chunk_rules(rules, chunk_size=DEFAULT_CHUNK_SIZE):
    """Group rules into evenly-sized chunks of approximately chunk_size.

    Uses round() to decide the number of chunks, then distributes rules
    as evenly as possible. This avoids creating oversized or undersized
    chunks at the boundary.
    """
    n = len(rules)
    if n <= chunk_size:
        return [rules]

    num_chunks = max(1, round(n / chunk_size))

    # Distribute evenly: some chunks get one extra rule.
    base_size = n // num_chunks
    extra = n % num_chunks

    chunks = []
    start = 0
    for i in range(num_chunks):
        size = base_size + (1 if i < extra else 0)
        chunks.append(rules[start:start + size])
        start += size

    return chunks


def process_doc(doc_path, chunk_size=DEFAULT_CHUNK_SIZE):
    """Process a best-practice document and return chunks as dicts."""
    with open(doc_path) as f:
        text = f.read()

    header, rules = split_into_rules(text)

    if not rules:
        return [{
            "doc": os.path.basename(doc_path),
            "chunk_index": 0,
            "total_chunks": 1,
            "rule_count": 0,
            "headings": [],
            "content": text,
        }]

    chunks = chunk_rules(rules, chunk_size)

    result = []
    for i, chunk in enumerate(chunks):
        chunk_content = (header.rstrip("\n") + "\n\n" +
                         "\n".join(r["text"] for r in chunk))
        headings = [r["heading"] for r in chunk]
        result.append({
            "doc": os.path.basename(doc_path),
            "chunk_index": i,
            "total_chunks": len(chunks),
            "rule_count": len(chunk),
            "headings": headings,
            "content": chunk_content,
        })

    return result


def main():
    parser = argparse.ArgumentParser(
        description="Chunk best-practice documents for parallel review")
    parser.add_argument("doc_path", help="Path to the best-practice document")
    parser.add_argument(
        "--chunk-size",
        type=int,
        default=DEFAULT_CHUNK_SIZE,
        help=f"Max rules per chunk (default: {DEFAULT_CHUNK_SIZE})",
    )
    args = parser.parse_args()

    chunks = process_doc(args.doc_path, args.chunk_size)
    json.dump(chunks, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
