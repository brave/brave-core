# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The sanitiser reports in a test run's output.

`ReportParser` reads the output a line at a time, as `runtest.py` streams
it, and gathers one `Report` per finding: an AddressSanitizer,
MemorySanitizer, ThreadSanitizer or UndefinedBehaviorSanitizer error, or a
leak LeakSanitizer reports. A report's `signature` names the bug, not the
run: the sanitiser, the kind of error, and the functions at the top of its
stack, without line numbers, addresses or sizes, so that the same bug has
the same signature from run to run and across code changes.

What tells reports apart is data: the tables below say how each
sanitiser's reports start, which stack frames are the sanitisers' and the
libraries' own rather than the code's, and which output makes a whole run
unreliable.
"""

from __future__ import annotations

from collections.abc import Iterable, Iterator
from dataclasses import dataclass, field
import enum
import hashlib
import json
from pathlib import Path
import re
from typing import Any


class Sanitizer(enum.StrEnum):
    ADDRESS = 'AddressSanitizer'
    LEAK = 'LeakSanitizer'
    MEMORY = 'MemorySanitizer'
    THREAD = 'ThreadSanitizer'
    UNDEFINED = 'UndefinedBehaviorSanitizer'


# How each sanitiser's report starts. The `kind` group, where there is one,
# names the kind of error.
_REPORT_STARTS: tuple[tuple[Sanitizer, re.Pattern[str]], ...] = (
    (Sanitizer.LEAK, re.compile(r'==\d+==ERROR: LeakSanitizer: ')),
    (
        Sanitizer.ADDRESS,
        re.compile(r'==\d+==ERROR: AddressSanitizer: (?P<kind>[\w-]+)'),
    ),
    (
        Sanitizer.MEMORY,
        re.compile(r'==\d+==WARNING: MemorySanitizer: (?P<kind>[\w-]+)'),
    ),
    (
        Sanitizer.THREAD,
        re.compile(r'WARNING: ThreadSanitizer: (?P<kind>[\w ]+?)(?: \(|$)'),
    ),
    (
        Sanitizer.UNDEFINED,
        re.compile(
            r'^(?P<location>\S+:\d+(?::\d+)?): runtime error: (?P<kind>.*)'
        ),
    ),
)

# The values in an UndefinedBehaviorSanitizer error, which differ from run to
# run, e.g. the operands of an overflow.
_UNDEFINED_BEHAVIOUR_VALUES = re.compile(r'\b(?:0x[0-9a-f]+|\d+)\b')

# How every report ends, with the sanitiser's one-line summary.
_SUMMARY = re.compile(r'^SUMMARY: \w+Sanitizer: (?P<summary>.*)')

# A leak in a LeakSanitizer report: each starts its own stack.
_LEAK = re.compile(r'^(?P<directness>Direct|Indirect) leak of \d+ byte')

# A stack frame, as the sanitisers print one once symbolised: its function,
# then its source file, with a line and column where the symboliser knows
# them.
_FRAME = re.compile(
    r'^\s*#\d+ 0x[0-9a-f]+ in (?P<function>.+?)'
    r'(?: (?P<file>[^\s()]*/[^\s():/]+|[^\s():/]+\.[A-Za-z0-9+]+|\?\?)'
    r'(?::(?P<line>\d+)(?::\d+)?)?)?$'
)

# The module offset a frame without debug information names instead of a
# file, e.g. ` (/lib/x86_64-linux-gnu/libc.so.6+0x29d8f)`.
_MODULE_OFFSET = re.compile(r' \([^()]+\+0x[0-9a-f]+\)$')

# The qualifiers that may follow a function's parameter list.
_TRAILING_QUALIFIERS = re.compile(r'(?: (?:const|volatile|noexcept|&|&&))+$')

# The frames that are the sanitisers', the allocators', the standard
# libraries' or the callback machinery's rather than the code's, which a
# signature leaves out: they are at the top of nearly every stack, and say
# nothing about where the bug is. They match a frame's bare function name.
_RUNTIME_FRAMES: tuple[re.Pattern[str], ...] = tuple(
    re.compile(pattern)
    for pattern in (
        # The sanitiser runtimes and their interceptors.
        r'^_*(?:asan|lsan|msan|tsan|ubsan|sanitizer)_',
        r'^_*interceptor_',
        # Allocation, in C, C++ and Rust.
        r'^(?:malloc|calloc|realloc|free|memalign|posix_memalign|valloc)$',
        r'^aligned_alloc$',
        r'^operator (?:new|delete)',
        r'^partition_alloc::',
        r'^allocator_shim::',
        r'^__rustc::',
        r'^__rust_',
        # The standard libraries, C++ and Rust, including Rust's impls on
        # slices and primitive types.
        r'^<?(?:std|alloc|core)::',
        r'^(?:std::)?make_(?:unique|shared)$',
        r'^<\[',
        r'^<(?:u8|str|char) as ',
        # Chromium's reference counting and callback machinery, which the
        # symboliser names without its namespace when inlined.
        r'^(?:base::)?MakeRefCounted$',
        r'^base::internal::',
        r'^base::(?:Once|Repeating)Callback::',
        r'^(?:Invoke|MakeItSo|RunImpl|RunOnce|Run|Create|BindImpl)$',
    )
)

# The output that makes a whole run's reports unreliable: the sanitiser ran
# out of memory, and reports what it could not do rather than a bug.
_UNRELIABLE_RUN_OUTPUT: tuple[str, ...] = (
    'AddressSanitizer failed to allocate',
    'ERROR: Failed to mmap',
    '"unable to mmap"',
)

# How many of a report's own frames its signature names.
SIGNATURE_FRAMES = 3

# The test a line of output says has started.
_TEST_START = re.compile(r'^\[ RUN      \] (?P<test>\S+)')


@dataclass(frozen=True)
class Frame:
    """A stack frame of a report."""

    # The function, without its parameter list.
    function: str
    # Where in the source, as `file:line` relative to `src/`, if the frame is
    # symbolised.
    location: str | None = None

    @classmethod
    def parse(cls, line: str) -> Frame | None:
        """The frame `line` prints, if it is one."""
        match = _FRAME.match(line)
        if match is None:
            return None
        location = None
        if match['file'] and match['file'] != '??':
            location = match['file'].removeprefix('../../')
            if match['line']:
                location += f':{match["line"]}'
        return cls(_bare_function(match['function']), location)

    @property
    def is_runtime(self) -> bool:
        """Whether the frame is the runtime's rather than the code's."""
        return any(pattern.search(self.function) for pattern in _RUNTIME_FRAMES)


def _bare_function(function: str) -> str:
    """`function` as a signature names it: without its module offset,
    parameters, qualifiers, template arguments or return type."""
    function = _MODULE_OFFSET.sub('', function)
    function = _TRAILING_QUALIFIERS.sub('', function)
    if function.endswith(')'):
        function = _without_trailing_group(function, '(', ')')
    function = _without_template_arguments(function)
    # A return type, which the symboliser prints for some templates, e.g.
    # `scoped_refptr<T> base::MakeRefCounted<T>(...)`.
    words = _split_outside_brackets(function)
    if len(words) > 1 and not words[-2].endswith('operator'):
        function = words[-1]
    return function


def _without_trailing_group(text: str, opening: str, closing: str) -> str:
    """`text` without the bracketed group it ends with."""
    depth = 0
    for index in range(len(text) - 1, -1, -1):
        if text[index] == closing:
            depth += 1
        elif text[index] == opening:
            depth -= 1
            if depth == 0:
                return text[:index] or text
    return text


def _without_template_arguments(name: str) -> str:
    """`name` without the template arguments that follow a name, keeping a
    Rust `<Type as Trait>::f` qualified path, inner arguments aside."""
    result: list[str] = []
    depth = 0
    for index, char in enumerate(name):
        follows_name = index > 0 and (
            name[index - 1].isalnum() or name[index - 1] in '_>'
        )
        if char == '<' and (depth or follows_name):
            depth += 1
        elif char == '>' and depth:
            depth -= 1
        elif not depth:
            result.append(char)
    return ''.join(result)


def _split_outside_brackets(text: str) -> list[str]:
    """`text`'s words, splitting at the spaces outside any brackets."""
    words, depth, start = [], 0, 0
    for index, char in enumerate(text):
        if char in '<([':
            depth += 1
        elif char in '>)]':
            depth -= 1
        elif char == ' ' and depth == 0:
            words.append(text[start:index])
            start = index + 1
    words.append(text[start:])
    return [word for word in words if word]


@dataclass(frozen=True)
class Report:
    """One finding of a sanitiser."""

    sanitizer: Sanitizer
    # What went wrong, e.g. `heap-use-after-free`, `direct-leak` or
    # `signed integer overflow`.
    kind: str
    # The sanitiser's summary line, without its `SUMMARY:` prefix.
    summary: str
    # The stack the finding is about: where the error happened, or where the
    # leaked memory was allocated.
    frames: tuple[Frame, ...]
    # The report, as the sanitiser printed it.
    text: str
    # The test that was running when the report started, if any. A leak is
    # reported as its process exits, so it may belong to any test that
    # process ran.
    test: str | None = None

    @property
    def own_frames(self) -> tuple[Frame, ...]:
        """The frames that are the code's rather than the runtime's."""
        return tuple(frame for frame in self.frames if not frame.is_runtime)

    @property
    def signature(self) -> str:
        """What the finding is: the same bug has the same signature from run
        to run, wherever its lines move."""
        frames = self.own_frames[:SIGNATURE_FRAMES]
        stack = ' < '.join(
            frame.function or frame.location or '?' for frame in frames
        )
        return f'{self.sanitizer}: {self.kind} in {stack or "?"}'

    @property
    def id(self) -> str:
        """A short digest of the signature, to search for the finding by."""
        return hashlib.sha256(self.signature.encode()).hexdigest()[:16]

    @property
    def headline(self) -> str:
        """A one-line description: the kind of error and where it happened."""
        top = self.own_frames[0] if self.own_frames else None
        if top is None:
            return f'{self.sanitizer} {self.kind}'
        where = f' ({top.location})' if top.location else ''
        return f'{self.sanitizer} {self.kind} in {top.function}{where}'

    def to_json(self) -> dict[str, Any]:
        return {
            'sanitizer': str(self.sanitizer),
            'kind': self.kind,
            'summary': self.summary,
            'frames': [
                {'function': frame.function, 'location': frame.location}
                for frame in self.frames
            ],
            'text': self.text,
            'test': self.test,
            'signature': self.signature,
            'id': self.id,
        }

    @classmethod
    def from_json(cls, data: dict[str, Any]) -> Report:
        return cls(
            sanitizer=Sanitizer(data['sanitizer']),
            kind=data['kind'],
            summary=data['summary'],
            frames=tuple(
                Frame(frame['function'], frame['location'])
                for frame in data['frames']
            ),
            text=data['text'],
            test=data['test'],
        )


@dataclass(frozen=True)
class RunReports:
    """The findings of a test run, as `runtest.py` writes them for the
    recipes to read."""

    reports: tuple[Report, ...] = ()
    # The output that makes the run's reports unreliable, if any: they may
    # be the sanitiser's failures rather than the code's.
    unreliable: tuple[str, ...] = ()

    def to_json(self) -> dict[str, Any]:
        return {
            'reports': [report.to_json() for report in self.reports],
            'unreliable': list(self.unreliable),
        }

    @classmethod
    def from_json(cls, data: dict[str, Any]) -> RunReports:
        return cls(
            reports=tuple(
                Report.from_json(report) for report in data['reports']
            ),
            unreliable=tuple(data['unreliable']),
        )

    def write(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            json.dumps(self.to_json(), indent=2) + '\n', encoding='utf-8'
        )

    @classmethod
    def read(cls, path: Path) -> RunReports:
        return cls.from_json(json.loads(path.read_text(encoding='utf-8')))


@dataclass
class _OpenReport:
    """A report being read, up to its summary line."""

    sanitizer: Sanitizer
    kind: str
    test: str | None
    lines: list[str] = field(default_factory=list)
    # Where a UBSan error happened, which its report may print no stack for.
    location: str | None = None

    def close(self, summary: str) -> Iterator[Report]:
        """The findings of the report, now that it has ended."""
        if self.sanitizer is Sanitizer.LEAK:
            yield from self._leaks(summary)
            return
        frames = _first_stack(self.lines)
        if not frames and self.location:
            frames = (Frame('', self.location.removeprefix('../../')),)
        yield Report(
            sanitizer=self.sanitizer,
            kind=self.kind,
            summary=summary,
            frames=frames,
            text='\n'.join(self.lines),
            test=self.test,
        )

    def _leaks(self, summary: str) -> Iterator[Report]:
        """A LeakSanitizer report's findings: each of its direct leaks, one
        per signature. A report of indirect leaks alone is one finding,
        which its first leak, the largest, stands for: they are all held
        by memory LeakSanitizer did not report as leaked."""
        blocks: list[tuple[str, list[str]]] = []
        for line in self.lines[1:]:
            if match := _LEAK.match(line):
                blocks.append((match['directness'], [line]))
            elif blocks:
                blocks[-1][1].append(line)
        direct = [
            block for directness, block in blocks if directness == 'Direct'
        ]
        if not direct:
            if blocks:
                yield self._leak(
                    'indirect-leak',
                    summary,
                    blocks[0][1],
                    '\n'.join(self.lines),
                )
            return
        seen = set()
        for block in direct:
            report = self._leak(
                'direct-leak',
                summary,
                block,
                '\n'.join([self.lines[0], *block]),
            )
            if report.signature not in seen:
                seen.add(report.signature)
                yield report

    def _leak(
        self, kind: str, summary: str, block: list[str], text: str
    ) -> Report:
        return Report(
            sanitizer=Sanitizer.LEAK,
            kind=kind,
            summary=summary,
            frames=_first_stack(block),
            text=text,
            test=self.test,
        )


def _first_stack(lines: Iterable[str]) -> tuple[Frame, ...]:
    """The frames of the first stack in `lines`."""
    frames: list[Frame] = []
    for line in lines:
        frame = Frame.parse(line)
        if frame is not None:
            frames.append(frame)
        elif frames:
            break
    return tuple(frames)


def _kind(sanitizer: Sanitizer, kind: str) -> str:
    """The kind of error a report starts with, as its signature names it."""
    if sanitizer is not Sanitizer.UNDEFINED:
        return kind
    # e.g. `signed integer overflow: 2147483647 + 1 cannot be represented`.
    check = kind.split(': ', 1)[0]
    return _UNDEFINED_BEHAVIOUR_VALUES.sub('N', check).strip()


class ReportParser:
    """Gathers the sanitiser reports in output read a line at a time."""

    def __init__(self) -> None:
        self._reports: list[Report] = []
        self._unreliable: list[str] = []
        self._test: str | None = None
        self._open: _OpenReport | None = None

    def process_line(self, line: str) -> None:
        if self._open is not None:
            self._open.lines.append(line)
            if match := _SUMMARY.match(line):
                self._reports.extend(self._open.close(match['summary']))
                self._open = None
            return
        if match := _TEST_START.match(line):
            self._test = match['test']
            return
        for output in _UNRELIABLE_RUN_OUTPUT:
            if output in line:
                self._unreliable.append(line.strip())
        for sanitizer, start in _REPORT_STARTS:
            if match := start.search(line):
                groups = match.groupdict()
                self._open = _OpenReport(
                    sanitizer=sanitizer,
                    kind=_kind(sanitizer, groups.get('kind') or 'leak'),
                    test=self._test,
                    lines=[line],
                    location=groups.get('location'),
                )
                return

    def finish(self) -> None:
        """Keeps the report the output ended in, if its process died before
        it reached its summary."""
        if self._open is not None:
            self._reports.extend(self._open.close(''))
            self._open = None

    @property
    def result(self) -> RunReports:
        """The findings so far, in the order they were reported."""
        return RunReports(tuple(self._reports), tuple(self._unreliable))
