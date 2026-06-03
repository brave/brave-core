# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Rebase v2 -- an alternative `brockit rebase` code path.

Wired behind `brockit.py rebase --v2`. The module exposes two
file-in / file-out transforms that brockit hooks into git via the
`GIT_SEQUENCE_EDITOR` and `GIT_EDITOR` env vars during a
`git rebase --interactive`:

* `rewrite_plan(todo_file, …)` -- reorders the rebase TODO produced by
  `GIT_SEQUENCE_EDITOR`. Pinned commits get grouped to the top, recyclable
  commits get optionally evicted, reassign commits get placed next to
  their target.
* `MessageWriter.parse(todo_file)` + `rewrite_with_last_message()` --
  composes the squash commit message produced by `GIT_EDITOR`, picking
  whichever surviving message corresponds to the kind of squash being
  performed (`PINNED` or `REASSIGNMENT`).

Whenever either step can't safely transform the file, it raises
`EditorRecoverableFailure`. The brockit dispatch catches it and routes
the file through `hand_off_to_editor`, which prepends the failure reason
as a `#`-comment and opens the user's editor.

Commit categories formalised here (see `EntryType`):

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
    - [cr*] Disable failing upstream tests.

* RECYCLABLE -- commits we can always drop and regenerate with tooling.
  They're discarded by `brockit.py rebase --discard-regen-changes`.
  Subjects that fit this category:

    - Update patches from Chromium * to Chromium *.
    - Updated strings for Chromium *.

* REASSIGNMENT -- `reassign!<hash>!` commits authored by
  `brockit reassign`. Repositioned next to their target when
  `pinned_squashed=True`.
* DEFAULT -- everything else.
"""

from __future__ import annotations

import enum
import logging
import os
import re
import shlex
from dataclasses import dataclass
from pathlib import Path

from terminal import terminal

# Brockit-specific prefix for commit messages, similar to `fixup!`.
REASSIGN_COMMIT_MSG_PREFIX = 'reassign!'

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

# Matches the contiguous `!`-separated subcommand prefix of a comment, e.g.
# `reassign!hash!`, `amend!`, `fixup!`. Git already uses `amend!`, `fixup!`,
# but brockit uses this format to make it a `!`-separated list for subcommands
# (e.g `reassign!hash!` → `['reassign', 'hash']`). The first token must start
# with a letter (it's the subcommand name itself -- `reassign`, `amend`,
# `fixup`, `squash`); subsequent tokens are allowed to start with a digit so
# commit short-hashes like `859ab9caa74` are captured intact.
_SUBCOMMAND_RE = re.compile(r'^[A-Za-z]\w*!(?:\w+!)*')

# Matches a `# <word>! <subject>` autosquash marker line inside a squash
# editor block (e.g. `# amend! Update from Chromium …`). Group 1 captures
# the marker word with its trailing `!`; group 2 captures the rest of
# the line as the commit-subject suffix.
_AUTOSQUASH_NOTE_RE = re.compile(r'#\s*(\w+!)\s*(.+)')


def _strip_cr_tag(subject: str) -> str:
    """Returns `subject` with any leading `[crNNN]` (or `[crNNN][...]…`)
    tag and the trailing whitespace removed. A subject that doesn't
    start with such a tag is returned unchanged."""
    return _CR_TAG_RE.sub('', subject, count=1)


def _strip_autosquash_prefixes(subject: str) -> str:
    """Returns `subject` with any leading stack of `fixup! ` / `amend! `
    / `squash! ` prefixes removed. Subjects without those prefixes are
    returned unchanged."""
    return _AUTOSQUASH_PREFIX_RE.sub('', subject)


def _match(subject: str, pattern: re.Pattern) -> bool:
    """True if `pattern` fully matches `subject` after stripping any
    leading `fixup!` / `amend!` / `squash!` autosquash prefixes and any
    `[crNNN]` tag. The stripped subject is also right-trimmed so a
    trailing space in the original line doesn't break the fullmatch."""
    cleaned = _strip_cr_tag(_strip_autosquash_prefixes(subject)).rstrip()
    return pattern.fullmatch(cleaned) is not None


# Ordered list of `(pinned-group-id, regex)` entries. The order *is* the
# squash priority order: when multiple pinned groups are present in the
# same rebase plan, they're emitted top-to-bottom in this order
# (version → plaster_reruns → conflict → gnrt → IWYU →
# resource_ids → lit_mangler → disable_tests). The regexes are
# anchored fullmatches applied to the `[crNNN]`-stripped subject, so each
# pattern matches both the tagged and the untagged form of the same commit.
PINNED_GROUPS = [
    ('version',
     re.compile(rf'Update from Chromium {_CR_VERSION} to '
                rf'Chromium {_CR_VERSION}\.?')),
    ('plaster_reruns',
     re.compile(rf'Apply-fixed 🩹 patches from Chromium {_CR_VERSION} to '
                rf'Chromium {_CR_VERSION}\.?')),
    ('conflict',
     re.compile(rf'Conflict-resolved patches from Chromium {_CR_VERSION} to '
                rf'Chromium {_CR_VERSION}\.?')),
    ('gnrt', re.compile(rf'`gnrt` run for Chromium {_CR_VERSION}\.?')),
    ('iwyu', re.compile(r'IWYU fixes\.?')),
    ('resource_ids', re.compile(r'Bump resource_ids\.?')),
    ('lit_mangler', re.compile(r'Update Lit mangler snapshot\.?')),
    ('disable_tests', re.compile(r'Disable failing upstream tests\.?')),
]

# Subjects produced by `npm run update_patches` and string generation --
# evicted when `discard_recyclable=True`.
RECYCLABLE_PATTERNS = [
    re.compile(rf'Update patches from Chromium {_CR_VERSION} to '
               rf'Chromium {_CR_VERSION}\.?'),
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


def get_entry_type_for_subject(subject: str) -> 'EntryType':
    """Classifies a bare commit subject (no TODO command/hash prefix)
    into one of the four `EntryType` variants."""
    if subject.startswith(REASSIGN_COMMIT_MSG_PREFIX):
        return EntryType.REASSIGNMENT
    if get_pinned_group_for_subject(subject) is not None:
        return EntryType.PINNED
    if is_recyclable(subject):
        return EntryType.RECYCLABLE
    return EntryType.DEFAULT


def get_git_editor(primary_env: str = 'GIT_EDITOR') -> str:
    """Returns the editor brockit should fall back to when a v2 rebase
    step can't transform the file it was handed. Resolution order:

    Must be called BEFORE the caller overrides `GIT_SEQUENCE_EDITOR` /
    `GIT_EDITOR` (e.g. before `Rebase.execute` sets its own dispatch as
    the editor).
    """
    fallbacks = [primary_env]
    if primary_env != 'GIT_EDITOR':
        fallbacks.append('GIT_EDITOR')
    fallbacks.extend(['VISUAL', 'EDITOR'])
    for env_name in fallbacks:
        value = os.environ.get(env_name)
        if not value:
            continue
        if 'brockit.py' in value and '--internal-rebase-' in value:
            raise NotImplementedError(
                f'${env_name} is already set to a brockit internal '
                f'dispatch ({value!r}). `get_git_editor` must run '
                f'BEFORE brockit overrides the env vars -- otherwise '
                f'the editor fallback would loop on ourselves.')
        return value
    return 'vim'


def hand_off_to_editor(todo_file: Path, reason: str, editor: str) -> None:
    """Spawns `editor` on `todo_file` and waits for it.

    `editor` is the caller-resolved editor command (typically the value
    captured by `get_git_editor` and threaded through via
    `--internal-rebase-crash-*-editor`). The caller is responsible for
    making sure it's non-empty before getting here -- this function
    has no env-var fallback of its own.

    When `reason` is non-empty, each of its lines is prepended to the
    file as a `#`-comment so the user sees why we couldn't rewrite the
    message automatically. Lines starting with `#` already are passed
    through unchanged.
    """
    if reason:
        intro = ('# B🚀 has been unable to handle this rebase. Investigate '
                 'why, and\n'
                 '# file a bug. Failure reason below.\n')
        prefix = intro + ''.join(
            (line if line.startswith('#') else f'# {line}') + '\n'
            for line in reason.splitlines())
        todo_file.write_text(prefix + todo_file.read_bytes().decode('utf-8'),
                             encoding='utf-8',
                             newline='')

    # `editor` may be a multi-token command (e.g. `vim -X`,
    # `code --wait`, `my_editor --feature="with space"`).
    terminal.run(shlex.split(editor) + [str(todo_file)], interactive=True)


class EntryType(enum.Enum):
    """The type of a given change based on the subject line.

    * `PINNED`       -- subject matches one of `PINNED_GROUPS`.
    * `RECYCLABLE`   -- subject matches one of `RECYCLABLE_PATTERNS`.
    * `REASSIGNMENT` -- subject (or autosquash subcommand) starts with
                        `reassign!`.
    * `DEFAULT`      -- none of the above; produced only by `EntryLine`.
                        `MessageWriter.parse` never returns this value
                        -- it raises `EditorRecoverableFailure` instead.
    """
    PINNED = 'pinned'
    RECYCLABLE = 'recyclable'
    REASSIGNMENT = 'reassignment'
    DEFAULT = 'default'


class EditorRecoverableFailure(Exception):
    """Parsing failures during rebase that should be investigated manually.

    This is used for cases where some unrecognised pattern is found and the
    code we have doesn't know how to handle it. This shouldn't happen, but if
    it does, we want to be able to inspect things, take over the process
    manually, and even correct the issue later on in brockit.

    The expected handling is for the actual editor to be launched for manual
    intervention, with the failure reason prepended as a comment to the file.

    This exception is *not* for actual violation of invariants. For those use
    more appropriate, non-recoverable exceptions. This is also not for natural
    user prompting. If anything like this is needed in the future, we should
    have proper flows that do not involve the use of exceptions for control
    flow.
    """


class EntryLine:
    """A parsed line from a `git rebase --interactive` TODO file.
    """

    def __init__(
            self,
            *,
            out: str,
            command: str,
            # Disabling pylint here as `hash` is an appropriate word.
            hash: str,  # pylint: disable=redefined-builtin
            message: str,
            subcommand: list | None = None,
            note: str | None = None):

        # The text for the entry line in the file. This is loaded from the
        # file, however it does get updated when fields in this class are
        # updated.
        self._out = out

        # The rebase command word(s) before the hash: `pick`, `edit`,
        # `squash`, `fixup`, `fixup -C`, etc. May contain spaces.
        self._command = command

        # The commit short-hash that follows the command.
        self._hash = hash

        # The `!`-separated autosquash prefix tokens, with the trailing
        # empty segment dropped (e.g. `['reassign', 'c080270dd7e']`).
        self._subcommand = subcommand

        # The commit subject. The subcommand is already pilled off in this
        # field, which means, no `fixup!`, `amend!`, `reassign!`, etc.
        self._message = message

        # The trailing ` # <note>` marker (e.g. `'empty'`), or `None`.
        self._note = note

        # Classification fields. `_entry_type` defaults to DEFAULT and
        # is overridden below based on the subcommand/message; the
        # result is read-only for the rest of this `EntryLine`'s
        # lifetime.
        self._pinned_group: str | None = None
        self._entry_type: EntryType = EntryType.DEFAULT
        if self._subcommand and self._subcommand[0] == 'reassign':
            self._entry_type = EntryType.REASSIGNMENT
        elif is_recyclable(self._message):
            self._entry_type = EntryType.RECYCLABLE
        else:
            pinned_group = get_pinned_group_for_subject(self._message)
            if pinned_group is not None:
                self._pinned_group = pinned_group
                self._entry_type = EntryType.PINNED

    # ----- read-only properties ---------------------------------------------

    @property
    def out(self) -> str:
        return self._out

    @property
    def hash(self) -> str:
        return self._hash

    @property
    def message(self) -> str:
        return self._message

    @property
    def subcommand(self) -> list | None:
        return self._subcommand

    @property
    def note(self) -> str | None:
        return self._note

    @property
    def pinned_group(self) -> str | None:
        return self._pinned_group

    @property
    def entry_type(self) -> 'EntryType':
        return self._entry_type

    @property
    def is_pinned(self) -> bool:
        return self._entry_type is EntryType.PINNED

    @property
    def is_recyclable(self) -> bool:
        return self._entry_type is EntryType.RECYCLABLE

    @property
    def is_reassignment(self) -> bool:
        return self._entry_type is EntryType.REASSIGNMENT

    @property
    def reassign_target_hash(self) -> str | None:
        """The target hash carried in the `reassign!<hash>!` prefix --
        or `None` if the prefix has no hash. Raises
        `NotImplementedError` on a non-reassignment commit (callers
        should gate on `is_reassignment` first)."""
        if not self.is_reassignment:
            raise NotImplementedError(
                'reassign_target_hash is only defined for reassignment '
                'commits')
        if self._subcommand and len(self._subcommand) >= 2:
            return self._subcommand[1]
        return None

    # ----- the one mutable property -----------------------------------------

    @property
    def command(self) -> str:
        """Assigning to this property also updates `out` so the emitted
        line stays in sync."""
        return self._command

    @command.setter
    def command(self, new_command: str) -> None:
        self._command = new_command
        self._refresh_out()

    # ----- public methods ---------------------------------------------------

    @staticmethod
    def parse(line: str) -> 'EntryLine | None':
        """Parses a TODO line into a `EntryLine`.

        Returns `None` for blank or pure-comment lines. Raises
        `EditorRecoverableFailure` for non-empty lines that don't match
        the expected `<command> <hash> # <comment>` shape -- the caller
        is expected to surface the failure via `hand_off_to_editor`.
        """
        if not line.strip() or line.lstrip().startswith('#'):
            return None

        # Split into at most three sections: command-part, comment, and an
        # optional trailing ` # <note>` marker (e.g. ` # empty`). Capping
        # the split keeps any `#` inside the comment itself intact.
        parts = line.rstrip().split('#', maxsplit=2)
        if len(parts) < 2:
            raise EditorRecoverableFailure(
                f'Cannot parse TODO line (no comment): {line!r}')

        cmd_part = parts[0].rstrip()
        comment = parts[1].strip()
        note = (parts[2].strip() or None) if len(parts) == 3 else None

        # The hash is the last whitespace-separated token of the command
        # part; everything before it is the command (e.g. `pick`, `edit`,
        # `fixup -C`).
        comand_parts = cmd_part.rsplit(maxsplit=1)
        if len(comand_parts) != 2:
            raise EditorRecoverableFailure(
                f'Cannot parse TODO line (no hash): {line!r}')
        command, commit_hash = comand_parts

        subcommand = None
        sub_match = _SUBCOMMAND_RE.match(comment)
        if sub_match:
            subcommand = [tok for tok in sub_match.group(0).split('!') if tok]
            message = comment[sub_match.end():].lstrip()
        else:
            message = comment

        return EntryLine(out=line,
                         command=command,
                         hash=commit_hash,
                         subcommand=subcommand,
                         message=message,
                         note=note)

    # ----- private helpers --------------------------------------------------

    def _refresh_out(self) -> None:
        """Regenerates `_out` from the current field values in the
        canonical `<command> <hash> # [<sub1>!<sub2>! ]<message>[ # <note>]`
        form. Called automatically by the `command` setter."""
        subcommand_prefix = (''.join(f'{tok}!' for tok in self._subcommand) +
                             ' ' if self._subcommand else '')
        line = (f'{self._command} {self._hash} # '
                f'{subcommand_prefix}{self._message}')
        if self._note:
            line = f'{line} # {self._note}'
        self._out = line


def rewrite_plan(*,
                 todo_file: Path,
                 pinned_squashed: bool = False,
                 discard_recyclable: bool = False) -> None:
    """Rewrites a `git rebase --interactive` TODO file in place.

    The two operations are independent and may be used alone or
    combined. When both flags are `False` this is a no-op: the file is
    neither read nor written.

    Args:
        todo_file: Path to the `git-rebase-todo` file that
            `GIT_SEQUENCE_EDITOR` was invoked with.
        pinned_squashed: When `True`, group pinned commits at the top
            of the plan, in `PINNED_GROUPS` priority order. The first
            commit in each group stays `pick`; subsequent ones become
            `squash`. Reassign commits are repositioned to sit
            immediately above their target (with the target rewritten
            to `squash`). Defaults to `False`.
        discard_recyclable: When `True`, drop recyclable commits from
            the plan entirely. Defaults to `False`.

    Raises:
        EditorRecoverableFailure: Any TODO line that doesn't parse
            (malformed shape, missing hash, etc.) bubbles up so the
            caller can punt to `hand_off_to_editor`.
    """

    if not pinned_squashed and not discard_recyclable:
        # Nothing to do -- use whatever plan git produced.
        return

    pinned_groups = {group: [] for group, _ in PINNED_GROUPS}
    all_others = []

    def add_reassign_before_target(reassign: EntryLine) -> None:
        """Looks for `reassign`'s target in `all_others` and, if found,
        inserts `reassign` immediately above it while rewriting the
        target to `squash`. Reassignment targets are required to be
        regular commits -- pinned and recyclable commits aren't
        supported, so a reassign whose target is pinned (or got dropped
        as recyclable) ends up orphaned and silently discarded."""

        def find_target(predicate) -> tuple[EntryLine | None, int | None]:
            """Walks `all_others` in reverse order and returns the first
            entry that satisfies `predicate`, paired with its index. The
            reverse walk is a small optimisation: a reassign's target is
            usually the immediately-preceding entry_line, so the typical hit
            is on the first iteration. Returns `(None, None)` when no
            entry matches."""
            return next(((c, i) for c, i in zip(
                reversed(all_others), range(len(all_others) - 1, -1, -1))
                         if predicate(c)), (None, None))

        target, target_idx = None, None
        if reassign.reassign_target_hash is not None:
            target, target_idx = find_target(
                lambda c: c.hash == reassign.reassign_target_hash)

        if target is None:
            # Fallback search by commit message. This is allowed.
            target, target_idx = find_target(
                lambda c: c.message == reassign.message)

        if target is None:
            # Reassignment is orphaned -- drop it and do nothing.
            logging.warning('Dropping orphaned reassignment: %s',
                            reassign.out.strip())
            return

        # The `target` becomes `squash` so its message/authorship is
        # absorbed by the reassign; the reassign keeps its `pick` and
        # slots in right above. The `command` setter on `EntryLine` keeps
        # `out` in sync automatically.
        target.command = 'squash'
        all_others.insert(target_idx, reassign)

    for line in todo_file.read_bytes().decode('utf-8').splitlines():
        entry_line = EntryLine.parse(line)
        if entry_line is None:
            # Irrelevant line (blank or comment).
            continue

        # Dropping discardable commits if we don't need them.
        if discard_recyclable and entry_line.is_recyclable:
            continue

        if pinned_squashed:
            if entry_line.is_reassignment:
                add_reassign_before_target(entry_line)
                continue
            if entry_line.pinned_group is not None:
                # First commit in the group becomes `pick`; subsequent
                # ones become `squash`. The setter keeps `out` in sync.
                entry_line.command = (
                    'pick' if not pinned_groups[entry_line.pinned_group] else
                    'squash')
                pinned_groups[entry_line.pinned_group].append(entry_line)
                continue

        all_others.append(entry_line)

    new_plan = [
        *(c.out for group in pinned_groups.values() for c in group),
        *(c.out for c in all_others)
    ]

    todo_file.write_text('\n'.join(new_plan) + '\n',
                         encoding='utf-8',
                         newline='')


_COMMIT_MSG_HEADER_GUARDS = re.compile(
    r'^# (This is the (?:1st|\d+(?:st|nd|rd|th)) commit message:|'
    r'This is the commit message #\d+:|'
    r'The commit message #\d+ will be skipped:)$')


class MsgBlock:
    """One commit-message block carved out of a `GIT_EDITOR` squash file.

    When git squashes N commits, it builds a single editor file whose
    body is the concatenation of each original commit's message, each
    preceded by a `# This is …` header line that git itself inserts.
    `MsgBlock` is the parsed unit for one of those sections.

    Example (one squash combining three commits):

        # This is a combination of 3 commits.
        # This is the 1st commit message:

        Update from Chromium 1.0.0.0 to Chromium 1.0.0.1

        # This is the commit message #2:

        # amend! Update from Chromium 1.0.0.0 to Chromium 1.0.0.1

        Tweaked: dropped an extraneous header bump.

        # The commit message #3 will be skipped:

        # fixup! Update from Chromium 1.0.0.0 to Chromium 1.0.0.1

        # Please enter the commit message for your changes. …

    `MsgBlock.parse` walks the file and produces:

    * Block 1 -- `full_message='Update from Chromium 1.0.0.0 to
      Chromium 1.0.0.1'`, `note=None` (no autosquash marker).
    * Block 2 -- `full_message='Tweaked: dropped an extraneous header
      bump.'`, `note=OriginalCommitNote(command='amend!', first_line=...)`.
    * Block 3 (the `will be skipped` block) is skipped by the parser and not
      supported.
    """

    @dataclass(frozen=True)
    class OriginalCommitNote:
        """The autosquash marker line that git inserts inside some
        commit-message blocks.
        """

        # The marker word including its trailing `!`. One of `'amend!'`,
        # `'squash!'`, `'fixup!'`.
        command: str

        # The commit-subject suffix on the same line as the marker.
        first_line: str

        @staticmethod
        def from_line(line: str) -> 'MsgBlock.OriginalCommitNote | None':
            """Parses `line` as an autosquash marker. Returns an
            `OriginalCommitNote` when it matches `# <word>! <subject>`,
            `None` otherwise (e.g. a plain content line)."""
            match = _AUTOSQUASH_NOTE_RE.match(line)
            if match:
                return MsgBlock.OriginalCommitNote(command=match.group(1),
                                                   first_line=match.group(2))
            return None

    def __init__(self,
                 *,
                 full_message: str,
                 note: 'MsgBlock.OriginalCommitNote | None' = None):

        # The commit body for this block with `#`-comment lines removed
        # and leading / trailing blanks trimmed.
        self._full_message = full_message

        # The autosquash marker that opened this block, or `None` when
        # the block had no marker.
        self._note = note

    @property
    def full_message(self) -> str:
        return self._full_message

    @property
    def note(self) -> 'MsgBlock.OriginalCommitNote | None':
        return self._note

    def _extend_message(self, more: str) -> None:
        """Appends `more` on a new line to `_full_message`. Used by
        `parse` when merging a `# squash!` block into its predecessor;
        kept private so external callers can't mutate a block's body
        ad-hoc."""
        self._full_message = f'{self._full_message}\n{more}'

    @staticmethod
    def parse(text: str) -> list['MsgBlock']:
        """Splits `text` into a list of `MsgBlock`s, in source order.

        This function goes over each block in the produced editor file and
        extracts the commit messages for each case. Blocks for autosquash
        changes are skipped. Blocks for `squash!` are merged into the previous
        block. The rest are emitted as-is.

        Raises `EditorRecoverableFailure` for malformed input (too few
        lines for a valid block, an empty body, or a `squash!` pointing
        at an empty previous block).
        """

        def get_message_blocks_ranges(lines: list[str]):
            start_idx = None
            for i, line in enumerate(lines):
                # Stop at the Git boilerplate
                if line.startswith("# Please enter the commit message"):
                    if start_idx is not None:
                        yield lines[start_idx:i]
                    return  # Cleanly exit the generator

                # When we hit a new header, yield the slice of the
                # previous block.
                if _COMMIT_MSG_HEADER_GUARDS.match(line):
                    if start_idx is not None:
                        yield lines[start_idx:i]
                    start_idx = i  # Mark the start of the new block

            # Yield the very last block if the loop finishes normally
            if start_idx is not None:
                yield lines[start_idx:]

        blocks = []
        for block_lines in get_message_blocks_ranges(text.splitlines()):
            # A valid block is at minimum `header`, `<blank>`, `<subject>`
            # -- three lines. Noted blocks (`# squash!`, `# amend!`,
            # `# fixup!`) are longer; we detect the note at index 2 if
            # enough lines are present.
            if len(block_lines) < 3:
                raise EditorRecoverableFailure(
                    f'Unexpected block format: {block_lines}')

            # `will be skipped` blocks aren't relevant for commit
            # messages -- git already commented out their content.
            # Example:
            #   The commit message #4 will be skipped:
            #
            #   fixup! [cr149][brockit] `@latest-{channel}` latest
            #          version for all platforms
            #
            if "will be skipped:" in block_lines[0]:
                continue

            # The commit note line only appears next to the header line,
            # with an empty line in between. For example:
            # This is the commit message #3:
            #
            # amend! [cr149][brockit] `@latest-{channel}` latest version for all
            note = MsgBlock.OriginalCommitNote.from_line(block_lines[2])

            if note:
                # The note pushes the beginning of the message a few
                # lines down.
                message_lines = block_lines[4:]
            else:
                # With no note, the message starts immediately after the
                # header and empty line.
                message_lines = block_lines[2:]

            msg = "\n".join(line.strip() for line in message_lines).strip()
            if (note is not None and note.command == "squash!" and msg
                    and blocks):
                if not blocks[-1].full_message:
                    raise EditorRecoverableFailure(
                        'Unexpected empty message in previous block when '
                        'processing a squash! block with a note. Previous '
                        f'block lines: {blocks[-1].full_message}')
                # For `# squash!`, append to the previous block's body
                # and discard this block.
                blocks[-1]._extend_message(msg)
            elif msg:
                blocks.append(MsgBlock(full_message=msg, note=note))
            elif note is not None and note.command == "fixup!":
                # A bare `# fixup!` block carries no message of its own
                # -- git is just echoing the marker for context (this
                # also happens with stacked `# fixup! fixup! …` markers).
                # Treat it the same as a `will be skipped:` block.
                continue
            else:
                raise EditorRecoverableFailure(
                    f'Unexpected empty message in block. Block lines: '
                    f'{block_lines}')

        return blocks


@dataclass
class MessageWriter:
    """Decides what to do with a `GIT_EDITOR` squash commit-message file.

    Construct one via the `MessageWriter.parse` factory: it reads the
    file, checks that the squash is recognisable (either a pinned
    subject or a `reassign!`), and stores the parsed `MsgBlock`s.

    Use `rewrite_with_last_message()` to write the surviving message back.

    Anything we can't classify raises `EditorRecoverableFailure` and is left
    for the user to edit.

    This class only supports PINNED and REASSIGNMENT message editing, because
    these are the changes we know what to do with. Changes of any other type
    arriving in this class are a sign of something went wrong, and will result
    in `EditorRecoverableFailure` being raised.
    """

    # The squash editor file we'll rewrite.
    todo_file: Path
    # Every commit-message block parsed out of `todo_file`, in source
    # order. The `reassign!` first block (in the reassignment case) is
    # retained here -- it's the *last* block that gets written back,
    # not the first.
    blocks: list[MsgBlock]

    # The set of first-block entry types we know how to rewrite. Other
    # classifications (RECYCLABLE, DEFAULT) are punted to the editor.
    _HANDLED_ENTRY_TYPES = frozenset(
        {EntryType.PINNED, EntryType.REASSIGNMENT})

    def rewrite_with_last_message(self) -> None:
        """Writes the trailing block's `full_message` (with a trailing
        newline) back to `todo_file`. Always valid:
        `MessageWriter.parse` only returns instances with at least one
        block whose first-block content classifies as PINNED or
        REASSIGNMENT."""
        self.todo_file.write_text(self.blocks[-1].full_message + '\n',
                                  encoding='utf-8',
                                  newline='')

    @staticmethod
    def parse(todo_file: Path) -> 'MessageWriter':
        """Reads `todo_file` and returns a `MessageWriter` provided the
        first commit-message block's subject classifies as one of
        `MessageWriter._HANDLED_ENTRY_TYPES` (currently `PINNED` and
        `REASSIGNMENT`).

        Any other first-block classification (RECYCLABLE, DEFAULT) and
        any file that fails to parse / yields no blocks raises
        `EditorRecoverableFailure`. The exception message names the
        rejected `EntryType` so the user sees, in the editor, why the
        rewrite was punted.
        """
        blocks = MsgBlock.parse(todo_file.read_bytes().decode('utf-8'))

        if not blocks:
            raise EditorRecoverableFailure(
                'No commit-message blocks were parsed from the editor file.')

        first_msg_line = blocks[0].full_message.splitlines()[0]
        entry_type = get_entry_type_for_subject(first_msg_line)
        if entry_type not in MessageWriter._HANDLED_ENTRY_TYPES:
            raise EditorRecoverableFailure(
                f'Cannot rewrite a squash commit message classified as '
                f'{entry_type.value!r}. First content line: '
                f'{first_msg_line!r}')
        return MessageWriter(todo_file=todo_file, blocks=blocks)
