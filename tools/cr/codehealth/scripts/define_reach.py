#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists `chromium_src` `#define` tokens whose reach changed between two tags.

A `chromium_src` override's `#define Foo BraveFoo` rewrites every `Foo` in the
upstream file it wraps. The count of `Foo` in the pristine upstream file (and,
for a header, its `.cc`) at each tag stands for that reach: a different count
means the define rewrites different code, and 0 at the new tag means it now
matches nothing. Tokens that reach nothing at both tags are listed too, unless
they are Brave's helpers (used by the override or a Brave file it includes, or
token-pasted) or a Brave patch adds them upstream.

    vpython3 tools/cr/codehealth/scripts/define_reach.py OLD..NEW
"""

from __future__ import annotations

import argparse
from collections.abc import Sequence
from dataclasses import dataclass
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot

_SUFFIXES = ('.h', '.cc', '.mm')


@dataclass(frozen=True)
class Reach:
    """How often a define's token occurs upstream at each tag."""

    override: str
    token: str
    old: int
    new: int

    def __str__(self) -> str:
        return f'{self.old:>3} -> {self.new:<3} {self.token}  {self.override}'


class DefineReach:
    """Counts each override's defined tokens in pristine upstream files."""

    # A define with a replacement: `#define Foo Bar`, `#define Foo(x) ...`.
    # Flag-like defines and include guards have none.
    _DEFINE = re.compile(
        r'^\s*#\s*define\s+([A-Za-z_]\w*)(?:\(|[ \t]+\S)', re.M
    )
    _OWN = re.compile(r'^(?:BRAVE_|brave_|BUILDFLAG_INTERNAL_)|_H_$|_BRAVE\b')

    def __init__(self, old: str, new: str) -> None:
        self._old, self._new = old, new

    _BRAVE_INCLUDE = re.compile(r'^\s*#\s*include\s+"brave/([^"]+)"', re.M)
    _WORD = re.compile(r'\b[A-Za-z_]\w*\b')

    def defines(
        self, text: str, included: Sequence[str] = ()
    ) -> list[tuple[str, bool]]:
        """The tokens `text` defines, its own `BRAVE_` macros left out, and
        whether each is a helper of Brave's: used beyond its definition in
        `text` or in the `included` Brave files, or pasted onto a prefix
        (`CAT(IMPL_, x)` reaching `IMPL_0`)."""
        tokens = []
        for token in self._DEFINE.findall(text):
            if self._OWN.search(token) or token in (t for t, _ in tokens):
                continue
            uses = len(re.findall(rf'\b{re.escape(token)}\b', text))
            defines = len(
                re.findall(
                    rf'#\s*(?:define|undef)\s+{re.escape(token)}\b', text
                )
            )
            helper = (
                uses > defines
                or any(self.count(token, [t]) for t in included)
                or self._pasted(token, text)
            )
            tokens.append((token, helper))
        return tokens

    @staticmethod
    def _pasted(token: str, text: str) -> bool:
        """Whether `text` names a `_`-ended prefix of `token` on its own."""
        return any(
            re.search(rf'\b{re.escape(token[: i + 1])}(?!\w)', text)
            for i, char in enumerate(token[:-1])
            if char == '_' and i > 0
        )

    @classmethod
    def patched_words(cls, patches: Sequence[str]) -> set[str]:
        """The words on lines Brave's patches add to upstream files."""
        words = set()
        for patch in patches:
            for line in patch.splitlines():
                if line.startswith('+') and not line.startswith('+++'):
                    words.update(cls._WORD.findall(line))
        return words

    @staticmethod
    def count(token: str, texts: list[str | None]) -> int:
        """How often `token` occurs as a word in `texts`, missing ones left
        out."""
        word = re.compile(rf'\b{re.escape(token)}\b')
        return sum(len(word.findall(t)) for t in texts if t)

    @staticmethod
    def upstream_files(override: str) -> list[str]:
        """The upstream files whose code an override's defines reach."""
        files = [override]
        stem, suffix = override.rsplit('.', 1)
        if suffix == 'h':
            files += [f'{stem}.cc', f'{stem}.mm']
        return files

    def run(self) -> list[Reach]:
        """The defines whose reach changed, or that reach nothing; dead ones
        first."""
        brave = repository.brave.root
        chromium_src = brave / 'chromium_src'
        overrides = {}
        for path in sorted(chromium_src.rglob('*')):
            if path.suffix in _SUFFIXES and path.is_file():
                text = path.read_bytes().decode('utf-8', 'replace')
                included = [
                    (brave / name).read_bytes().decode('utf-8', 'replace')
                    for name in self._BRAVE_INCLUDE.findall(text)
                    if (brave / name).is_file()
                ]
                tokens = self.defines(text, included)
                if tokens:
                    # Git wants `/` in `tag:path`, also on Windows.
                    overrides[path.relative_to(chromium_src).as_posix()] = (
                        tokens
                    )
        wanted = [
            (tag, f)
            for o in overrides
            for f in self.upstream_files(o)
            for tag in (self._old, self._new)
        ]
        texts = self._read(wanted)
        # A token no pristine file names can still be reached by a patch
        # adding it (`BASE_OVERRIDDEN_FEATURE` in each features `.cc`).
        patched = self.patched_words(
            [
                p.read_bytes().decode('utf-8', 'replace')
                for p in (brave / 'patches').rglob('*.patch')
            ]
        )
        changed = []
        for override, tokens in overrides.items():
            files = self.upstream_files(override)
            old = [texts[(self._old, f)] for f in files]
            new = [texts[(self._new, f)] for f in files]
            if not any(old) and not any(new):
                continue  # A Brave-only file.
            for token, helper in tokens:
                reach = Reach(
                    override,
                    token,
                    self.count(token, old),
                    self.count(token, new),
                )
                if reach.old != reach.new or (
                    reach.new == 0 and not helper and token not in patched
                ):
                    changed.append(reach)
        return sorted(changed, key=lambda r: (r.new != 0, r.override, r.token))

    @staticmethod
    def _read(
        wanted: list[tuple[str, str]],
    ) -> dict[tuple[str, str], str | None]:
        """`git cat-file --batch` for every `tag:path`, None when missing."""
        request = ''.join(f'{tag}:{path}\n' for tag, path in wanted).encode()
        output = subprocess.run(
            ['git', '-C', str(repository.chromium.root), 'cat-file', '--batch'],
            input=request,
            capture_output=True,
            check=True,
        ).stdout
        texts, pos = {}, 0
        for key in wanted:
            end = output.index(b'\n', pos)
            header = output[pos:end].split()
            pos = end + 1
            if header[-1] == b'missing':
                texts[key] = None
                continue
            size = int(header[2])
            texts[key] = output[pos : pos + size].decode('utf-8', 'replace')
            pos += size + 1
        return texts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    changed = DefineReach(old, new).run()
    print(f'old -> new token  override ({len(changed)} changed; 0 = dead)')
    for reach in changed:
        print(reach)
    return 0


if __name__ == '__main__':
    sys.exit(main())
