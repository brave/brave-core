#!/usr/bin/env python
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Checks whether a pull request body preserves the pull request template.

Python port of the `pull-request` mode of Homebrew's check_template.rb
(Homebrew/brew#22186). A pull request is considered complete when at least
75% of the template's checkbox/heading items appear verbatim in the body
(checkboxes count whether ticked or not) and the body discloses AI/LLM use.

Usage: check_pr_template.py pull-request BODY TEMPLATE [TITLE REPOSITORY]
Prints "true" or "false".
"""

import re
import sys

CHECKBOX_MARKER = re.compile(r'^- \[[ xX]\] ')
NORMALISED_CHECKBOX = '- [ ] '
HTML_COMMENT_LINE = re.compile(r'<!--.*-->')
MARKDOWN_HEADING = re.compile(r'^#+ ')
MARKDOWN_HORIZONTAL_LINE = re.compile(r'^-+$')
AI_MENTION = re.compile(r'\b(?:AI|LLM)\b', re.IGNORECASE)
REVERT_TITLE = re.compile(r'Revert ".+"')
REQUIRED_TEMPLATE_PERCENTAGE = 75
PERCENTAGE_SCALE = 100


def normalised_lines(text):
    lines = []
    seen = set()
    for line in text.splitlines():
        line = CHECKBOX_MARKER.sub(NORMALISED_CHECKBOX, line.strip(), count=1)
        if not line:
            continue
        if MARKDOWN_HORIZONTAL_LINE.fullmatch(line):
            continue
        if HTML_COMMENT_LINE.fullmatch(line):
            continue
        if line in seen:
            continue
        seen.add(line)
        lines.append(line)
    return lines


def template_items(template):
    return [
        line for line in normalised_lines(template)
        if line.startswith(NORMALISED_CHECKBOX) or MARKDOWN_HEADING.match(line)
    ]


def is_complete(body, template, title='', repository=''):
    if repository and title:
        if REVERT_TITLE.fullmatch(title):
            if re.match(r'\s*Reverts ' + re.escape(repository) + r'#\d+\r?$',
                        body):
                return True

    normalised_body = normalised_lines(body)
    items = template_items(template)
    present = sum(1 for item in items if item in normalised_body)
    preserves_template = present * PERCENTAGE_SCALE >= len(
        items) * REQUIRED_TEMPLATE_PERCENTAGE
    discloses_ai = any(AI_MENTION.search(line) for line in normalised_body)
    return preserves_template and discloses_ai


def read_text(path):
    with open(path, 'rb') as f:
        return f.read().decode('utf-8', errors='replace')


def main(argv):
    if len(argv) < 4 or argv[1] != 'pull-request':
        print(
            'Usage: check_pr_template.py pull-request BODY TEMPLATE [TITLE REPOSITORY]',
            file=sys.stderr)
        return 1
    body = read_text(argv[2])
    template = read_text(argv[3])
    title = argv[4] if len(argv) > 4 else ''
    repository = argv[5] if len(argv) > 5 else ''
    print(
        'true' if is_complete(body, template, title, repository) else 'false')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
