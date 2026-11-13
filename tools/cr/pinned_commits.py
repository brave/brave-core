# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The commit subjects `brockit rebase` groups or discards.

This module has no dependencies outside the standard library, as the
`commit-msg` hook imports it too.

* PINNED -- commits that should be unique in the history of the branch.
  Multiple occurrences of the same pinned change are auto-squashed by
  `brockit.py rebase --squash-minor-bumps`, and the resulting squash is
  moved to the top of the branch history. Priority order is the entry
  order in `PINNED_GROUPS`. Subjects that fit this category:

    - Update from Chromium * to Chromium *.
    - Apply-fixed 🩹 patches from Chromium * to Chromium *.
    - Conflict-resolved patches from Chromium * to Chromium *.
    - [cr*] `gnrt` run for Chromium *.
    - [cr*] IWYU fixes.
    - [cr*] Bump resource_ids.
    - [cr*] Update Lit mangler snapshot.
    - [cr*] Version CodeHealth tracker.
    - [cr*] Version lift updates.
    - [cr*] Disable failing upstream tests.
    - [cr*] Update lists for upstream flake tests.

* RECYCLABLE -- commits we can always drop and regenerate with tooling.
  They're discarded by `brockit.py rebase --discard-regen-changes`.
  Subjects that fit this category:

    - Update patches from Chromium * to Chromium *.
    - Updated strings for Chromium *.
"""

from __future__ import annotations

import re

# Strips one or more leading bracketed tags whose first token starts with
# `cr` followed by digits -- e.g. `[cr149] `, `[cr149][ios] `. Leaves
# unrelated bracketed tags alone. This means branch tags are irrelevant
# when determining the commit type.
_CR_TAG_RE = re.compile(r'^(?:\[cr\d+\](?:\[[^\]]+\])*\s+)?')

# Strips repeated leading autosquash prefixes like `fixup! `, `amend! `,
# `squash! ` -- these stack when commits fix up an already-fixup commit
# (e.g. `fixup! fixup! fixup! Conflict-resolved patches …`). Pre-stripping
# them lets the pinned/recyclable regexes match the underlying subject
# regardless of how many autosquash levels deep the user has gone.
_AUTOSQUASH_PREFIX_RE = re.compile(r'^(?:(?:fixup|amend|squash)!\s+)+')

# Chromium version shape: exactly four dot-separated numeric segments
# (e.g. 149.0.7827.5).
_CR_VERSION = r'\d+\.\d+\.\d+\.\d+'


def strip_cr_tag(subject: str) -> str:
    """Returns `subject` with any leading `[crNNN]` (or `[crNNN][...]…`)
    tag and the trailing whitespace removed. A subject that doesn't
    start with such a tag is returned unchanged."""
    return _CR_TAG_RE.sub('', subject, count=1)


def strip_autosquash_prefixes(subject: str) -> str:
    """Returns `subject` with any leading stack of `fixup! ` / `amend! `
    / `squash! ` prefixes removed. Subjects without those prefixes are
    returned unchanged."""
    return _AUTOSQUASH_PREFIX_RE.sub('', subject)


def _match(subject: str, pattern: re.Pattern) -> bool:
    """True if `pattern` fully matches `subject` after stripping any
    leading `fixup!` / `amend!` / `squash!` autosquash prefixes and any
    `[crNNN]` tag. The stripped subject is also right-trimmed so a
    trailing space in the original line doesn't break the fullmatch."""
    cleaned = strip_cr_tag(strip_autosquash_prefixes(subject)).rstrip()
    return pattern.fullmatch(cleaned) is not None


# Ordered list of `(pinned-group-id, regex)` entries. The order *is* the
# squash priority order: when multiple pinned groups are present in the
# same rebase plan, they're emitted top-to-bottom in this order
# (version → plaster_reruns → conflict → gnrt → IWYU →
# resource_ids → lit_mangler → codehealth_tracker → lift_updates →
# disable_tests → dead_upstream_tests → flake_tests). The regexes are
# anchored fullmatches applied to the `[crNNN]`-stripped subject, so each
# pattern matches both the tagged and the untagged form of the same commit.
PINNED_GROUPS = [
    (
        'version',
        re.compile(
            rf'Update from Chromium {_CR_VERSION} to '
            rf'Chromium {_CR_VERSION}\.?'
        ),
    ),
    (
        'plaster_reruns',
        re.compile(
            rf'Apply-fixed 🩹 patches from Chromium {_CR_VERSION} to '
            rf'Chromium {_CR_VERSION}\.?'
        ),
    ),
    (
        'conflict',
        re.compile(
            rf'Conflict-resolved patches from Chromium {_CR_VERSION} to '
            rf'Chromium {_CR_VERSION}\.?'
        ),
    ),
    ('gnrt', re.compile(rf'`gnrt` run for Chromium {_CR_VERSION}\.?')),
    ('iwyu', re.compile(r'IWYU fixes\.?')),
    ('resource_ids', re.compile(r'Bump resource_ids\.?')),
    ('lit_mangler', re.compile(r'Update Lit mangler snapshot\.?')),
    ('codehealth_tracker', re.compile(r'Version CodeHealth tracker\.?')),
    ('lift_updates', re.compile(r'Version lift updates\.?')),
    (
        'disable_tests',
        re.compile(
            r'Disable failing upstream tests\.?|'
            r'Filter upstream tests as needed\.?'
        ),
    ),
    (
        'dead_upstream_tests',
        re.compile(r'Remove dead upstream tests from filters\.?'),
    ),
    ('flake_tests', re.compile(r'Update lists for upstream flake tests\.?')),
]

# Subjects produced by `pnpm run update_patches` and string generation --
# evicted when `discard_recyclable=True`.
RECYCLABLE_PATTERNS = [
    re.compile(
        rf'Update patches from Chromium {_CR_VERSION} to '
        rf'Chromium {_CR_VERSION}\.?'
    ),
    re.compile(rf'Updated strings for Chromium {_CR_VERSION}\.?'),
]


def get_pinned_group_for_subject(subject: str) -> str | None:
    """Returns the `PINNED_GROUPS` group id that `subject` matches
    (e.g. `'version'`, `'iwyu'`), or `None` when no pattern matches.
    Matching strips a leading `[crNNN]`-style tag, so the tagged and
    untagged forms of the same subject return the same group id."""
    for pinned_group, pattern in PINNED_GROUPS:
        if _match(subject, pattern):
            return pinned_group
    return None


def is_recyclable(subject: str) -> bool:
    """Returns True when `subject` matches any `RECYCLABLE_PATTERNS`
    entry -- i.e. the commit is auto-generated and can be regenerated
    from the current source tree."""
    return any(_match(subject, p) for p in RECYCLABLE_PATTERNS)
