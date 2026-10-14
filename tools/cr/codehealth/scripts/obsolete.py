#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Finds Brave code that a range of upstream commits made obsolete.

These are the mechanical checks of code health sweep g:

    cited-bugs       Brave-cited crbugs and CLs that the range's commits name
                     in `Bug:`, `Fixed:` or `Reviewed-on:`, reverts included
    patch-adoptions  lines Brave's patches add that upstream added too
    test-filters     filter entries the range renamed, re-enabled, orphaned
                     or copied into a new suite, and filtered fixtures a new
                     suite derives from
    removed-names    names, files, constants and features the range removed
                     that Brave still uses or overrides
    lift-overrides   the overrides the lift's commits added, and the patches
                     whose added lines they changed, that the range touches

Each prints one finding per line, for `sweep.py`'s `tool:` field. Findings that
predate the range are counted; `--debt` lists them, for a fresh tracker.
`--brave REF` reads Brave at a ref instead of the working tree: at
`origin/master`, the checks show what the lift had to fix.

    vpython3 tools/cr/codehealth/scripts/obsolete.py CHECK OLD..NEW
        [--brave REF] [--debt]
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from collections.abc import Iterable, Iterator
import contextlib
from dataclasses import dataclass, field
import fnmatch
import functools
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot

# Brave paths whose words aren't Brave using upstream names: vendored code,
# and patches and plasters, whose context the rebase regenerates.
_NOT_BRAVE_USES = (
    ':!third_party',
    ':!vendor',
    ':!**/node_modules/**',
    ':!patches',
    ':!rewrite',
    ':!tools/cr',
    ':!tools/crates',
)


@dataclass(frozen=True)
class Git:
    """A repository, read at `ref`, or at its working tree when it's None."""

    root: Path
    ref: str | None = None

    def run(self, *args: str, stdin: str | None = None) -> str:
        """Runs git, and returns its output.

        Returns:
            The output, which is empty when git exits with 1, as a grep
            without a match does.

        Raises:
            subprocess.CalledProcessError: git failed otherwise.
        """
        result = subprocess.run(
            ['git', '-C', str(self.root), *args],
            input=stdin,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace',
            check=False,
        )
        if result.returncode not in (0, 1):
            raise subprocess.CalledProcessError(
                result.returncode, args, result.stdout, result.stderr
            )
        return result.stdout

    def grep(self, *args: str, paths: tuple[str, ...] = ()) -> str:
        """`git grep`'s output, at the ref or in the working tree."""
        refs = (self.ref,) if self.ref else ()
        return self.run('grep', '-I', *args, *refs, '--', *paths)

    def grep_lines(
        self, *args: str, paths: tuple[str, ...] = ()
    ) -> Iterator[tuple[str, str, str]]:
        """`(path, line number, text)` per line of a `grep -n`."""
        for line in self.grep('-n', *args, paths=paths).splitlines():
            if self.ref:
                line = line[len(self.ref) + 1 :]
            path, lineno, text = (line.split(':', 2) + ['', ''])[:3]
            yield path, lineno, text

    def files(self, *paths: str) -> list[str]:
        """The tracked files under `paths`."""
        if self.ref:
            out = self.run(
                'ls-tree', '-r', '--name-only', self.ref, '--', *paths
            )
        else:
            out = self.run('ls-files', '--', *paths)
        return out.splitlines()

    def show(self, path: str, ref: str | None = None) -> str | None:
        """A file's text at `ref` (by default the Git's own), or None."""
        ref = ref or self.ref
        if ref is None:
            try:
                return (
                    (self.root / path).read_bytes().decode('utf-8', 'replace')
                )
            except OSError:
                return None
        result = subprocess.run(
            ['git', '-C', str(self.root), 'show', f'{ref}:{path}'],
            capture_output=True,
            check=False,
        )
        if result.returncode:
            return None
        return result.stdout.decode('utf-8', 'replace')


@dataclass
class RangeCommit:
    """A commit of the upstream range, with the lines of its body."""

    sha: str
    subject: str
    body: list[str] = field(default_factory=list)


@dataclass(frozen=True)
class Range:
    """The upstream range, and Brave read at a ref or its working tree."""

    old: str
    new: str
    brave_ref: str | None = None
    # Whether to list findings that predate the range, not only count them.
    debt: bool = False

    @functools.cached_property
    def chromium(self) -> Git:
        return Git(repository.chromium.root)

    @functools.cached_property
    def brave(self) -> Git:
        return Git(repository.brave.root, self.brave_ref)

    def commits(self) -> list[RangeCommit]:
        """The range's commits, newest first."""
        commits = []
        out = self.chromium.run(
            'log', '--format=@@@ %H %s%n%B', f'{self.old}..{self.new}'
        )
        for line in out.splitlines():
            if line.startswith('@@@ '):
                sha, _, subject = line[4:].partition(' ')
                commits.append(RangeCommit(sha, subject))
            elif commits:
                commits[-1].body.append(line)
        return commits


def _debt_lines(range_: Range, debt: list[str], what: str) -> list[str]:
    """The count of the findings that predate the range, then the findings
    with `--debt`."""
    lines = [f'{len(debt)} {what}']
    if range_.debt:
        lines += debt
    elif debt:
        lines.append('    (list them with --debt)')
    return lines


class CitedBugs:
    """Brave's crbug and CL citations, matched against the range's bodies."""

    _BUG = re.compile(
        r'(?:crbug\.com/(?:chromium/)?|crbug/|issues\.chromium\.org/'
        r'(?:issues/)?|bugs\.chromium\.org/p/chromium/issues/detail\?id=)'
        r'(\d{5,})'
    )
    _CL = re.compile(
        r'(?:chromium-review\.googlesource\.com/c/chromium/src/\+/|'
        r'crrev\.com/c/)(\d{5,})'
    )
    _TRAILER = re.compile(r'^\s*(>?)\s*(Bug|Fixed)\s*[:=]\s*(.*)', re.I)
    _TOKEN_SEPARATOR = re.compile(r'[,\s]+')
    # `b:` ids are Google's tracker, not Chromium's.
    _OTHER_TRACKER = re.compile(r'^(?!chromium:)[a-z]+:\d')
    _TOKEN_ID = re.compile(r'(\d{6,})')
    # Brave paths whose citations count, and those of the commits whose
    # bodies do: the ones changing Brave's overrides.
    _OVERRIDES = ('chromium_src', 'patches', 'rewrite', 'test/filters')
    # A bug that more range commits name is an umbrella (Bedrock,
    # exit-time destructors), not a fix.
    UMBRELLA = 5
    _WHERE = 3
    _QUOTED = '    '

    def __init__(self, range_: Range):
        self._range = range_

    def citations(self) -> dict[tuple[str, str], list[str]]:
        """`{(kind, id): [where Brave cites it]}`, kind `crbug` or `CL`."""
        cited: dict[tuple[str, str], list[str]] = defaultdict(list)
        brave = self._range.brave
        for path, lineno, text in brave.grep_lines(
            '-P',
            f'{self._BUG.pattern}|{self._CL.pattern}',
            paths=(
                '.',
                ':!third_party',
                ':!vendor',
                ':!**/node_modules/**',
                ':!tools/cr',
            ),
        ):
            # A patch's context and removed lines are upstream's citations.
            if path.endswith('.patch') and not text.startswith('+'):
                continue
            for kind, ident in self._ids(text):
                cited[(kind, ident)].append(f'{path}:{lineno}')
        log = brave.run(
            'log',
            '--format=@@@ %h %s%n%b',
            brave.ref or 'HEAD',
            '--',
            *self._OVERRIDES,
        )
        subject = ''
        for line in log.splitlines():
            if line.startswith('@@@ '):
                subject = line[4:]
                continue
            # The indented block is the quoted culprit's own message, whose
            # bugs would match the culprit itself.
            if line.startswith(self._QUOTED):
                continue
            for kind, ident in self._ids(line):
                # A lift's commits cite the upstream CL they follow, which
                # the range has by definition; their bugs are what counts.
                if kind == 'CL':
                    continue
                where = f'commit {subject}'
                if where not in cited[(kind, ident)]:
                    cited[(kind, ident)].append(where)
        return cited

    def _ids(self, text: str) -> Iterator[tuple[str, str]]:
        for match in self._BUG.finditer(text):
            yield 'crbug', match.group(1)
        for match in self._CL.finditer(text):
            yield 'CL', match.group(1)

    def named(self, commit: RangeCommit) -> Iterator[tuple[str, str, str]]:
        """`(kind, id, how)` for each bug and CL the commit's body names."""
        for line in commit.body:
            trailer = self._TRAILER.match(line)
            if trailer:
                quoted, how, rest = trailer.groups()
                for token in self._TOKEN_SEPARATOR.split(rest):
                    if self._OTHER_TRACKER.match(token):
                        continue
                    ident = self._TOKEN_ID.search(token)
                    if ident:
                        yield (
                            'crbug',
                            ident.group(1),
                            (f'quoted {how}' if quoted else how),
                        )
            # The commit's own CL, or the one a revert or reland quotes; other
            # mentions are context.
            if line.startswith('Reviewed-on:'):
                how = 'Reviewed-on'
            elif line.lstrip().startswith('>') and 'Reviewed-on:' in line:
                how = 'quoted CL'
            else:
                continue
            for match in self._CL.finditer(line):
                yield 'CL', match.group(1), how

    def run(self) -> list[str]:
        """The cited ids the range names, with the commits naming them."""
        cited = self.citations()
        commits = self._range.commits()
        subjects = {c.subject for c in commits}
        hits: dict[tuple[str, str], list[str]] = defaultdict(list)
        naming: Counter[tuple[str, str]] = Counter()
        for commit in commits:
            seen = set()
            for kind, ident, how in self.named(commit):
                key = (kind, ident)
                if key not in cited or key in seen:
                    continue
                seen.add(key)
                if not how.startswith('quoted'):
                    naming[key] += 1
                note = ''
                if f'Revert "{commit.subject}"' in subjects:
                    note = ' (reverted in range)'
                hits[key].append(
                    f'    {how:<12} {commit.sha[:12]} {commit.subject}{note}'
                )
        lines, umbrellas = [], []
        for key in sorted(hits):
            kind, ident = key
            if kind == 'crbug' and naming[key] > self.UMBRELLA:
                umbrellas.append(f'{ident} ({naming[key]} commits)')
                continue
            wheres = cited[key]
            more = (
                f' [+{len(wheres) - self._WHERE} more]'
                if len(wheres) > self._WHERE
                else ''
            )
            brave = ', '.join(wheres[: self._WHERE])
            lines.append(f'{kind} {ident}  brave: {brave}{more}')
            lines += hits[key]
        if umbrellas:
            lines.append(f'Umbrella bugs, skipped: {", ".join(umbrellas)}')
        lines.append(
            f'{len(hits) - len(umbrellas)} cited ids named in the range'
        )
        return lines


class PatchAdoptions:
    """Lines a Brave patch adds that the pristine file now has itself."""

    _DIFF = re.compile(r'^diff --git a/(\S+) b/(\S+)')
    # Lines too common to say anything: braces, `#endif`, `return true;`.
    _GENERIC = re.compile(
        r'^(?:[\W_]*|#\s*(?:endif|else).*|return\b.{0,12}|break;|default:|'
        r'//.*|/\*.*|\*.*|(?:public|private|protected):|.{0,11})$'
    )

    _BOILERPLATE = 3

    def __init__(self, range_: Range):
        self._range = range_

    @staticmethod
    def upstream_path(patch: str, path: str) -> str:
        """The upstream file a patch's diff of `path` applies to: patches in
        `patches/v8/` apply to `v8/`."""
        return (Path(patch).parent.relative_to('patches') / path).as_posix()

    def patches(self) -> Iterator[tuple[str, str, Counter[str], Counter[str]]]:
        """`(patch, upstream path, + lines, - lines)` per patched file."""
        brave = self._range.brave
        for patch in brave.files('patches'):
            if not patch.endswith('.patch'):
                continue
            text = brave.show(patch) or ''
            path, plus, minus = None, Counter(), Counter()
            for line in text.splitlines():
                diff = self._DIFF.match(line)
                if diff:
                    if path:
                        yield (
                            patch,
                            self.upstream_path(patch, path),
                            plus,
                            minus,
                        )
                    path, plus, minus = diff.group(2), Counter(), Counter()
                elif line.startswith(('+++', '---')):
                    continue
                elif line.startswith('+'):
                    plus[line[1:].strip()] += 1
                elif line.startswith('-'):
                    minus[line[1:].strip()] += 1
            if path:
                yield patch, self.upstream_path(patch, path), plus, minus

    def run(self) -> list[str]:
        """The added lines upstream has too, per patched file."""
        chromium = self._range.chromium
        lines, debt, outside = [], [], 0
        for patch, path, plus, minus in self.patches():
            net = [text for text, count in plus.items() if count > minus[text]]
            added = [
                text
                for text in net
                if not self._GENERIC.match(text) and 'brave' not in text.lower()
            ]
            if not added:
                continue
            # A line naming Brave is what the patch is for, which upstream
            # can't have: the patch still does something.
            brave_specific = any('brave' in text.lower() for text in net)
            old = chromium.show(path, self._range.old)
            new = chromium.show(path, self._range.new)
            if new is None:
                # A sub-repository (V8, DevTools), or deleted upstream.
                outside += old is None
                continue
            old_lines = Counter(
                line.strip() for line in (old or '').splitlines()
            )
            new_lines = Counter(line.strip() for line in new.splitlines())
            for text in added:
                before, after = old_lines[text], new_lines[text]
                # A line the file repeats is boilerplate, like a feature's
                # `base::FEATURE_DISABLED_BY_DEFAULT);`.
                if before < after < self._BOILERPLATE:
                    lines.append(
                        f'adopted ({before} -> {after})\t{patch}\t{path}\t'
                        f'+{text[:120]}'
                    )
            # A patch whose lines upstream all has does nothing; one line
            # among many is mostly the patch repeating a line on purpose. It
            # had them before the range too.
            if not brave_specific and all(
                old_lines[text] and new_lines[text] for text in added
            ):
                debt.append(
                    f'already upstream\t{patch}\t{path}\t'
                    f'all {len(added)} added lines, e.g. +{added[0][:100]}'
                )
        lines.sort()
        lines.append(f'{outside} patched files outside chromium/src skipped')
        return lines + _debt_lines(
            self._range, sorted(debt), 'patched files already upstream'
        )


@dataclass(frozen=True)
class FilterEntry:
    """A test filter line, as `Suite.Test` to look up in a `TestIndex`."""

    where: str
    text: str
    suite: str
    test: str
    prefixed: bool


class TestIndex:
    """The `TEST*` and `INSTANTIATE_*` macros of the tree at a tag."""

    _START = re.compile(r'^\s*([A-Z_]*TEST[A-Z_0-9]*|INSTANTIATE_[A-Z_]*)\(')
    _ARGS = re.compile(r'\(\s*(\w*)\s*,\s*(\w+)')
    _PREFIXES = ('MAYBE_', 'DISABLED_', 'PRE_')

    def __init__(self, lines: Iterable[str] = ()):
        # suite -> test name -> {(kind, disabled)}; kind F, P or T(yped).
        self.suites: dict[str, dict[str, set[tuple[str, bool]]]] = defaultdict(
            lambda: defaultdict(set)
        )
        # suite -> the prefixes its `INSTANTIATE_*` macros give it.
        self.prefixes: dict[str, set[str]] = defaultdict(set)
        self.add(list(lines))

    @classmethod
    def at(cls, git: Git, tag: str) -> TestIndex:
        """The index of the `.cc` and `.mm` files at `tag`."""
        out = git.run(
            'grep',
            '-h',
            '-A2',
            '-E',
            cls._START.pattern.replace('\\s', '[[:space:]]'),
            tag,
            '--',
            '*.cc',
            '*.mm',
        )
        return cls(out.splitlines())

    def add(self, lines: list[str]) -> None:
        """Indexes the macros in `lines`, which can span three lines."""
        for i, line in enumerate(lines):
            start = self._START.match(line)
            if not start:
                continue
            macro = start.group(1)
            if macro.startswith('FRIEND'):
                continue
            args = self._ARGS.search(
                ' '.join(lines[i : i + 3]), start.end() - 1
            )
            if not args:
                continue
            first, second = args.groups()
            if macro.startswith('INSTANTIATE_'):
                self.prefixes[self._strip(second)[0]].add(first)
                continue
            if not first:
                continue
            suite, suite_disabled = self._strip(first)
            name, disabled = self._strip(second)
            if 'TYPED' in macro:
                kind = 'T'
            elif macro.endswith('_P'):
                kind = 'P'
            else:
                kind = 'F'
            self.suites[suite][name].add((kind, disabled or suite_disabled))

    @classmethod
    def _strip(cls, name: str) -> tuple[str, bool]:
        """The name without `MAYBE_`, `DISABLED_` and `PRE_`, and whether
        it was disabled."""
        disabled = False
        while name.startswith(cls._PREFIXES):
            prefix = next(p for p in cls._PREFIXES if name.startswith(p))
            disabled |= prefix == 'DISABLED_'
            name = name[len(prefix) :]
        return name, disabled

    def matches(self, entry: FilterEntry) -> dict[str, set[tuple[str, bool]]]:
        """The test names the entry matches, with their kinds."""
        names = self.suites.get(entry.suite)
        if not names:
            return {}
        return {
            name: kinds
            for name, kinds in names.items()
            if fnmatch.fnmatchcase(name, entry.test)
        }


class TestFilters:
    """Brave's test filter entries against the test macros at both tags."""

    # Test names shorter than this are too common to follow into new suites.
    _COPIED_MIN = 12
    # `class Derived [final] : public [ns::]Base`, which can wrap before `:`.
    _SUBCLASS = re.compile(
        r'\bclass\s+(\w+)(?:\s+final)?\s*:\s*public\s+(?:\w+::)*(\w+)'
    )

    def __init__(
        self,
        range_: Range,
        old: TestIndex | None = None,
        new: TestIndex | None = None,
    ):
        self._range = range_
        self._old = old or TestIndex.at(range_.chromium, range_.old)
        self._new = new or TestIndex.at(range_.chromium, range_.new)

    @staticmethod
    def parse(where: str, text: str) -> FilterEntry | None:
        """`[-][Prefix/]Suite[/N].Test[/N]`; None for what can't be checked."""
        pattern = text.strip().lstrip('-')
        if not pattern or pattern.startswith('#') or '.' not in pattern:
            return None
        suite_part, _, test_part = pattern.partition('.')
        parts = suite_part.split('/')
        prefixed = (len(parts) > 1 and not parts[-1].isdigit()) or len(
            parts
        ) > 2
        suite = parts[1] if prefixed else parts[0]
        if '*' in suite or not suite:
            return None
        return FilterEntry(
            where, text.strip(), suite, test_part.split('/')[0], prefixed
        )

    def entries(self) -> list[FilterEntry]:
        """Brave's filter entries that name a suite."""
        brave = self._range.brave
        entries = []
        for path in brave.files('test/filters'):
            if not path.endswith('.filter'):
                continue
            for number, line in enumerate(
                (brave.show(path) or '').splitlines(), 1
            ):
                entry = self.parse(f'{path}:{number}', line)
                if entry:
                    entries.append(entry)
        return entries

    def check(self, entry: FilterEntry) -> list[str]:
        """What the range did to the tests the entry matches."""
        old, new = self._old.matches(entry), self._new.matches(entry)
        if old and not new:
            moved = sorted(
                suite
                for suite, names in self._new.suites.items()
                if entry.test in names and suite != entry.suite
            )
            hint = f'; now in {", ".join(moved[:3])}' if moved else ''
            return [f'gone in range{hint}']
        if not old and not new:
            return ['matches nothing at either tag']
        found = []
        old_kinds = {k for kinds in old.values() for k, _ in kinds}
        new_kinds = {k for kinds in new.values() for k, _ in kinds}
        if entry.prefixed and 'P' in old_kinds and 'P' not in new_kinds:
            found.append('became TEST_F: the prefixed entry matches nothing')
        if not entry.prefixed and 'F' in old_kinds and new_kinds == {'P'}:
            found.append('became TEST_P: the filter no longer applies')
        if '*' not in entry.test and entry.test in new:
            was = {d for _, d in old.get(entry.test, ())}
            now = {d for _, d in new[entry.test]}
            if was == {True} and False in now:
                found.append('DISABLED_ dropped upstream')
        if '*' not in entry.test and len(entry.test) >= self._COPIED_MIN:
            copies = sorted(
                suite
                for suite, names in self._new.suites.items()
                if entry.test in names
                and suite != entry.suite
                and suite not in self._old.suites
            )
            if copies:
                found.append(f'copied into new {", ".join(copies[:3])}')
        return found

    def new_subclasses(self) -> dict[str, list[tuple[str, str]]]:
        """Base class -> `(subclass, commit)` for each class the range's
        commits declare."""
        out = self._range.chromium.run(
            'log',
            '-p',
            '-U0',
            '--no-color',
            '--format=@@@ %h',
            '-G',
            ': *public',
            f'{self._range.old}..{self._range.new}',
            '--',
            '*.cc',
            '*.h',
            '*.mm',
        )
        found: dict[str, list[tuple[str, str]]] = defaultdict(list)
        sha, added = '', []

        def flush() -> None:
            for match in self._SUBCLASS.finditer(' '.join(added)):
                found[match.group(2)].append((match.group(1), sha))
            added.clear()

        for line in out.splitlines():
            if line.startswith('+') and not line.startswith('+++'):
                added.append(line[1:])
                continue
            flush()
            if line.startswith('@@@ '):
                sha = line[4:]
        flush()
        return found

    def run(self) -> list[str]:
        """The findings, and the entries stale before the range, counted."""
        lines, debt = [], []
        entries = self.entries()
        for entry in entries:
            for finding in self.check(entry):
                line = f'{entry.where}\t{entry.text}\t{finding}'
                if finding.startswith('matches nothing'):
                    debt.append(line)
                else:
                    lines.append(line)
        # A new suite deriving from a filtered fixture usually fails for the
        # same reason (cr159: a visual classification suite of a filtered
        # client-side detection fixture).
        first_entry = {}
        for entry in entries:
            first_entry.setdefault(entry.suite, entry)
        for base, subclasses in sorted(self.new_subclasses().items()):
            entry = first_entry.get(base)
            if entry is None:
                continue
            for derived, sha in subclasses:
                if (
                    derived in self._new.suites
                    and derived not in self._old.suites
                ):
                    lines.append(
                        f'{entry.where}\t{entry.text}\tnew subclass suite '
                        f'{derived} ({sha})'
                    )
        lines.append(f'{len(debt)} entries match nothing at either tag')
        if self._range.debt:
            lines += debt
        else:
            per_file = Counter(line.split(':', 1)[0] for line in debt)
            lines += [
                f'    {n} in {path}' for path, n in per_file.most_common()
            ]
            lines.append('    (list them with --debt)')
        return lines


class RemovedNames:
    """What the range removed that Brave still uses or overrides."""

    _IDENT = '[A-Za-z_][A-Za-z0-9_]{7,}'
    _WORD = re.compile(rf'\b{_IDENT}\b')
    # API names: types, functions and `k` constants. Locals and fields that
    # Brave's code happens to share are noise.
    _API = re.compile(r'k?[A-Z][a-z0-9]+(?:[A-Z][a-z0-9]*)+')
    _CODE = (
        '*.cc',
        '*.h',
        '*.mm',
        '*.java',
        '*.ts',
        '*.js',
        '*.gn',
        '*.gni',
        '*.mojom',
        '*.swift',
        '*.py',
        '*.xml',
        '*.grd',
        '*.grdp',
        '*.idl',
    )
    _FEATURE = re.compile(r'^X\("(\w+)"\)', re.M)
    _FEATURES = 'base/compile_overridden_features.inc'
    # The names a replaced header declares: constants and functions.
    _NAME = re.compile(r'\b(k[A-Z]\w+|[A-Z]\w+(?=\s*\())')
    _COMMENT = re.compile(r'//[^\n]*|/\*.*?\*/', re.S)
    # A line that is only a comment: a mention there isn't a use.
    _COMMENT_LINE = re.compile(r'^\s*(?://|/?\*|#|<!--)')
    _WHERE = 2
    # The statuses of rows that predate the range.
    _PREDATES = (' not upstream: ', '\tmissing at both')

    def __init__(self, range_: Range):
        self._range = range_

    @staticmethod
    @contextlib.contextmanager
    def _pattern_file(needles: set[str]) -> Iterator[str]:
        """A temporary `grep -f` file of the needles, one per line."""
        with tempfile.NamedTemporaryFile(
            'w', encoding='utf-8', newline='', suffix='.txt'
        ) as patterns:
            patterns.write('\n'.join(sorted(needles)) + '\n')
            patterns.flush()
            yield patterns.name

    def _brave_locations(
        self,
        needles: set[str],
        words: bool = True,
        exclude: tuple[str, ...] = (),
    ) -> dict[str, list[str]]:
        """Where Brave spells each needle, outside vendored code."""
        found: dict[str, list[str]] = defaultdict(list)
        if not needles:
            return found
        with self._pattern_file(needles) as patterns:
            for path, lineno, needle in self._range.brave.grep_lines(
                '-o',
                *(['-w'] if words else []),
                '-F',
                '-f',
                patterns,
                paths=(*self._CODE, *_NOT_BRAVE_USES, *exclude),
            ):
                if len(found[needle]) < self._WHERE:
                    found[needle].append(f'{path}:{lineno}')
        return found

    def _code_locations(
        self, needles: set[str]
    ) -> tuple[dict[str, list[str]], dict[str, list[str]]]:
        """Where Brave uses each needle, and where it only names it in a
        comment line."""
        found: dict[str, list[str]] = defaultdict(list)
        comments: dict[str, list[str]] = defaultdict(list)
        if not needles:
            return found, comments
        word = re.compile(
            r'\b(' + '|'.join(map(re.escape, sorted(needles))) + r')\b'
        )
        with self._pattern_file(needles) as patterns:
            for path, lineno, text in self._range.brave.grep_lines(
                '-w',
                '-F',
                '-f',
                patterns,
                paths=(*self._CODE, *_NOT_BRAVE_USES),
            ):
                bucket = comments if self._COMMENT_LINE.match(text) else found
                for needle in set(word.findall(text)):
                    if len(bucket[needle]) < self._WHERE:
                        bucket[needle].append(f'{path}:{lineno}')
        for needle in found:
            comments.pop(needle, None)
        return found, comments

    def _present(self, tag: str, needles: set[str]) -> set[str]:
        """The needles the tree spells as identifiers at `tag`. Lists every
        identifier and keeps the needles with GNU grep: `git grep -w -F -f`
        with thousands of needles takes hours."""
        if not needles:
            return set()
        with self._pattern_file(needles) as patterns:
            with subprocess.Popen(
                [
                    'git',
                    '-C',
                    str(self._range.chromium.root),
                    'grep',
                    '-I',
                    '-h',
                    '-o',
                    '-E',
                    self._IDENT,
                    tag,
                    '--',
                    *self._CODE,
                ],
                stdout=subprocess.PIPE,
            ) as words:
                out = subprocess.run(
                    ['grep', '-F', '-x', '-f', patterns],
                    stdin=words.stdout,
                    capture_output=True,
                    text=True,
                    encoding='utf-8',
                    check=False,
                ).stdout
        return set(out.splitlines())

    def names(self) -> list[str]:
        """API names the range removed that Brave uses, or only mentions in
        comments."""
        removed, added = set(), set()
        with subprocess.Popen(
            [
                'git',
                '-C',
                str(self._range.chromium.root),
                'diff',
                '-U0',
                '--no-renames',
                '--no-color',
                self._range.old,
                self._range.new,
                '--',
                *self._CODE,
            ],
            stdout=subprocess.PIPE,
            text=True,
            encoding='utf-8',
            errors='replace',
        ) as process:
            for line in process.stdout:
                if line.startswith(('---', '+++')):
                    continue
                if line.startswith('-'):
                    removed.update(self._WORD.findall(line))
                elif line.startswith('+'):
                    added.update(self._WORD.findall(line))
        candidates = {n for n in removed - added if self._API.fullmatch(n)}
        brave_words = set(
            self._range.brave.grep(
                '-h',
                '-o',
                '-E',
                self._IDENT,
                paths=self._CODE + _NOT_BRAVE_USES,
            ).split()
        )
        used = candidates & brave_words
        gone = used - self._present(self._range.new, used)
        uses, comments = self._code_locations(gone)
        return [
            f'{kind}\t{name}\tbrave: {", ".join(where[name])}'
            for kind, where in (('name', uses), ('comment', comments))
            for name in sorted(where)
        ]

    def files(self) -> list[str]:
        """Files the range deleted or moved that Brave overrides or names."""
        out = self._range.chromium.run(
            'diff',
            '--name-status',
            '--diff-filter=DR',
            self._range.old,
            self._range.new,
        )
        moves: dict[str, str] = {}
        for line in out.splitlines():
            parts = line.split('\t')
            if parts[0].startswith(('D', 'R')) and not parts[1].endswith(
                '.java'
            ):
                moves[parts[1]] = parts[2] if len(parts) > 2 else ''
        brave = self._range.brave
        overrides = set(
            brave.files('chromium_src', 'rewrite', 'build/chromium/resources')
        )
        lines = []
        for path in sorted(moves):
            dest = f' -> {moves[path]}' if moves[path] else ''
            for override in (
                f'chromium_src/{path}',
                f'rewrite/{path}.yaml',
                f'build/chromium/resources/{path}',
            ):
                if override in overrides:
                    lines.append(f'override\t{path}{dest}\t{override}')
        where = self._brave_locations(set(moves), words=False)
        for path in sorted(where):
            dest = f' -> {moves[path]}' if moves[path] else ''
            brave_uses = ', '.join(where[path])
            lines.append(f'file\t{path}{dest}\tbrave: {brave_uses}')
        return lines

    def headers(self) -> list[str]:
        """Names a header replaced outright declares that the original
        doesn't at the new tag: deleted in the range, or before it."""
        brave, chromium = self._range.brave, self._range.chromium
        lines = []
        for path in brave.files('chromium_src'):
            if not path.endswith('.h'):
                continue
            upstream = path[len('chromium_src/') :]
            text = brave.show(path) or ''
            if f'#include <{upstream}>' in text:
                continue
            old = chromium.show(upstream, self._range.old)
            new = chromium.show(upstream, self._range.new)
            if old is None and new is None:
                continue
            names = {
                n
                for n in self._NAME.findall(self._COMMENT.sub('', text))
                if 'Brave' not in n
            } - set(self._NAME.findall(new or ''))
            in_range = names & set(self._NAME.findall(old or ''))
            for status, found in (
                ('deleted in range', in_range),
                ('not upstream', names - in_range),
            ):
                if found:
                    lines.append(
                        f'header\t{path}\t{len(found)} {status}: '
                        f'{", ".join(sorted(found)[:12])}'
                    )
        return lines

    def features(self) -> list[str]:
        """Features Brave overrides that upstream no longer declares."""
        text = self._range.brave.show(self._FEATURES) or ''
        names = set(self._FEATURE.findall(text))
        # Brave's own features: those Brave declares.
        names -= {
            needle[len('FEATURE(k') :]
            for needle in self._brave_locations(
                {f'FEATURE(k{n}' for n in names}, words=False
            )
        }
        needles = {f'"{n}"' for n in names} | {f'k{n}' for n in names}
        old = self._present_fixed(self._range.old, needles)
        new = self._present_fixed(self._range.new, needles)
        lines = []
        for name in sorted(names):
            forms = {f'"{name}"', f'k{name}'}
            if forms & new:
                continue
            status = 'removed in range' if forms & old else 'missing at both'
            lines.append(f'feature\t{self._FEATURES}\t{name}\t{status}')
        return lines

    def _present_fixed(self, tag: str, needles: set[str]) -> set[str]:
        """The needles the C++ sources spell anywhere at `tag`."""
        with self._pattern_file(needles) as patterns:
            out = self._range.chromium.run(
                'grep',
                '-I',
                '-h',
                '-o',
                '-F',
                '-f',
                patterns,
                tag,
                '--',
                '*.cc',
                '*.h',
                '*.mm',
            )
        return set(out.splitlines())

    def run(self) -> list[str]:
        """All of the check's findings, those that predate the range last."""
        lines, debt = [], []
        for line in (
            self.files() + self.headers() + self.features() + self.names()
        ):
            if any(status in line for status in self._PREDATES):
                debt.append(line)
            else:
                lines.append(line)
        return lines + _debt_lines(
            self._range, debt, 'header names and features missing at both tags'
        )


class LiftOverrides:
    """The overrides the lift's own commits added, and the patches whose
    added lines they changed: a regenerated patch's line numbers and context
    are churn, its `+` lines are Brave's change."""

    BASE = 'origin/master'
    _TOUCHING = 3
    _PATHS = ('patches', 'rewrite', 'chromium_src')
    _FIRST_DIFF = re.compile(r'^diff --git a/\S+ b/(\S+)', re.M)

    def __init__(self, range_: Range):
        self._range = range_

    def run(self) -> list[str]:
        """The lift's overrides that range commits touch, each with those
        commits."""
        brave = self._range.brave
        head = brave.ref or 'HEAD'
        span = f'{self._base(head)}...{head}'
        lines = [
            f'added\t{path}'
            for path in brave.run(
                'diff',
                '--name-only',
                '--diff-filter=A',
                span,
                '--',
                *self._PATHS,
            ).splitlines()
        ]
        changed: Counter[str] = Counter()
        path = None
        for line in brave.run(
            'diff', '-U0', '--diff-filter=M', span, '--', 'patches'
        ).splitlines():
            if line.startswith('diff --git '):
                path = line.split(' b/', 1)[-1]
            elif line.startswith(('++', '-+')) and not line.startswith(
                ('+++ ', '--- ')
            ):
                changed[path] += 1
        lines += [
            f'changed {n} added lines\t{path}'
            for path, n in sorted(changed.items())
        ]
        touched, debt = [], []
        for line in lines:
            if commits := self._touching(line):
                touched.append(line + ''.join(f'\n    {c}' for c in commits))
            else:
                debt.append(line)
        return touched + _debt_lines(
            self._range, debt, 'lift overrides no range commit touches'
        )

    def _base(self, head: str) -> str:
        """The branch's upstream, its cr base (`origin/cr<N-1>` while that
        hasn't landed), or `BASE` when `head` has none."""
        try:
            upstream = self._range.brave.run(
                'rev-parse', '--abbrev-ref', f'{head}@{{upstream}}'
            ).strip()
        except subprocess.CalledProcessError:
            upstream = ''
        return upstream or self.BASE

    def _touching(self, line: str) -> list[str]:
        """The range's commits that touched the override's upstream file."""
        override = line.split('\t')[-1]
        if override.startswith('patches/'):
            text = self._range.brave.show(override) or ''
            match = self._FIRST_DIFF.search(text)
            if not match:
                return []
            upstream = PatchAdoptions.upstream_path(override, match.group(1))
        elif override.startswith('rewrite/'):
            upstream = override[len('rewrite/') :].removesuffix('.yaml')
        else:
            upstream = override[len('chromium_src/') :]
        # A Lit mangler is named after its input, with or without `.ts`.
        upstream = upstream.removesuffix('.lit_mangler.ts')
        out = self._range.chromium.run(
            'log',
            '--format=%h %s',
            f'{self._range.old}..{self._range.new}',
            '--',
            upstream,
            f'{upstream}.ts',
        )
        return out.splitlines()[: self._TOUCHING]


_CHECKS = {
    'cited-bugs': CitedBugs,
    'lift-overrides': LiftOverrides,
    'patch-adoptions': PatchAdoptions,
    'test-filters': TestFilters,
    'removed-names': RemovedNames,
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('check', choices=sorted(_CHECKS))
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    parser.add_argument('--brave', help='Read Brave at this ref.')
    parser.add_argument(
        '--debt',
        action='store_true',
        help='List findings that predate the range, rather than count them.',
    )
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    for line in _CHECKS[args.check](
        Range(old, new, args.brave, args.debt)
    ).run():
        print(line)
    return 0


if __name__ == '__main__':
    sys.exit(main())
