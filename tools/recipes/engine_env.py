# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Environment merging for step execution.

This module is kept as a standalone module so the production step runner can
compose a child environment without importing the engine. Given a base
environment plus the `context` module's whole-variable overrides and path
prefix/suffix additions, it produces the environment a step's subprocess runs
with.
"""

from __future__ import annotations

from collections import defaultdict
from collections.abc import Mapping, Sequence
from itertools import chain


def merge_envs(
    original: Mapping[str, str],
    overrides: Mapping[str, str | None],
    prefixes: Mapping[str, Sequence[str]],
    suffixes: Mapping[str, Sequence[str]],
    pathsep: str,
) -> dict[str, str]:
    """Merge *overrides*/*prefixes*/*suffixes* onto the *original* environment.

    Entries in *overrides* overwrite the corresponding variable, while a value
    of `None` removes it. Values may contain `%(KEY)s`, substituted from
    *original* (so a variable like `PATH` can be amended rather than
    overwritten). *prefixes* and *suffixes* prepend / append their lists to the
    named variable, joined with *pathsep*. When a key has both an affix and an
    override, the override value is folded in as the last prefix component.
    """
    result = dict(original)
    if not any((prefixes, suffixes, overrides)):
        return result

    # Backing for `%(KEY)s` substitution: unknown keys resolve to '' rather than
    # raising, matching the upstream behaviour.
    subst: Mapping[str, str] = defaultdict(str, **dict(original))

    merged: set[str] = set()
    for key in set(suffixes).union(prefixes):
        pfxs = tuple(prefixes.get(key, ()))
        sfxs = tuple(suffixes.get(key, ()))
        if not (pfxs or sfxs):
            continue

        # If the same key is also overridden, fold it in here (and skip it in
        # the override pass below).
        merged.add(key)
        if key in overrides:
            val = overrides[key]
            if val is not None:
                val = str(val) % subst
        else:
            # Not overridden: append the original value iff it is non-empty.
            val = original.get(key, '')
        if val:
            pfxs += (val, )
        result[key] = pathsep.join(str(v) for v in chain(pfxs, sfxs))

    for key, val in overrides.items():
        if key in merged:
            continue
        if val is None:
            result.pop(key, None)
        else:
            result[key] = str(val) % subst

    return result
