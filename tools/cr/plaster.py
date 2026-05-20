#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import hashlib
import logging
from pathlib import Path, PurePath
import json
import os
import re
import subprocess
import sys
import tomllib

from terminal import IncendiaryErrorHandler, console, is_verbose, terminal
import repository

# The path to the directory containing plaster files in brave-core.
PLASTER_FILES_PATH = repository.brave.root / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = repository.brave.root / 'patches'

# ============================================================================
# Experimental substitution back-ends
#
# `[[ast-substitution]]` is routed to ast-grep; `[[comby-substitution]]` is
# routed to comby. The substitution methods themselves live on `PlasterFile`
# (`_apply_ast_substitutions`, `_apply_comby_substitutions`); this section
# holds the flags, binary paths, and pure helpers they depend on.
#
# To turn either back-end off without removing the code, flip its
# `*_SUBSTITUTION_ENABLED` flag below. A plaster file that contains the
# corresponding TOML table while the flag is False errors out rather than
# being silently ignored, so the "I disabled it and forgot why my plaster
# stopped working" case is loud.
# ============================================================================

AST_SUBSTITUTION_ENABLED = True
COMBY_SUBSTITUTION_ENABLED = True

# Rewriter binaries are deployed by:
#   tools/cr/ast_grep/build_ast_grep.py  -> third_party/ast-grep-toolchain/
#   tools/cr/comby/build_comby.py        -> third_party/comby-toolchain/
#
# Anchored to `__file__` rather than `repository.brave.root` so the paths
# survive cwd changes: tests use `FakeChromiumRepo` which chdirs into a
# temp fake-brave directory before invoking `PlasterFile.apply()`. A
# cwd-relative anchor would then resolve against the temp dir, miss the
# real binary, and trip the binary-missing branch even when ast-grep /
# comby are actually installed.
_BRAVE_ROOT: Path = Path(__file__).resolve().parents[2]
AST_GREP_BIN: Path = (_BRAVE_ROOT / 'third_party' / 'ast-grep-toolchain' /
                      'bin' / 'ast-grep')
COMBY_BIN: Path = (_BRAVE_ROOT / 'third_party' / 'comby-toolchain' / 'bin' /
                   'comby')


def _infer_ast_grep_language(source: Path) -> str | None:
    """Map a source path to an ast-grep `--lang` value.

    Returns None for extensions we have no mapping for; the caller
    surfaces that as a plaster error rather than silently picking a
    default that would mis-parse the file.
    """
    return {
        '.h': 'cpp',
        '.hpp': 'cpp',
        '.cc': 'cpp',
        '.cpp': 'cpp',
        '.cxx': 'cpp',
        '.c': 'c',
        '.py': 'python',
        '.js': 'javascript',
        '.jsx': 'jsx',
        '.ts': 'typescript',
        '.tsx': 'tsx',
        '.go': 'go',
        '.rs': 'rust',
        '.java': 'java',
    }.get(source.suffix.lower())


def _infer_comby_matcher(source: Path) -> str:
    """Map a source path to a comby matcher token.

    Falls back to `.generic` for unknown extensions: comby's generic
    matcher still understands balanced delimiters but does not know
    about language-specific comment / string syntax, which is good
    enough for many one-off substitutions. `.gn` / `.gni` deliberately
    fall through to `.generic` for now -- comby has no GN matcher and
    `.python` is close but not exact; patterns there need to be
    conservative enough not to need comment / string awareness.
    """
    return {
        '.h': '.cpp',
        '.hpp': '.cpp',
        '.cc': '.cpp',
        '.cpp': '.cpp',
        '.cxx': '.cpp',
        '.c': '.c',
        '.py': '.py',
        '.js': '.js',
        '.ts': '.ts',
        '.go': '.go',
        '.rs': '.rs',
        '.java': '.java',
    }.get(source.suffix.lower(), '.generic')


def _format_subprocess_error(e: subprocess.CalledProcessError) -> str:
    """Compact stderr-or-stringified-exception for plaster's error list."""
    if e.stderr:
        return e.stderr.strip()
    return str(e)

@dataclass
class PathChecksumPair:
    """ A path/checksum pair to check for changes in a file.

    This class is used to store the path to a file and its checksum, which
    provides a way to track changes in the file content.
    """

    # The file path relative to the brave-core root.
    path: Path

    # Cached checksum of the file content. It will be set to None if the file
    # does not exist.
    checksum: str | None = field(init=False)

    def __post_init__(self):
        """Initialize the PathChecksumPair and calculate the checksum."""
        self.checksum = self.calculate_file_checksum()

    def calculate_file_checksum(self) -> str | None:
        """Calculate the SHA-256 checksum of the file's current content."""
        if not self.path.exists():
            return None
        checksum_generator = hashlib.sha256()
        with self.path.open('r', encoding='utf-8') as file:
            for chunk in iter(lambda: file.read(4096), ""):
                checksum_generator.update(chunk.encode())
        return checksum_generator.hexdigest()

    def save_if_changed(self, new_content: str, dry_run: bool = False) -> bool:
        """Save new content to the file only if its checksum differs.

        Args:
            new_content:
              The new content to write to the file.
            dry_run:
              If True, the actual write will not be performed, only a check
              for changes will be made.
        Returns:
            bool:
              If the contents of the file are considered up-to-date, returns
              False. Otherwise, returns True, applying file changes if not doing
              a dry run.
        """
        new_checksum = hashlib.sha256(new_content.encode()).hexdigest()
        if self.checksum == new_checksum:
            return False  # No change detected
        logging.debug('Saving: %s', self.path)
        if not dry_run:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            # On Windows we checkout files in Linux mode, so we should make
            # sure not to use Windows newlines here.
            self.path.write_text(new_content, encoding='utf-8', newline='\n')
            assert (new_checksum == self.calculate_file_checksum())
        self.checksum = new_checksum
        return True


@dataclass(frozen=True)
class Patchinfo:
    """In-memory representation of a parsed .patchinfo file.
    """

    @dataclass(frozen=True)
    class Entry:
        """A single path/checksum pair from a .patchinfo file."""

        # Path of the file the entry refers to. Stored as a string because
        # that is how it is serialized in JSON.
        path: str

        # SHA-256 checksum (hex) of the file at `path`.
        checksum: str

    # Schema version of the patchinfo file format.
    schema_version: int

    # SHA-256 checksum (hex) of the .patch file.
    patch_checksum: str

    # The file the patch applies to, paired with its post-patch checksum.
    # JSON stores this under "appliesTo" as an array, but in practice we only
    # ever produce and accept a single entry.
    applies_to: Entry

    # The plaster .toml file that produced this patchinfo.
    plaster: Entry

    @staticmethod
    def from_json(content: str) -> Patchinfo | None:
        """Parse the JSON content of a .patchinfo file.

        Returns None if the content is not valid JSON, if any required
        path/checksum pair is missing, if any field has the wrong type, or
        if "appliesTo" does not contain exactly one entry.
        """
        try:
            data = json.loads(content)
        except json.JSONDecodeError:
            return None
        if not isinstance(data, dict):
            return None

        schema_version = data.get('schemaVersion')
        if not isinstance(schema_version, int):
            return None
        patch_checksum = data.get('patchChecksum')
        if not isinstance(patch_checksum, str):
            return None

        applies_to_raw = data.get('appliesTo')
        if not isinstance(applies_to_raw, list) or len(applies_to_raw) != 1:
            return None
        entry = applies_to_raw[0]
        if not isinstance(entry, dict):
            return None
        applies_path = entry.get('path')
        applies_checksum = entry.get('checksum')
        if (not isinstance(applies_path, str)
                or not isinstance(applies_checksum, str)):
            return None

        plaster_raw = data.get('plaster')
        if not isinstance(plaster_raw, dict):
            return None
        plaster_path = plaster_raw.get('path')
        plaster_checksum = plaster_raw.get('checksum')
        if (not isinstance(plaster_path, str)
                or not isinstance(plaster_checksum, str)):
            return None

        return Patchinfo(
            schema_version=schema_version,
            patch_checksum=patch_checksum,
            applies_to=Patchinfo.Entry(path=applies_path,
                                       checksum=applies_checksum),
            plaster=Patchinfo.Entry(path=plaster_path,
                                    checksum=plaster_checksum),
        )

    def to_json(self) -> str:
        """Serialize this Patchinfo to the .patchinfo JSON representation."""
        return json.dumps({
            'schemaVersion': self.schema_version,
            'patchChecksum': self.patch_checksum,
            'appliesTo': [{
                'path': self.applies_to.path,
                'checksum': self.applies_to.checksum,
            }],
            'plaster': {
                'path': self.plaster.path,
                'checksum': self.plaster.checksum,
            },
        })


@dataclass
class PatchinfoBuilder:
    """ Builds a .patchinfo file's contents for a given plaster.

    A builder for the patchfile metadata, known as .patchinfo, which is used
    by our machinery when applying patches to determine if a file needs to be
    updated or not.

    patchinfo files are not committted to the repository. These files have the
    same name as the patch file, but with a .patchinfo extension. They usually
    look like this:
    {
      "schemaVersion": 1,
      "patchChecksum": "78cb50920870416befe307fa4272bb6076e5000ba25dfbcac2cf00",
      "appliesTo": [
        {
          "path": "browser/autocomplete_classifier_factory.cc",
          "checksum": "da89e921d83a9708e2f7490ba72c5c2cd05d22541d8e266611bcd34"
        }
      ]
    }

    This builder extends the contents of patchinfo files to include the
    details of the plaster file, including path and checksum, under the
    "plaster" key.
    {
      "schemaVersion": 1,
      "patchChecksum": "78cb50920870416befe307fa4272bb6076e5000ba25dfbcac2cf00",
      "appliesTo": [
        {
          "path": "browser/autocomplete_classifier_factory.cc",
          "checksum": "da89e921d83a9708e2f7490ba72c5c2cd05d22541d8e266611bcd345"
        }
      ],
      "plaster": {
        "path": "rewrite/browser/autocomplete_classifier_factory.cc.toml",
        "checksum": "a11a8eed2bec0083f7c9be762fced0f6db372a47d40c20b93037db0473"
      }
    }

    It produces a patchinfo file and, with its hash data, decides whether
    changes need to be persisted to the source, patch, and patchinfo files.
    These three files are supposed to be changed only when the resulting new
    content is different from what is in disk.

    The new additions to .patchinfo are not known yet to `apply_patches`, but
    this may change in the future.
    """

    # Path to the plaster file, relative to the brave-core root.
    plaster_file: Path

    # Contents of the plaster file as a string (read during initialization).
    plaster_contents: str = field(init=False)

    # SHA-256 checksum of the plaster file contents.
    plaster_checksum: str | None = field(init=False)

    # The relative path to the source file that the plaster file applies to.
    # This field is kept separate to allow the use in git commands to the
    # repository.
    source: Path = field(init=False)

    # PathChecksumPair object representing the target source file.
    source_with_checksum: PathChecksumPair = field(init=False)

    # PathChecksumPair object representing the patch file.
    patch: PathChecksumPair = field(init=False)

    # PathChecksumPair object representing the patchinfo metadata file.
    patchinfo: PathChecksumPair = field(init=False)

    def __post_init__(self):
        """Initializes the PatchinfoBuilder data with checksums and paths."""
        self.plaster_contents = self.plaster_file.read_bytes().decode('utf-8')
        self.plaster_checksum = hashlib.sha256(
            self.plaster_contents.encode()).hexdigest()

        # Setup source file (path derived from plaster file, no extension).
        self.source = self.plaster_file.relative_to(
            PLASTER_FILES_PATH).with_suffix('')
        self.source_with_checksum = PathChecksumPair(
            Path(repository.chromium.from_brave(self.source)))

        # Setup patch file (named based on source path, with dashes).
        self.patch = PathChecksumPair(
            PATCHES_PATH /
            f'{str(PurePath(self.source).as_posix()).replace("/", "-")}.patch')

        # Setup patchinfo metadata file (same as the patch .patchinfo
        # extension).
        self.patchinfo = PathChecksumPair(
            self.patch.path.with_suffix('.patchinfo'))

        # This is set relative, so it gets validated to be under the
        # brave-core root.
        self.plaster_file = self.plaster_file.relative_to(
            repository.brave.root)

    def save_source_if_changed(self,
                               content: str,
                               dry_run: bool = False) -> bool:
        """Save new content to the source file only if its checksum has changed.

        Args:
            content:
              The new content to write to the source file.
            dry_run:
              Indicates whether the actual write should not be performed.
        """
        return self.source_with_checksum.save_if_changed(new_content=content,
                                                         dry_run=dry_run)

    def save_patch_if_changed(self, dry_run: bool = False) -> bool:
        """Save the generated git diff to the patch file only if it has changed.

        Args:
            dry_run:
              Indicates whether the actual write should not be performed.
        """
        content = repository.chromium.run_git('diff',
                                              '--src-prefix=a/',
                                              '--dst-prefix=b/',
                                              '--default-prefix',
                                              '--full-index',
                                              '--ignore-space-at-eol',
                                              self.source,
                                              no_trim=True)
        return self.patch.save_if_changed(new_content=content, dry_run=dry_run)

    def save_patchinfo_if_changed(self):
        """Save the patchinfo metadata JSON only if it has changed."""
        content = Patchinfo(
            schema_version=1,
            patch_checksum=self.patch.checksum,
            applies_to=Patchinfo.Entry(
                path=str(self.source),
                checksum=self.source_with_checksum.checksum,
            ),
            plaster=Patchinfo.Entry(
                path=str(self.plaster_file),
                checksum=self.plaster_checksum,
            ),
        ).to_json()
        self.patchinfo.save_if_changed(content)


@dataclass
class PlasterFile:
    """ Class representing an plaster file.
    This class is used to apply plaster files to sources in other repositories.
    """

    # The path to the plaster file. This path is relative to the brave-core
    # root.
    path: Path

    @classmethod
    def find_all(cls) -> list[PlasterFile]:
        """ Finds all plaster files in the rewrite directory.
        Returns:
            A list of PlasterFile objects.
        """
        plaster_files = []

        for root, _, files in os.walk(PLASTER_FILES_PATH):
            for file in files:
                if file.endswith('.toml'):
                    plaster_files.append(cls(Path(root) / file))

        return plaster_files

    def needs_apply(self) -> bool:
        """Returns True if this plaster file needs to be re-applied.

        The basics of this function is that it checks if a given plaster file
        or its source last change occurred after the last change for
        `.pathcinfo`. If that's the case, then it means something changed in
        these files, since the last time the patch was applied with our
        tooling, which warrent checksum checks for all of them, and at that
        point if any of the checksum values don't match, we do return True.
        """
        source_relative = self.path.relative_to(
            PLASTER_FILES_PATH).with_suffix('')
        source_path = Path(repository.chromium.from_brave(source_relative))
        patch_stem = source_relative.as_posix().replace('/', '-')
        patch_path = PATCHES_PATH / f'{patch_stem}.patch'
        patchinfo_path = PATCHES_PATH / f'{patch_stem}.patchinfo'

        # Only the plaster toml is guaranteed to exist here; any of the
        # other files may be missing and that is by itself a reason to
        # re-apply.
        if (not patchinfo_path.exists() or not patch_path.exists()
                or not source_path.exists()):
            return True

        patchinfo_mtime = patchinfo_path.stat().st_mtime
        plaster_mtime = self.path.stat().st_mtime
        patch_mtime = patch_path.stat().st_mtime
        source_mtime = source_path.stat().st_mtime

        if (patchinfo_mtime >= plaster_mtime
                and patchinfo_mtime >= source_mtime
                and patchinfo_mtime >= patch_mtime):
            return False

        try:
            content = patchinfo_path.read_bytes().decode('utf-8')
        except OSError:
            return True

        info = Patchinfo.from_json(content)
        if info is None:
            return True

        return (
            PathChecksumPair(source_path).checksum != info.applies_to.checksum
            or PathChecksumPair(self.path).checksum != info.plaster.checksum
            or PathChecksumPair(patch_path).checksum != info.patch_checksum)

    def apply(self, dry_run=False):
        info = PatchinfoBuilder(self.path)
        plaster_file = tomllib.loads(info.plaster_contents)
        contents = repository.chromium.read_file(info.source)
        errors = []

        # Experimental structural-rewrite passes run before the regex
        # path so they can "make the source pliable" first (e.g. drop
        # `final`, virtualize a destructor) and the regex pass can then
        # do textual finalisation on top. To disable either back-end
        # without removing it, flip its module-level flag below; a
        # disabled table that still appears in a plaster file is a hard
        # error rather than a silent skip.
        if AST_SUBSTITUTION_ENABLED:
            contents = self._apply_ast_substitutions(contents, plaster_file,
                                                     info.source, errors)
        elif plaster_file.get('ast-substitution'):
            errors.append(
                f'[[ast-substitution]] is present in {self.path} but '
                f'AST_SUBSTITUTION_ENABLED is False in plaster.py')

        if COMBY_SUBSTITUTION_ENABLED:
            contents = self._apply_comby_substitutions(contents, plaster_file,
                                                       info.source, errors)
        elif plaster_file.get('comby-substitution'):
            errors.append(
                f'[[comby-substitution]] is present in {self.path} but '
                f'COMBY_SUBSTITUTION_ENABLED is False in plaster.py')

        try:
            # `or []` tolerates plaster files that use only ast / comby
            # substitutions and have no `[[substitution]]` entries.
            for substitution in plaster_file.get('substitution') or []:
                description = substitution.get('description')
                re_pattern = substitution.get('re_pattern')
                pattern = substitution.get('pattern')
                replace = substitution.get('replace')
                expected_count = substitution.get('count', 1)
                flags = substitution.get('re_flags', [])

                if description is None:
                    raise ValueError(
                        f'No description specified in {info.source}')

                if re_pattern is not None and pattern is not None:
                    raise ValueError(
                        f'Please specify either pattern or re_pattern '
                        f' in {info.source}')

                if re_pattern is None:
                    if pattern is None:
                        raise ValueError(
                            f'No pattern specified in {info.source}')
                    re_pattern = re.escape(pattern)

                if replace is None:
                    raise ValueError(
                        f'No replace value specified in {info.source}')

                re_flags = 0
                for flag in flags:
                    # Only accept valid re flags
                    if flag.isupper() and hasattr(re, flag):
                        re_flags |= getattr(re, flag)
                    else:
                        raise ValueError(
                            f'Invalid re flag specified: {flag} in '
                            f'{info.source}')

                contents, num_changes = re.subn(
                    re_pattern,
                    replace,
                    contents,
                    flags=re_flags,
                    # We dont't want to explicitly limit the number of matches
                    # here, we want to control what matches using the match
                    # pattern and then ensure the output matches only what we
                    # expected
                    count=0)

                # count == 0 means "replace all matches" and bypass count
                # validation
                if expected_count not in (0, num_changes):
                    errors.append(
                        f'Unexpected number of matches ({num_changes} vs '
                        f'{expected_count}) in {self.path}')

        except re.error as e:
            errors.append(f'Invalid regex: {e} in {self.path}')

        if errors:
            raise PlasterApplyError(errors)

        has_changed = info.save_source_if_changed(contents, dry_run=dry_run)
        has_changed = info.save_patch_if_changed(
            dry_run=dry_run) or has_changed
        if dry_run:
            if has_changed:
                raise PlasterFileNeedsRegen(
                    f"Plaster file needs to be reapplied: {self.path}")
        else:
            info.save_patchinfo_if_changed()

    def _apply_ast_substitutions(self, contents: str, plaster_data: dict,
                                 source: Path, errors: list[str]) -> str:
        """Apply every `[[ast-substitution]]` entry, returning the new content.

        Each entry triggers two ast-grep invocations: one with
        `--json=compact` to count matches (so the `count` assertion is
        symmetric with the regex code path), one with `--rewrite` to
        apply the substitution. Errors are appended to `errors`; an
        error in one entry does not stop later entries from being
        attempted.

        No-op (returns `contents` unchanged) when the plaster file has
        no `[[ast-substitution]]` entries.
        """
        entries = plaster_data.get('ast-substitution') or []
        if not entries:
            return contents
        if not AST_GREP_BIN.is_file():
            errors.append(
                f'[[ast-substitution]] entries in {self.path} but ast-grep '
                f'is not installed at {AST_GREP_BIN}. See '
                f'brave/tools/cr/ast_grep/README.md for build instructions.')
            return contents

        language = _infer_ast_grep_language(source)
        if language is None:
            errors.append(
                f'[[ast-substitution]] does not know how to handle the file '
                f'extension {source.suffix} ({self.path})')
            return contents

        for entry in entries:
            description = entry.get('description')
            pattern = entry.get('pattern')
            replace = entry.get('replace')
            expected_count = entry.get('count', 1)

            if description is None:
                errors.append(
                    f'[[ast-substitution]] missing description in {self.path}')
                continue
            if pattern is None or replace is None:
                errors.append(
                    f'[[ast-substitution]] "{description}" needs both '
                    f'pattern and replace ({self.path})')
                continue

            # Count pass.
            find_cmd = [
                str(AST_GREP_BIN), 'run', '--pattern', pattern, '--lang',
                language, '--json=compact', '--stdin'
            ]
            try:
                find_out = terminal.run(find_cmd, stdin=contents).stdout
            except subprocess.CalledProcessError as e:
                # ast-grep exits 1 (with `[]` on stdout) when the
                # pattern parses fine but matches nothing. Treat that
                # as a legitimate "zero matches" answer so the count
                # assertion below can report the mismatch.
                stdout = e.stdout or ''
                if e.returncode == 1 and stdout.strip() in ('[]', ''):
                    find_out = stdout
                else:
                    errors.append(
                        f'ast-grep find failed for "{description}" in '
                        f'{self.path}: {_format_subprocess_error(e)}')
                    continue
            try:
                matches = json.loads(find_out) if find_out.strip() else []
            except json.JSONDecodeError:
                matches = []
            num_changes = len(matches) if isinstance(matches, list) else 0

            # count == 0 means "any count is acceptable", matching the
            # regex path's semantics.
            if expected_count not in (0, num_changes):
                errors.append(
                    f'Unexpected number of ast-grep matches ({num_changes} '
                    f'vs {expected_count}) for "{description}" in '
                    f'{self.path}')
                continue

            # Rewrite pass. `--update-all` is required to make ast-grep
            # emit the rewritten content on stdout; without it `--stdin`
            # produces a colorised diff preview instead. The "Applied N
            # changes" log line goes to stderr, so stdout stays clean.
            rewrite_cmd = [
                str(AST_GREP_BIN), 'run', '--pattern', pattern, '--rewrite',
                replace, '--lang', language, '--stdin', '--update-all'
            ]
            try:
                new_contents = terminal.run(rewrite_cmd, stdin=contents).stdout
            except subprocess.CalledProcessError as e:
                errors.append(
                    f'ast-grep rewrite failed for "{description}" in '
                    f'{self.path}: {_format_subprocess_error(e)}')
                continue
            # `--update-all --stdin` appends an extra trailing newline
            # to the output. Re-anchor the trailing newline count to
            # the input so the generated patch doesn't pick up the
            # artefact.
            trailing = '\n' * (len(contents) - len(contents.rstrip('\n')))
            contents = new_contents.rstrip('\n') + trailing

        return contents

    def _apply_comby_substitutions(self, contents: str, plaster_data: dict,
                                   source: Path, errors: list[str]) -> str:
        """Apply every `[[comby-substitution]]` entry, returning the new
        content.

        Mirrors `_apply_ast_substitutions`: a `-match-only -json-lines`
        pass counts matches (one JSON object per match), then a rewrite
        pass actually substitutes. The matcher token is inferred from
        the source file extension; an explicit `matcher` field on the
        entry overrides that.

        No-op when the plaster file has no `[[comby-substitution]]`
        entries.
        """
        entries = plaster_data.get('comby-substitution') or []
        if not entries:
            return contents
        if not COMBY_BIN.is_file():
            errors.append(
                f'[[comby-substitution]] entries in {self.path} but comby '
                f'is not installed at {COMBY_BIN}. See '
                f'brave/tools/cr/comby/README.md for build instructions.')
            return contents

        default_matcher = _infer_comby_matcher(source)

        for entry in entries:
            description = entry.get('description')
            pattern = entry.get('pattern')
            replace = entry.get('replace')
            expected_count = entry.get('count', 1)
            matcher = entry.get('matcher', default_matcher)

            if description is None:
                errors.append(f'[[comby-substitution]] missing description in '
                              f'{self.path}')
                continue
            if pattern is None or replace is None:
                errors.append(
                    f'[[comby-substitution]] "{description}" needs both '
                    f'pattern and replace ({self.path})')
                continue

            # Count pass: `-json-lines` emits one JSON object *per
            # file* (not per match), with the matches collected in a
            # `matches` array. For stdin input that's a single line, but
            # we handle multi-line output anyway so the count remains
            # correct if comby's output shape evolves.
            find_cmd = [
                str(COMBY_BIN), pattern, replace, matcher, '-stdin', '-stdout',
                '-match-only', '-json-lines'
            ]
            try:
                find_out = terminal.run(find_cmd, stdin=contents).stdout
            except subprocess.CalledProcessError as e:
                errors.append(f'comby find failed for "{description}" in '
                              f'{self.path}: {_format_subprocess_error(e)}')
                continue
            num_changes = 0
            for line in find_out.splitlines():
                if not line.strip():
                    continue
                try:
                    num_changes += len(json.loads(line).get('matches', []))
                except json.JSONDecodeError:
                    pass

            if expected_count not in (0, num_changes):
                errors.append(
                    f'Unexpected number of comby matches ({num_changes} vs '
                    f'{expected_count}) for "{description}" in {self.path}')
                continue

            # Rewrite pass.
            rewrite_cmd = [
                str(COMBY_BIN), pattern, replace, matcher, '-stdin', '-stdout'
            ]
            try:
                contents = terminal.run(rewrite_cmd, stdin=contents).stdout
            except subprocess.CalledProcessError as e:
                errors.append(f'comby rewrite failed for "{description}" in '
                              f'{self.path}: {_format_subprocess_error(e)}')
                continue

        return contents


class PlasterError(Exception):
    """Base class for errors reported by the plaster tool."""


class PlasterFileNeedsRegen(PlasterError):
    pass


class PlasterApplyError(PlasterError):
    """Raised when applying a plaster file produces substitution errors."""

    def __init__(self, errors: list[str]):
        self.errors = errors
        super().__init__(
            'There were errors attempting to apply the patches:\n' +
            '\n'.join(errors))


def get_plaster_files(filepaths: list[str] | None = None) -> list[PlasterFile]:
    """Returns plaster files matching the provided file paths.

    If no file paths are provided, all plaster files are returned.
    """
    if not filepaths:
        return PlasterFile.find_all()

    expected_plaster_files = set()
    for filepath in filepaths:
        filepath = PurePath(filepath).as_posix()
        if filepath.startswith('patches/') and filepath.endswith('.patch'):
            base = filepath[len('patches/'):-len('.patch')]
            plaster_relative = base.replace('-', '/') + '.toml'
            plaster_path = f'rewrite/{plaster_relative}'
            expected_plaster_files.add(plaster_path)
        elif filepath.startswith('rewrite/') and filepath.endswith('.toml'):
            expected_plaster_files.add(filepath)
        else:
            raise ValueError(f'Unexpected file path: {filepath}')

    plaster_parent = Path(PLASTER_FILES_PATH).parent

    # A set of candidate plaster files.
    candidate_paths = sorted(plaster_parent / Path(path)
                             for path in expected_plaster_files)

    # TODO(https://github.com/brave/brave-browser/issues/46880): For now we
    # discard any plaster file passed in that does not exist. At some point
    # we will need a good answer of what to do once a plaster file is
    # deleted, but for now no check is being implemented to not break
    # git revert on presubmit checks.
    return [PlasterFile(path) for path in candidate_paths if path.exists()]


def apply(args):
    """Applies plaster files to brave.

    This function default mode, meaning no arguments were provided when calling
    `plaster apply` will cause a `needs_apply` check to be used before applying
    a plaster, which is a way to make calling plaster inexpensive.

    When `--all` is passed, this causes all plaster files to be reapplied. When
    specific plaster file paths or patch file paths are passed, only those are
    run.
    """
    filepaths = getattr(args, 'filepaths', None)
    apply_all = getattr(args, 'all', False)
    skip_up_to_date = not apply_all and not filepaths

    with terminal.with_status('Applying plaster files'):
        plaster_files = get_plaster_files(filepaths)
        for plaster_file in plaster_files:
            if skip_up_to_date and not plaster_file.needs_apply():
                logging.debug('Up-to-date, skipping: %s', plaster_file.path)
                continue
            console.log(f'Applying plaster file: {plaster_file.path}')
            plaster_file.apply()
    return 0


def check(args):
    """Checks whether plaster files need to be reapplied.
    """
    plaster_files = get_plaster_files(getattr(args, 'filepaths', None))
    for plaster_file in plaster_files:
        logging.debug('Checking plaster file: %s', plaster_file.path)
        plaster_file.apply(dry_run=True)
    return 0


def main():
    parser = argparse.ArgumentParser(
        description=
        '🩹 A tool to generate patches into Chromium using plaster files.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging')
    parser.add_argument('--infra-mode',
                        action='store_true',
                        help='Enable infra mode')
    subparsers = parser.add_subparsers(dest='command', required=True)

    # Add the 'apply' subparser
    apply_parser = subparsers.add_parser(
        'apply', help='Apply all plaster files to the sources in brave-core')
    apply_parser.add_argument(
        '--all',
        action='store_true',
        help='Re-apply every plaster file unconditionally, even if the '
        'patchinfo indicates it is already up-to-date.')
    apply_parser.add_argument('filepaths',
                              nargs='*',
                              help='Filepaths to apply')
    apply_parser.set_defaults(func=apply)

    # Add the 'check' subparser
    check_parser = subparsers.add_parser(
        'check', help='Check that plaster files are applied to sources.')
    check_parser.add_argument('filepaths',
                              nargs='*',
                              help='Filepaths to check')
    check_parser.set_defaults(func=check)

    args = parser.parse_args()

    if hasattr(args, 'infra_mode') and args.infra_mode:
        terminal.set_infra_mode()

    logging.basicConfig(level=logging.DEBUG if is_verbose() else logging.INFO,
                        format='%(message)s',
                        handlers=[IncendiaryErrorHandler()],
                        force=True)

    try:
        return args.func(args)
    except PlasterError as e:
        print(e, file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
