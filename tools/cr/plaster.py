#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import abc
import argparse
import ast
from dataclasses import dataclass, field
import hashlib
import logging
import mmap
from pathlib import Path, PurePath
import json
import os
import platform
import re
import string
import subprocess
import sys
import textwrap
from types import MappingProxyType
from typing import ClassVar, Final

from rich.markdown import Markdown
import yaml

from terminal import IncendiaryErrorHandler, console, is_verbose, terminal
import repository

# A round-about import for https://github.com/keleshev/schema, vendored under
# depot_tools. It could be helpful to deploy this under our own third_party in
# the future.
sys.path.insert(
    0,
    str(
        Path(__file__).resolve().parents[3] / 'third_party' / 'depot_tools' /
        'third_party'))
import schema  # pylint: disable=wrong-import-position,import-error

# The path to the directory containing plaster files in brave-core.
PLASTER_FILES_PATH = repository.brave.root / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = repository.brave.root / 'patches'

# The plaster file extension.
PLASTER_EXTENSION = '.yaml'

# A particular gitattributes file that is used to ensure we get deterministic
# patch output across platforms and git versions.
PLASTER_GITATTRIBUTES_PATH = Path(__file__).parent / 'plaster_gitattributes'

# The declarative ast-grep rewriters spec, loaded and validated by
# `RewritersEval`.
REWRITERS_FILE = Path(__file__).parent / 'rewriters.pyl'


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
        """Calculate the SHA-256 checksum of the file's raw bytes.
        """
        if not self.path.exists():
            return None
        checksum_generator = hashlib.sha256()
        with self.path.open('rb') as file:
            if os.fstat(file.fileno()).st_size == 0:
                return checksum_generator.hexdigest()
            with mmap.mmap(file.fileno(), 0,
                           access=mmap.ACCESS_READ) as mapped:
                checksum_generator.update(mapped)
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

    # The plaster file that produced this patchinfo (`.yaml`).
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
        "path": "rewrite/browser/autocomplete_classifier_factory.cc.yaml",
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
        # The hunk-header function context (the text git appends after
        # `@@ ... @@`) is computed by the userdiff driver git selects for the
        # source. That selection depends on the ambient environment (a
        # per-user/global `core.attributesFile`, the system gitattributes, and
        # git version built-ins), so the same source can yield different patch
        # bytes on different machines even though the change is identical. This
        # git diff peculiarity was causing `.mm` files to produce different
        # patch files depending in which platform plaster was being run.
        #
        # We pin the attibutes when creating a diff (`plaster_gitattributes`),
        # and ignore the system gitattributes (`GIT_ATTR_NOSYSTEM`), so the
        # output is deterministic across platforms and git versions.
        content = repository.chromium.run_git(
            '-c',
            f'core.attributesFile={PLASTER_GITATTRIBUTES_PATH}',
            'diff',
            '--src-prefix=a/',
            '--dst-prefix=b/',
            '--default-prefix',
            '--full-index',
            '--ignore-space-at-eol',
            self.source,
            no_trim=True,
            env={
                **os.environ, 'GIT_ATTR_NOSYSTEM': '1'
            })
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


class Rewriter(abc.ABC):
    """A single transformation applied to a plaster's target file.

    Each rewriter knows how to parse and validate them from its YAML body via
    `parse`.
    """

    # Static, per-subclass metadata. Declared `ClassVar` so they are shared by
    # the class (never per-instance); each concrete rewriter overrides them
    # with `Final` constants (see `Regex`).

    # The `<name>:` key that selects this rewriter in a `substitutions:` entry.
    NAME: ClassVar[str] = ''

    # A rewriter-specific override for the default value of `count:`.
    DEFAULT_COUNT: ClassVar[int | None] = None

    # One-line summary shown in the `Rewriters` help index.
    SUMMARY: ClassVar[str] = ''

    # Full Markdown help shown by `plaster --help <NAME>`. Falls back to the
    # docstring.
    HELP: ClassVar[str] = ''

    @abc.abstractmethod
    def apply(self,
              contents: str,
              *,
              count: int,
              description: str,
              blank_for_parse: bool = False) -> tuple[str, list[str]]:
        """Transform `contents`, returning (new_contents, validation_errors).

        `count` is the entry's `count:` (default 1, `0` meaning "one or more").
        Each rewriter decides how it applies. For a single-site rewriter it is
        the expected number of matches. A composed rewriter may instead give
        each of its operations its own expectation. `errors` is the list of
        human-readable count/validation failures (empty when the entry applied
        as expected). `blank_for_parse` is the file-wide
        `blank_macros_for_ast_parsing` flag used by AST rewriters.
        """

    @classmethod
    @abc.abstractmethod
    def parse(cls, body: object, *, description: str) -> Rewriter:
        """Build a rewriter from its YAML body, validating its fields."""

    @classmethod
    def validate_count(cls, count: int, description: str) -> None:
        """Reject an unsupported `count:` for this rewriter.
        """

    @classmethod
    def help_text(cls) -> str:
        """The full, user-facing help for this rewriter as dedented Markdown."""
        return textwrap.dedent(cls.HELP or cls.__doc__ or '').strip('\n')

    def source_language(self) -> str | None:
        """The source language this rewriter requires, or None if agnostic.

        AST rewriters parse the target in a specific language (the `<lang>.`
        prefix of their op id), so they may only be used on sources of that
        language. Text rewriters are language-agnostic and return None.
        """
        return None


# The file-wide plaster keys allowed at the top level of a YAML plaster.
_TOP_LEVEL_KEYS: Final = frozenset(
    {'substitutions', 'blank_macros_for_ast_parsing'})

# Target-source suffixes plaster treats as C++. `blank_macros_for_ast_parsing`
# blanks constructs for the tree-sitter C++ parser, so it is only meaningful for
# these.
_CXX_SOURCE_SUFFIXES: Final = frozenset(
    {'.h', '.hpp', '.hxx', '.h++', '.cc', '.cpp', '.cxx', '.c++', '.mm'})


def _is_cxx_source(plaster_path: Path) -> bool:
    """True if the plaster's target source (its path minus `.yaml`) is C++."""
    return plaster_path.with_suffix('').suffix in _CXX_SOURCE_SUFFIXES


@dataclass(frozen=True)
class PlasterSpec:
    """A parsed plaster file: its substitutions plus its file-wide options."""

    # The `substitutions:` entries, in order.
    substitutions: list[Substitution]

    # Indicates to the AST-rewriter that it needs to blank all cxx preprocessor
    # directives that can interfere with parsing.
    blank_macros_for_ast_parsing: bool = False


@dataclass(frozen=True)
class Substitution:
    """One `substitutions:` entry: a shared envelope composing a `Rewriter`.

    The envelope carries `description` and `expected_count` and owns the YAML
    dispatch in `from_yaml`, which picks the `Rewriter` per item and hands it
    its body to parse. `apply` simply delegates to the composed rewriter.
    """

    # Human-readable label, used as a prefix in error messages.
    description: str

    # Expected number of changes; `0` means "one or more matches".
    expected_count: int

    # The rewriter this entry composes; `apply` delegates to it.
    rewriter: Rewriter

    def apply(self,
              contents: str,
              *,
              blank_for_parse: bool = False) -> tuple[str, list[str]]:
        """Apply the composed rewriter, returning (new_contents, errors).
        """
        return self.rewriter.apply(contents,
                                   count=self.expected_count,
                                   description=self.description,
                                   blank_for_parse=blank_for_parse)

    class _NoDupSafeLoader(yaml.SafeLoader):
        """`yaml.SafeLoader` that rejects duplicate mapping keys.

        `yaml.safe_load` silently keeps the last value for a duplicate
        key — which would let a typo'd substitution field shadow the
        real one and apply the wrong patch. We surface those cases as
        a `ValueError` instead.
        """

        @staticmethod
        def _construct_mapping_strict(loader: yaml.SafeLoader,
                                      node: yaml.MappingNode) -> dict:
            """Construct a dict from `node`, raising on duplicate keys."""
            loader.flatten_mapping(node)
            seen: set = set()
            duplicates: list = []
            for key_node, _ in node.value:
                key = loader.construct_object(key_node, deep=True)
                if key in seen:
                    duplicates.append(key)
                else:
                    seen.add(key)
            if duplicates:
                raise ValueError('Duplicate key(s) in YAML mapping: ' +
                                 ', '.join(repr(k) for k in duplicates))
            return loader.construct_mapping(node, deep=True)

        # Install the strict constructor.
        yaml_constructors = yaml.SafeLoader.yaml_constructors | {
            'tag:yaml.org,2002:map': _construct_mapping_strict,
        }

    @staticmethod
    def from_yaml(contents: str) -> PlasterSpec:
        """Parse a YAML plaster file into a `PlasterSpec`.

        YAML plasters use a top-level `substitutions:` list, plus file-level
        values.
        """
        data = yaml.load(contents, Loader=Substitution._NoDupSafeLoader)
        if data is None:
            data = {}
        if not isinstance(data, dict):
            raise ValueError('Plaster YAML must be a mapping at the top level')
        unknown = sorted(set(data) - _TOP_LEVEL_KEYS)
        if unknown:
            raise ValueError('Unrecognised top-level plaster key(s): ' +
                             ', '.join(repr(k)
                                       for k in unknown) + '; allowed: ' +
                             ', '.join(sorted(_TOP_LEVEL_KEYS)))
        blank_for_parse = data.get('blank_macros_for_ast_parsing', False)
        if not isinstance(blank_for_parse, bool):
            raise ValueError(
                '`blank_macros_for_ast_parsing` must be a boolean')
        raw = data.get('substitutions')
        if raw is None:
            raise ValueError(
                'Plaster YAML is missing required `substitutions:` key')
        if not isinstance(raw, list):
            raise ValueError('YAML `substitutions` must be a list')
        if not raw:
            raise ValueError(
                'Plaster YAML `substitutions:` must contain at least one entry'
            )
        return PlasterSpec(
            substitutions=[Substitution._from_item(entry) for entry in raw],
            blank_macros_for_ast_parsing=blank_for_parse)

    @staticmethod
    def _from_item(data: object) -> Substitution:
        """Validate one `substitutions:` entry and build its rewriter.

        An entry names a rewriter by carrying one of `_REWRITERS` as a key,
        alongside the item-level `description` and optional `count`.

        Raises:
            ValueError: on a malformed entry (missing/typo'd keys, a missing
                or unknown rewriter, or invalid rewriter-specific fields).
        """
        if not isinstance(data, dict):
            raise ValueError(f'substitution entry must be a mapping, got '
                             f'{type(data).__name__}')
        keys = set(data)

        description = data.get('description')
        if not isinstance(description, str):
            raise ValueError('No description specified for substitution entry')

        rewriter_keys = sorted(keys & _REWRITERS.keys())
        if len(rewriter_keys) > 1:
            raise ValueError(
                f'Only one rewriter allowed per entry, got '
                f'{", ".join(rewriter_keys)} (in "{description}")')

        if rewriter_keys:
            name = rewriter_keys[0]
            unknown = sorted(keys - {'description', 'count', name})
            if unknown:
                raise ValueError(f'Unrecognised key(s) for the "{name}" '
                                 f'rewriter: '
                                 f'{", ".join(repr(k) for k in unknown)} '
                                 f'(in "{description}")')
            # The chosen rewriter decides what an omitted `count:` means, and
            # may reject a count it does not support.
            expected_count = Substitution._parse_count(
                data, description, _REWRITERS[name].DEFAULT_COUNT)
            _REWRITERS[name].validate_count(expected_count, description)
            rewriter = _REWRITERS[name].parse(data[name],
                                              description=description)
            return Substitution(description=description,
                                expected_count=expected_count,
                                rewriter=rewriter)

        # Every entry must name a rewriter. Surface the most helpful error we
        # can: a mapping-valued stray key is almost certainly an attempt to use
        # a rewriter that is not registered -- point the user at the ones that
        # are; anything else is an unrecognised key.
        unknown = sorted(keys - {'description', 'count'})
        bad_rewriters = [k for k in unknown if isinstance(data[k], dict)]
        if bad_rewriters:
            available = ', '.join(sorted(_REWRITERS)) or '(none)'
            raise ValueError(
                f'Unknown rewriter(s): '
                f'{", ".join(repr(k) for k in bad_rewriters)}; available '
                f'rewriters: {available} (in "{description}")')
        if unknown:
            raise ValueError('Unrecognised substitution key(s): '
                             f'{", ".join(repr(k) for k in unknown)}')
        raise ValueError(
            f'No rewriter specified (in "{description}"); expected one of: '
            f'{", ".join(sorted(_REWRITERS)) or "(none)"}')

    @staticmethod
    def _parse_count(data: dict[str, object],
                     description: str,
                     default: int | None = None) -> int:
        expected_count = data.get('count', 1 if default is None else default)
        # bool is a subclass of int; reject it explicitly.
        if (not isinstance(expected_count, int)
                or isinstance(expected_count, bool)):
            raise ValueError(f'count must be an integer (in "{description}")')
        return expected_count


@dataclass(frozen=True)
class MatchExpectation:
    """How many matches an operation must make to count as correctly applied.

    A closed range `[minimum, maximum]`; `maximum is None` means unbounded. The
    canonical forms cover every case plaster needs:

    - `exactly(n)`   — precisely `n` matches (the default, `exactly(1)`).
    - `at_least_one` — one or more (the `count: 0` form).
    - `optional`     — any number including zero; never fails on its own. Use
      for an operation that may legitimately find nothing, leaving a
      cross-operation rule (e.g. "at least one of these") to a rewriter's own
      validation.
    """

    minimum: int
    maximum: int | None

    @classmethod
    def exactly(cls, n: int) -> MatchExpectation:
        return cls(n, n)

    @classmethod
    def at_least_one(cls) -> MatchExpectation:
        return cls(1, None)

    @classmethod
    def optional(cls) -> MatchExpectation:
        return cls(0, None)

    @classmethod
    def from_count(cls, count: int) -> MatchExpectation:
        """Map an entry's `count:` to an expectation (`0` -> one-or-more)."""
        return cls.at_least_one() if count == 0 else cls.exactly(count)

    def accepts(self, matches: int) -> bool:
        """True if `matches` satisfies this expectation."""
        return matches >= self.minimum and (self.maximum is None
                                            or matches <= self.maximum)

    def error_for(self, matches: int) -> str | None:
        """A failure message if `matches` is unacceptable, else None.

        The `exactly`/`at_least_one` wordings match plaster's historical count
        errors so existing diagnostics are unchanged.
        """
        if self.accepts(matches):
            return None
        if self.minimum == 1 and self.maximum is None:
            return 'Expected at least one match but found none'
        if self.minimum == self.maximum:
            return (f'Unexpected number of matches ({matches} vs '
                    f'{self.minimum})')
        upper = 'any' if self.maximum is None else self.maximum
        return (f'Unexpected number of matches ({matches} vs '
                f'{self.minimum}..{upper})')


class Regex(Rewriter):
    """A regex substitution applied with `re.subn` (the native rewriter).
    """

    NAME: Final = 'regex'
    SUMMARY: Final = 'A Python `re.subn` substitution (the default rewriter).'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Applies a Python `re.subn` substitution to the target source. This is
        the default rewriter, and the most flexible one.

        Fields (nested under the `regex:` key):

        - `pattern` — a literal string to match; it is regex-escaped for you.
          Mutually exclusive with `re_pattern`.
        - `re_pattern` — a regular expression to match. Mutually exclusive with
          `pattern`.
        - `replace` — the replacement string. Backreferences such as `\1` refer
          to capture groups in the pattern.
        - `re_flags` — a list of Python `re` flag names, e.g. `[DOTALL]`.

        The item-level `count` field asserts how many matches are expected
        (default 1; 0 means "one or more").

        Example:

        ```yaml
        substitutions:
          - description: Zero the split-view inset.
            regex:
              re_pattern: 'kSplitViewContentInset = 8;'
              replace: 'kSplitViewContentInset = 0;'
        ```
    """

    # The pattern/replacement fields, nested under the `regex:` op key.
    _FIELD_KEYS = frozenset(('pattern', 're_pattern', 'replace', 're_flags'))

    def __init__(self, *, re_pattern: str, replace: str, re_flags: int = 0):
        self._re_pattern = re_pattern
        self._replace = replace
        self._re_flags = re_flags

    def apply(self,
              contents: str,
              *,
              count: int,
              description: str,
              blank_for_parse: bool = False) -> tuple[str, list[str]]:
        del description  # Only the count matters for the regex diagnostic.
        del blank_for_parse  # Text substitution never parses; nothing to relax.
        # `count=0` here means "replace every match"; how many were expected is
        # validated afterwards against the entry's `count:`.
        content, matches = re.subn(self._re_pattern,
                                   self._replace,
                                   contents,
                                   flags=self._re_flags,
                                   count=0)
        error = MatchExpectation.from_count(count).error_for(matches)
        return content, [error] if error else []

    @classmethod
    def parse(cls, body: object, *, description: str) -> Regex:
        """Build from a `regex:` body (a mapping of the regex fields)."""
        if not isinstance(body, dict):
            raise ValueError(f'"regex" must be a mapping (in "{description}")')
        unknown = sorted(set(body) - cls._FIELD_KEYS)
        if unknown:
            raise ValueError(
                f'Unrecognised regex field(s): '
                f'{", ".join(repr(k) for k in unknown)} (in "{description}")')

        pattern = body.get('pattern')
        re_pattern = body.get('re_pattern')
        if pattern is not None and re_pattern is not None:
            raise ValueError(
                f'Please specify either pattern or re_pattern, not both '
                f'(in "{description}")')
        if pattern is None and re_pattern is None:
            raise ValueError(f'No pattern specified (in "{description}")')
        if pattern is not None:
            if not isinstance(pattern, str):
                raise ValueError(
                    f'pattern must be a string (in "{description}")')
            re_pattern = re.escape(pattern)
        elif not isinstance(re_pattern, str):
            raise ValueError(
                f're_pattern must be a string (in "{description}")')

        replace = body.get('replace')
        if not isinstance(replace, str):
            raise ValueError(
                f'No replace value specified (in "{description}")')

        return cls(re_pattern=re_pattern,
                   replace=replace,
                   re_flags=cls._parse_flags(body.get('re_flags', []),
                                             description))

    @staticmethod
    def _parse_flags(flags_raw: object, description: str) -> int:
        if not isinstance(flags_raw, list):
            raise ValueError(
                f're_flags must be a list of strings (in "{description}")')
        re_flags = 0
        for flag in flags_raw:
            if not isinstance(flag, str):
                raise ValueError(
                    f're_flags entries must be strings (in "{description}")')
            if not (flag.isupper() and hasattr(re, flag)):
                raise ValueError(f'Invalid re flag specified: {flag}')
            re_flags |= getattr(re, flag)
        return re_flags


@dataclass(frozen=True)
class Operation:
    """One planned ast-grep op invocation: the whole contract to the engine.

    `op_id` names an `ast.rewriter` in `rewriters.pyl`; `inputs` binds each of
    that op's declared `inputs` to a concrete string. Everything else the engine
    needs -- which matcher to run, which adjacent tokens to consume, the
    replacement template -- lives in the op spec, so a frontend `Rewriter`
    composes purely by emitting `Operation`s (one, several of the same op, or a
    mix of different ops) and never touches engine internals.

    `expectation` is how many matches this operation must make to be considered
    correctly applied; it is validation metadata that `AstRewriter.run` ignores
    (the engine just reports how many matches it made) and the composing
    `Rewriter` checks. A composed rewriter sets a per-operation expectation --
    e.g. `add_friend` expects each friend inserted exactly once regardless of
    how many friends there are -- rather than folding everything into one total.
    """

    # The `rewriters.pyl` rewriter op id this invokes (e.g. `cxx.make_virtual`).
    op_id: str

    # Values bound to the op's declared `inputs`, injected into its templates.
    inputs: dict[str, str]

    # How many matches this operation must make (default: exactly one).
    expectation: MatchExpectation = MatchExpectation.exactly(1)


class _AstGrepRewriter(Rewriter):
    """Base for rewriters backed by an ast-grep op declared in `rewriters.pyl`.

    A concrete subclass sets the usual `NAME`/`SUMMARY`/`HELP` metadata plus the
    `OP_ID` it resolves to. `apply` runs whatever `operations` returns against
    the engine, so a subclass expresses itself entirely by *composing*
    operations rather than by driving the engine.

    The default `operations`/`parse` cover the common 1:1 case: the YAML body is
    a flat mapping of exactly the op's declared `inputs` (read from the spec,
    the single source of truth), producing a single operation whose expectation
    is the entry's `count:`. Subclasses that take a richer body (e.g. a list-
    valued field expanding to several operations) override `parse` and
    `operations`, and may override `validate_outcomes` to add cross-operation
    rules (e.g. "at least one of these optional operations must apply").
    """

    # The `rewriters.pyl` op id this resolves to (e.g. `cxx.make_virtual`).
    OP_ID: ClassVar[str] = ''

    def __init__(self, inputs: dict[str, str] | None = None):
        # The flat default binds these to the op's single operation; composing
        # subclasses hold their own parsed shape and leave this empty.
        self._inputs = inputs if inputs is not None else {}

    def source_language(self) -> str:
        """The `<lang>.` prefix for the op (e.g. `cxx` for `cxx.make_virtual`).

        AST rewriters parse the target in this language, so they are only valid
        on sources of it.
        """
        return self.OP_ID.split('.', 1)[0]

    def operations(self, count: int) -> list[Operation]:
        """The ordered ast-grep operations this rewriter expands to.

        `count` is the entry's `count:`; the default single operation adopts it
        as its expectation, so a flat rewriter keeps plaster's original count
        semantics. Composed rewriters typically ignore it in favour of a
        per-operation expectation.
        """
        return [
            Operation(self.OP_ID, self._inputs,
                      MatchExpectation.from_count(count))
        ]

    def apply(self,
              contents: str,
              *,
              count: int,
              description: str,
              blank_for_parse: bool = False) -> tuple[str, list[str]]:
        engine = AstRewriter(RewritersEval.load(),
                             contents,
                             blank_for_parse=blank_for_parse)
        outcomes = [(op, engine.run(op)) for op in self.operations(count)]
        return engine.content, self.validate_outcomes(outcomes, description)

    def validate_outcomes(self, outcomes: list[tuple[Operation, int]],
                          description: str) -> list[str]:
        """Errors for `(operation, matches)` outcomes; empty when all applied.

        The default checks each operation against its own `expectation`.
        Override to add cross-operation rules -- an override typically calls
        `super().validate_outcomes(...)` first, then appends group checks.
        """
        del description  # Historical count diagnostics do not name the entry.
        errors = []
        for op, matches in outcomes:
            error = op.expectation.error_for(matches)
            if error:
                errors.append(error)
        return errors

    @classmethod
    def declared_inputs(cls) -> frozenset[str]:
        """The op's declared `inputs`, read from the `rewriters.pyl` spec."""
        return frozenset(RewritersEval.load().rewriter(cls.OP_ID)['inputs'])

    @classmethod
    def parse(cls, body: object, *, description: str) -> _AstGrepRewriter:
        """Validate a `<NAME>:` body of string args and build the rewriter."""
        keys = cls.declared_inputs()
        if not isinstance(body, dict):
            raise ValueError(
                f'"{cls.NAME}" must be a mapping (in "{description}")')
        unknown = sorted(set(body) - keys)
        if unknown:
            raise ValueError(
                f'Unrecognised {cls.NAME} arg(s): '
                f'{", ".join(repr(k) for k in unknown)} (in "{description}")')
        missing = sorted(keys - set(body))
        if missing:
            raise ValueError(f'{cls.NAME} requires arg(s): '
                             f'{", ".join(missing)} (in "{description}")')
        for key in sorted(keys):
            if not isinstance(body[key], str):
                raise ValueError(f'{cls.NAME} `{key}` must be a string '
                                 f'(in "{description}")')
        return cls({key: body[key] for key in keys})


class MakeVirtual(_AstGrepRewriter):
    """Make a C++ method declaration `virtual`, via the ast-grep rewriters."""

    NAME: Final = 'make_virtual'
    OP_ID: Final = 'cxx.make_virtual'
    SUMMARY: Final = 'Prepend `virtual ` to a class method declaration.'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Adds `virtual ` to a C++ method declaration. A leading attribute like
        `[[nodiscard]]` is kept first, so `virtual` lands after it
        (`[[nodiscard]] virtual T Foo()`), as the grammar requires.

        Fields:

        - `class_name` — the class declaring the method.
        - `method_name` — the method's name. Quote a destructor
          (`'~Foo'`), since a leading `~` is YAML null.

        Each overload sharing the name is one change, so an overloaded method
        needs a matching `count`.

        Example:

        ```yaml
        substitutions:
          - description: Make the destructor virtual for subclassing.
            make_virtual:
              class_name: DraggingTabsSession
              method_name: '~DraggingTabsSession'
        ```
    """


class AddFriend(_AstGrepRewriter):
    """Add one or more `friend` declarations to a C++ class's private section,
    via the ast-grep rewriters."""

    NAME: Final = 'add_friend'
    OP_ID: Final = 'cxx.add_friend'
    SUMMARY: Final = 'Add `friend` declaration(s) to a class private section.'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Inserts one or more `friend` declarations as the first lines of a
        class's private section. The class must have a `private:` section.

        Fields:

        - `class_name` — the class to befriend from.
        - `friend_type` — the friend's declaration body, e.g. `class BraveFoo`
          becomes `friend class BraveFoo;`. May be a single string, or a list
          of them.

        Example:

        ```yaml
        substitutions:
          - description: Let the Brave subclass reach private members.
            add_friend:
              class_name: MultiContentsView
              friend_type: class BraveMultiContentsView

          - description: Friend the Brave worker and its test.
            add_friend:
              class_name: DataTypeWorker
              friend_type:
                - class BraveDataTypeWorker
                - class BraveDataTypeWorkerTest
        ```
    """

    def __init__(self, *, class_name: str, friend_types: list[str]):
        # This rewriter holds its own parsed shape and builds operations from
        # it, so the base's flat inputs stay empty.
        super().__init__()
        self._class_name = class_name
        self._friend_types = friend_types

    def operations(self, count: int) -> list[Operation]:
        # Each friend targets the class's single private section, so it is
        # expected exactly once -- independent of how many friends are listed,
        # which is why the entry's `count:` is not used here.
        #
        # `add_friend` inserts each friend as the *first* private line, so a
        # later insertion ends up above an earlier one. Emit in reverse so the
        # friends land in the order the user listed them.
        del count
        return [
            Operation(self.OP_ID, {
                'class_name': self._class_name,
                'friend_type': friend_type,
            }, MatchExpectation.exactly(1))
            for friend_type in reversed(self._friend_types)
        ]

    @classmethod
    def parse(cls, body: object, *, description: str) -> AddFriend:
        """Validate an `add_friend:` body, accepting a single friend or a list.

        `friend_type` may be a string (one friend) or a non-empty list of
        strings (several); everything else matches the flat-args form.
        """
        if not isinstance(body, dict):
            raise ValueError(
                f'"{cls.NAME}" must be a mapping (in "{description}")')
        required = {'class_name', 'friend_type'}
        unknown = sorted(set(body) - required)
        if unknown:
            raise ValueError(
                f'Unrecognised {cls.NAME} arg(s): '
                f'{", ".join(repr(k) for k in unknown)} (in "{description}")')
        missing = sorted(required - set(body))
        if missing:
            raise ValueError(f'{cls.NAME} requires arg(s): '
                             f'{", ".join(missing)} (in "{description}")')
        if not isinstance(body['class_name'], str):
            raise ValueError(f'{cls.NAME} `class_name` must be a string '
                             f'(in "{description}")')
        return cls(class_name=body['class_name'],
                   friend_types=cls._parse_friend_types(
                       body['friend_type'], description))

    @staticmethod
    def _parse_friend_types(value: object, description: str) -> list[str]:
        """Normalise `friend_type` to a non-empty list of strings."""
        if isinstance(value, str):
            return [value]
        if (isinstance(value, list) and value
                and all(isinstance(item, str) for item in value)):
            return list(value)
        raise ValueError(
            f'{AddFriend.NAME} `friend_type` must be a string or a non-empty '
            f'list of strings (in "{description}")')


class DropFinal(_AstGrepRewriter):
    """Remove the `final` specifier from a C++ class declaration, via the
    ast-grep rewriters, so the class can be subclassed."""

    NAME: Final = 'drop_final'
    OP_ID: Final = 'cxx.drop_final'
    SUMMARY: Final = 'Remove the `final` specifier from a class declaration.'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Removes the `final` specifier from a C++ class declaration, so the class
        can be subclassed. Only the class-head `final` is removed; a method's
        trailing `final` is left untouched.

        Fields:

        - `class_name` — the class to drop `final` from.

        Example:

        ```yaml
        substitutions:
          - description: Drop `final` so Brave can subclass.
            drop_final:
              class_name: DraggingTabsSession
        ```
    """


class PreemptFunctionImpl(_AstGrepRewriter):
    """Insert a statement block at the top of a C++ function body, via the
    ast-grep rewriters.
    """

    NAME: Final = 'preempt_function_impl'
    OP_ID: Final = 'cxx.preempt_function_impl'
    SUMMARY: Final = 'Insert a statement block at the top of a function body.'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Inserts a statement block as the first thing in a C++ function's body,
        right after the opening brace. Use it to preempt the implementation of
        an upstream function, rather than renaming it to an inner
        `_ChromiumImpl`.

        Fields:

        - `function_name` — the function's name exactly as its definition
          writes it: a qualified `Class::Method` for a member, or the bare name
          for a free function.

        Exactly one of the following selects what to insert:

        - `return_if` — a boolean condition that expands to `if (<condition>)
          return;`, or `if (<condition>) return <return_value>;` when
          `return_value` is also given.
        - `code` — a free-form statement block. Write it flush-left; the whole
          block is indented to the body's first-statement level for you.

        - `return_value` — optional, only valid with `return_if`: the value the
          generated early return yields.

        Each overload sharing the name is one match, so an overloaded method
        needs a matching `count`.

        Examples:

        ```yaml
        substitutions:
          - description: Skip autocomplete when Brave has it disabled.
            preempt_function_impl:
              function_name: OmniboxController::StartAutocomplete
              return_if: '!IsAutocompleteEnabled(client_->GetPrefs())'

          - description: Never expose the feedback buttons; return false early.
            preempt_function_impl:
              function_name: OmniboxResultView::IsFeedbackButtonVisible
              return_if: 'true'
              return_value: 'false'

          - description: Pin Brave-managed actions before the upstream body.
            preempt_function_impl:
              function_name: CustomizeToolbarHandler::PinAction
              code: |-
                if (PinBraveAction(action_id, prefs(), pin)) {
                  return;
                }
        ```

        The last entry turns this upstream definition:

        ```cpp
        void CustomizeToolbarHandler::PinAction(int action_id, bool pin) {
          // ... upstream body ...
        }
        ```

        into (note the flush-left `code` block indented to the body level):

        ```cpp
        void CustomizeToolbarHandler::PinAction(int action_id, bool pin) {
          if (PinBraveAction(action_id, prefs(), pin)) {
            return;
          }
          // ... upstream body ...
        }
        ```
    """

    # The keys that select what to insert; exactly one must be present.
    _MODE_KEYS: Final = frozenset({'code', 'return_if'})

    def __init__(self, *, function_name: str, prologue: str):
        # This rewriter holds its own parsed shape (the resolved prologue text)
        # and builds its operation from it, so the base's flat inputs stay
        # empty.
        super().__init__()
        self._function_name = function_name
        self._prologue = prologue

    def operations(self, count: int) -> list[Operation]:
        # A function body is matched once per overload, so the entry's `count:`
        # carries through unchanged (an overloaded method needs a matching
        # count).
        #
        # The prologue lands as the body's first statement, so indent every
        # non-blank line to the body's first level -- two spaces, which is where
        # an out-of-line `Class::Method` definition always sits (file/namespace
        # scope). Escape backslashes last, since the text is spliced into a
        # `re.sub` replacement where a backslash is special.
        prologue = _indent_yaml(self._prologue).replace('\\', '\\\\')
        return [
            Operation(self.OP_ID, {
                'function_name': self._function_name,
                'prologue': prologue,
            }, MatchExpectation.from_count(count))
        ]

    @classmethod
    def parse(cls, body: object, *, description: str) -> PreemptFunctionImpl:
        """Validate a `preempt_function_impl:` body and resolve what to insert.

        Requires `function_name`, plus exactly one of `code` or `return_if`;
        `return_value` is optional and only valid with `return_if`.
        """
        if not isinstance(body, dict):
            raise ValueError(
                f'"{cls.NAME}" must be a mapping (in "{description}")')
        allowed = {'function_name', 'return_value'} | cls._MODE_KEYS
        unknown = sorted(set(body) - allowed)
        if unknown:
            raise ValueError(
                f'Unrecognised {cls.NAME} arg(s): '
                f'{", ".join(repr(k) for k in unknown)} (in "{description}")')
        if not isinstance(body.get('function_name'),
                          str) or not body['function_name']:
            raise ValueError(f'{cls.NAME} `function_name` must be a non-empty '
                             f'string (in "{description}")')
        prologue = cls._resolve_prologue(body, description)
        return cls(function_name=body['function_name'], prologue=prologue)

    @classmethod
    def _resolve_prologue(cls, body: dict, description: str) -> str:
        """Build the text to insert from the `code`/`return_if` fields."""
        modes = sorted(cls._MODE_KEYS & set(body))
        if len(modes) != 1:
            raise ValueError(
                f'{cls.NAME} requires exactly one of `code` or `return_if` '
                f'(in "{description}")')
        mode = modes[0]
        if mode == 'code':
            if 'return_value' in body:
                raise ValueError(
                    f'{cls.NAME} `return_value` is only valid with `return_if` '
                    f'(in "{description}")')
            code = body['code']
            if not isinstance(code, str) or not code:
                raise ValueError(
                    f'{cls.NAME} `code` must be a non-empty string '
                    f'(in "{description}")')
            return code
        condition = body['return_if']
        if not isinstance(condition, str) or not condition:
            raise ValueError(f'{cls.NAME} `return_if` must be a non-empty '
                             f'string (in "{description}")')
        return_value = body.get('return_value', '')
        if not isinstance(return_value, str):
            raise ValueError(f'{cls.NAME} `return_value` must be a string '
                             f'(in "{description}")')
        value = f' {return_value}' if return_value else ''
        return f'if ({condition}) return{value};'


class RenameClass(_AstGrepRewriter):
    """Rename a C++ class."""

    NAME: Final = 'rename_class'
    OP_ID: Final = 'cxx.rename_class'
    SUMMARY: Final = 'Rename a class and every code token of its name.'
    # For this rewriter we have less of a concerns about `count`.
    DEFAULT_COUNT: Final = 0
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Renames a C++ class everywhere its name appears as a *code token*: the
        class declaration, type positions (`Foo*`, `<Foo>`), the `Foo::`
        qualifier on out-of-line members, and constructor/destructor names.

        Fields:

        - `class_name` — the current class name.
        - `rename` — the name to rename it to (e.g. `Foo_ChromiumImpl`).

        Example:

        ```yaml
        substitutions:
          - description: Rename so the Brave subclass can reuse the name.
            rename_class:
              class_name: TestLauncher
              rename: TestLauncher_ChromiumImpl
        ```
    """


class AddToProtected(_AstGrepRewriter):
    """Add a member declaration to a C++ class's `protected:` section"""

    NAME: Final = 'add_to_protected'
    # Two ops share the work (see `operations`); this names the language and a
    # representative op for the base's helpers.
    OP_ID: Final = 'cxx.add_to_protected'
    SUMMARY: Final = 'Add code to a class protected section (creating one if '\
                     'needed).'
    # Authored in Markdown; `Help` renders it with rich.
    HELP: Final = r"""
        Adds code to a C++ class's `protected:` section. It will either use an
        existing `protected:` section or create a new one.

        Fields:

        - `class_name` — the class to add to.
        - `code` — free-form text to insert in the protected section, e.g. a
          declaration like `virtual void OnFoo() = 0;`.

        Example:

        ```yaml
        substitutions:
          - description: Add a protected hook for the Brave subclass to override.
            add_to_protected:
              class_name: TestLauncher_ChromiumImpl
              code: 'virtual const TestResult& OnTestResult(const TestResult& result) = 0;'
        ```
    """

    # This rewriter uses composite operations that look for an existing
    # protected section, or add one before `private:`.
    _ADD_TO_EXISTING: Final = 'cxx.add_to_protected'
    _CREATE_BEFORE_PRIVATE: Final = 'cxx.add_protected_before_private'

    @classmethod
    def validate_count(cls, count: int, description: str) -> None:
        # The code is inserted once, into whichever placement applies, so a
        # `count:` other than 1 is meaningless here.
        if count != 1:
            raise ValueError(f'{cls.NAME} adds the code exactly once and does '
                             f'not accept a count other than 1 '
                             f'(in "{description}")')

    def __init__(self, *, class_name: str, code: str):
        super().__init__()
        self._class_name = class_name
        self._code = code

    def apply(self,
              contents: str,
              *,
              count: int,
              description: str,
              blank_for_parse: bool = False) -> tuple[str, list[str]]:
        # The code is inserted exactly once either in an existing protected
        # section or in a new one created just before a private section.
        del count, description
        engine = AstRewriter(RewritersEval.load(),
                             contents,
                             blank_for_parse=blank_for_parse)
        code = _indent_yaml(self._code).replace('\\', '\\\\')
        inputs = {'class_name': self._class_name, 'code': code}
        op = Operation(self._ADD_TO_EXISTING, inputs,
                       MatchExpectation.exactly(1))
        changes = engine.run(op)
        if changes == 0:
            op = Operation(self._CREATE_BEFORE_PRIVATE, inputs,
                           MatchExpectation.exactly(1))
            changes = engine.run(op)
        error = op.expectation.error_for(changes)
        return engine.content, [error] if error else []

    @classmethod
    def parse(cls, body: object, *, description: str) -> AddToProtected:
        """Validate an `add_to_protected:` body of string args."""
        if not isinstance(body, dict):
            raise ValueError(
                f'"{cls.NAME}" must be a mapping (in "{description}")')
        required = {'class_name', 'code'}
        unknown = sorted(set(body) - required)
        if unknown:
            raise ValueError(
                f'Unrecognised {cls.NAME} arg(s): '
                f'{", ".join(repr(k) for k in unknown)} (in "{description}")')
        missing = sorted(required - set(body))
        if missing:
            raise ValueError(f'{cls.NAME} requires arg(s): '
                             f'{", ".join(missing)} (in "{description}")')
        for key in sorted(required):
            if not isinstance(body[key], str) or not body[key]:
                raise ValueError(f'{cls.NAME} `{key}` must be a non-empty '
                                 f'string (in "{description}")')
        return cls(class_name=body['class_name'], code=body['code'])


_REWRITERS: MappingProxyType[str, type[Rewriter]] = MappingProxyType({
    rewriter.NAME: rewriter
    for rewriter in (Regex, MakeVirtual, AddFriend, DropFinal,
                     PreemptFunctionImpl, RenameClass, AddToProtected)
})


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
                if file.endswith(PLASTER_EXTENSION):
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

        # Only the plaster file itself is guaranteed to exist here; any
        # of the other files may be missing and that is by itself a
        # reason to re-apply.
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
        suffix = self.path.suffix.lower()

        info = PatchinfoBuilder(self.path)
        if suffix == '.yaml':
            doc = Substitution.from_yaml(info.plaster_contents)
            is_cxx = _is_cxx_source(self.path)
            if doc.blank_macros_for_ast_parsing and not is_cxx:
                raise ValueError(
                    '`blank_macros_for_ast_parsing` is only supported for C++ '
                    f'sources (in {self.path})')
            if not is_cxx:
                for substitution in doc.substitutions:
                    if substitution.rewriter.source_language() == 'cxx':
                        raise ValueError(
                            f'the `{substitution.rewriter.NAME}` rewriter is '
                            f'only supported for C++ sources (in {self.path})')
        else:
            raise ValueError(f'Unsupported plaster file extension: {suffix}')
        contents = repository.chromium.read_file(info.source)
        errors = []

        try:
            for substitution in doc.substitutions:
                # Each substitution owns its count validation (per-operation for
                # composed rewriters) and reports any failures; we just attach
                # the file being patched.
                contents, sub_errors = substitution.apply(
                    contents, blank_for_parse=doc.blank_macros_for_ast_parsing)
                errors.extend(f'{error} in {self.path}'
                              for error in sub_errors)

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


class RewritersSchemaError(PlasterError):
    """Raised when `rewriters.pyl` does not conform to the expected schema."""


# Namespace mapping for ast-grep rewriter types. This list will grow as more
# rewriters are added for other languages.
_LANGUAGE_BY_PREFIX = MappingProxyType({'cxx': 'cpp'})


def _is_regex(pattern: str) -> bool:
    """schema predicate: True if `pattern` compiles as a regular expression."""
    try:
        re.compile(pattern)
        return True
    except re.error:
        return False


def _is_op_id(op_id: str) -> bool:
    """schema predicate: True if `op_id` is `<known-lang>.<name>`."""
    prefix, _, name = op_id.partition('.')
    return bool(name) and prefix in _LANGUAGE_BY_PREFIX


def _placeholders(template: str) -> set[str]:
    """The set of named `{placeholder}` fields referenced in `template`."""
    return {
        name
        for _, name, _, _ in string.Formatter().parse(template) if name
    }


# Reusable leaf schemas.
_NON_EMPTY_STR = schema.And(str, len, error='must be a non-empty string')
_REGEX_STR = schema.And(str,
                        _is_regex,
                        error='must be a valid regular expression')
_OP_ID = schema.And(str,
                    _is_op_id,
                    error='op id must be "<lang>.<name>" with a known '
                    'language prefix')

# The "result" block shared by matcher and rewriter ops.
_RESULT_SCHEMA = {
    'node': _NON_EMPTY_STR,
}

# A matcher op: a templated ast-grep query plus its result shape. Its inputs
# are the template's `{placeholder}`s, inferred rather than declared.
_MATCHER_SCHEMA = {
    'template': _NON_EMPTY_STR,
    'result': _RESULT_SCHEMA,
}

# A rewriter op: locates nodes through a matcher op and edits each via a regex
# substitution. `inputs` is its public interface (validated against the
# templates it feeds in `_check_cross_references`); `replace` may name adjacent
# tokens to fold into each rewritten span. `first_match`, when true, rewrites
# only the first match (in source order) and ignores any later ones.
_REWRITER_SCHEMA = {
    'matcher': _NON_EMPTY_STR,
    'inputs': [str],
    schema.Optional('first_match'): bool,
    'replace': {
        schema.Optional('consume_before'): str,
        schema.Optional('consume_after'): str,
        're_pattern': _REGEX_STR,
        'replace': str,
    },
    'result': _RESULT_SCHEMA,
}

# Top-level schema for rewriters.pyl.
_REWRITERS_SCHEMA = schema.Schema({
    schema.Optional('ast.matcher'): {
        schema.Optional(_OP_ID): _MATCHER_SCHEMA
    },
    schema.Optional('ast.rewriter'): {
        schema.Optional(_OP_ID): _REWRITER_SCHEMA
    },
})


class RewritersEval:
    """Loads and schema-validates `rewriters.pyl`.

    The main function of this class is to make sure the file is valid, and to
    provide access to the loaded content. Note the distinction from the
    `Rewriter` op classes above: `Rewriter`/`Regex` are the `substitutions:`
    transforms, while this loads the declarative ast-grep matcher/rewriter
    *specs* those transforms will drive.
    """

    # Process-wide singleton, loaded once from REWRITERS_FILE by load().
    _instance: RewritersEval | None = None

    def __init__(self, content: str, *, source: str = str(REWRITERS_FILE)):
        """Parse and validate `content` (the text of a rewriters.pyl file).

        Raises RewritersSchemaError if the content is not a valid literal or
        does not satisfy the schema. `source` is only used in error messages.
        """
        self._source = source
        data = self._parse(content, source)
        try:
            data = _REWRITERS_SCHEMA.validate(data)
        except schema.SchemaError as e:
            raise RewritersSchemaError(f'{source}: {e}') from e
        self._matchers = data.get('ast.matcher', {})
        self._rewriters = data.get('ast.rewriter', {})
        self._check_cross_references()

    @classmethod
    def load(cls) -> RewritersEval:
        """Return the process-wide RewritersEval, reading rewriters.pyl once."""
        if cls._instance is None:
            cls._instance = cls(REWRITERS_FILE.read_bytes().decode('utf-8'),
                                source=str(REWRITERS_FILE))
        return cls._instance

    # -- access -------------------------------------------------------------

    @property
    def matchers(self) -> MappingProxyType[str, dict]:
        """Read-only mapping of matcher op id -> validated matcher spec."""
        return MappingProxyType(self._matchers)

    @property
    def rewriters(self) -> MappingProxyType[str, dict]:
        """Read-only mapping of rewriter op id -> validated rewriter spec."""
        return MappingProxyType(self._rewriters)

    def matcher(self, op_id: str) -> dict:
        """Return the matcher spec for `op_id`, or raise if it is unknown."""
        try:
            return self._matchers[op_id]
        except KeyError:
            raise RewritersSchemaError(
                f'unknown matcher op: {op_id!r}') from None

    def rewriter(self, op_id: str) -> dict:
        """Return the rewriter spec for `op_id`, or raise if it is unknown."""
        try:
            return self._rewriters[op_id]
        except KeyError:
            raise RewritersSchemaError(
                f'unknown rewriter op: {op_id!r}') from None

    @classmethod
    def language_of(cls, op_id: str) -> str:
        """Return the ast-grep language id derived from `op_id`'s prefix."""
        prefix = op_id.split('.', 1)[0]
        try:
            return _LANGUAGE_BY_PREFIX[prefix]
        except KeyError:
            raise RewritersSchemaError(
                f'op {op_id!r} has unknown language prefix {prefix!r}; '
                f'known prefixes: {sorted(_LANGUAGE_BY_PREFIX)}') from None

    # -- validation ---------------------------------------------------------

    @staticmethod
    def _parse(content: str, source: str) -> object:
        """Evaluate `content` as a Python literal (shape is checked later)."""
        try:
            return ast.literal_eval(content)
        except (ValueError, SyntaxError, TypeError) as e:
            raise RewritersSchemaError(
                f'{source}: not a valid Python literal: {e}') from e

    def _check_cross_references(self) -> None:
        """Validate rules that span records, which schema cannot express.

        Each rewriter must reference a matcher that exists, must declare the
        same result node as that matcher (it replaces the node the matcher
        locates, so the two must agree), and its declared `inputs` must be
        exactly the `{placeholder}`s used across the matcher template and the
        `replace` template -- so the op's advertised interface never drifts from
        what its templates actually consume.
        """
        for op_id, spec in self._rewriters.items():
            ref = spec['matcher']
            if ref not in self._matchers:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} references unknown '
                    f'matcher {ref!r}')
            matcher = self._matchers[ref]
            matcher_node = matcher['result']['node']
            if spec['result']['node'] != matcher_node:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} result node '
                    f'{spec["result"]["node"]!r} does not match matcher '
                    f'{ref!r} node {matcher_node!r}')

            declared = set(spec['inputs'])
            used = (_placeholders(matcher['template'])
                    | _placeholders(spec['replace']['replace']))
            undeclared = sorted(used - declared)
            if undeclared:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} templates use '
                    f'undeclared input(s): {", ".join(undeclared)}')
            unused = sorted(declared - used)
            if unused:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} declares input(s) '
                    f'never used in its templates: {", ".join(unused)}')


def _ast_grep_platform_dir() -> str:
    """Host-OS token in ast-grep's per-platform dir name (`ast-grep-<os>`).

    Mirrors `_platform_dir()` in third_party/ast-grep/build_ast_grep.py.
    """
    if sys.platform == 'darwin':
        return 'mac_arm64' if platform.machine() == 'arm64' else 'mac'
    if sys.platform == 'win32':
        return 'win'
    return 'linux'


# Path to the ast-grep binary provisioned under brave/third_party/ast-grep.
_AST_GREP_EXE = '.exe' if sys.platform == 'win32' else ''
AST_GREP_BIN = (Path(__file__).resolve().parents[2] / 'third_party' /
                'ast-grep' / f'ast-grep-{_ast_grep_platform_dir()}' / 'bin' /
                f'ast-grep{_AST_GREP_EXE}')


class AstGrepError(PlasterError):
    """Raised when the ast-grep binary fails to run a rule."""


@dataclass(frozen=True)
class AstMatch:
    """The byte range of a single ast-grep match within the scanned source.

    `start` is the match's byte offset and `length` its size in bytes; `end`
    is the exclusive offset one past it. The matched text is not stored -- read
    it back from the source (`source[start:end]`) so the bytes have a single
    source of truth, regardless of multi-byte content upstream.
    """

    start: int
    length: int

    @property
    def end(self) -> int:
        """Exclusive byte offset one past the match (`start + length`)."""
        return self.start + self.length


def _indent_yaml(text: str, spaces: int = 2) -> str:
    """Indent every non-blank line of `text` by `spaces`, for nesting YAML."""
    pad = ' ' * spaces
    return '\n'.join(pad + line if line.strip() else line
                     for line in text.splitlines())


def run_ast_grep(*, language: str, rule_body: str,
                 source: str) -> list[AstMatch]:
    """Run an ast-grep rule against in-memory source and return its matches.

    This function is the main entrypoint for the ast-grep binary invocation.

    `rule_body` is an ast-grep YAML rule body (the part under `rule:`). Source
    is fed over stdin with an explicit `--lang`. The reported offsets are into
    `source`'s UTF-8 bytes, regardless of how stdin is encoded.
    """
    doc = (f'id: plaster\nlanguage: {language}\nrule:\n' +
           _indent_yaml(rule_body))
    # Route through terminal.run for consistent logging / infra keep-alive,
    # like the rest of plaster's subprocess use. It runs with check=True, so a
    # non-zero exit (e.g. a bad rule) raises CalledProcessError.
    try:
        result = terminal.run([
            AST_GREP_BIN, 'scan', '--stdin', '--inline-rules', doc,
            '--json=stream'
        ],
                              stdin=source)
    except subprocess.CalledProcessError as e:
        raise AstGrepError(f'ast-grep failed ({e.returncode}): '
                           f'{(e.stderr or "").strip()}') from e

    matches = []
    for line in result.stdout.splitlines():
        if not line.strip():
            continue
        obj = json.loads(line)
        span = obj['range']['byteOffset']
        matches.append(
            AstMatch(start=span['start'], length=span['end'] - span['start']))
    return matches


class AstRewriter:
    """Applies ast-grep rewriter ops to the contents of a single file.

    Constructed with an already-parsed `RewritersEval` and the target file's
    contents. `run` executes one bound `Operation` over the current contents,
    mutating them in place and returning how many places changed; call it
    repeatedly to accumulate edits. The engine is fully self-contained: an op's
    matcher, replacement, and any adjacent tokens to consume all come from the
    op spec, so the `Rewriter` classes that drive it only choose which
    operations to run.
    """

    # Class-head export macro (e.g. `MODULES_EXPORT`, `COMPONENT_EXPORT(FOO)`):
    # the keyword and its space, the macro, then the start of the real name.
    # Used by `_prepare_cxx_for_parse`.
    _EXPORT_MACRO_RE = re.compile(r'(\b(?:class|struct)\s+)'
                                  r'([A-Z][A-Z0-9_]*_EXPORT(?:\s*\([^()]*\))?)'
                                  r'(\s+[A-Za-z_])')

    # A preprocessor conditional directive line (`#if`, `#ifdef` ...), matched
    # without its trailing newline. Used by `_prepare_cxx_for_parse`.
    _CXX_CONDITIONAL_RE = re.compile(
        r'(?m)^[ \t]*#[ \t]*(?:if|ifdef|ifndef|elif|else|endif)\b.*$')

    def __init__(self,
                 rewriters: RewritersEval,
                 content: str,
                 *,
                 blank_for_parse: bool = False):
        self._rewriters = rewriters
        self._source = content
        # When set, blank constructs tree-sitter cannot parse before matching
        # (see `_prepare_cxx_for_parse`). Off by default. Opt-in per file.
        self._blank_for_parse = blank_for_parse

    @property
    def content(self) -> str:
        """The current file contents, reflecting every applied rewrite."""
        return self._source

    def _prepare_cxx_for_parse(self) -> str:
        """Normalise C++ source for tree-sitter parsing

        Two constructs are handled:

        - A class-head export macro (`class MODULES_EXPORT Foo`): tree-sitter
          has no macro definitions, so it reads the macro as the class name and
          drops the real name, any `final`, and the body into an error node.
          Blanking the macro leaves a plain `class Foo ...`.
        - A preprocessor conditional (`#if`/`#endif`/...), most damaging inside
          a base-specifier list (`#if defined(USE_AURA)`), which has no grammar
          node and breaks the class head. Blanking the directive lines, keeping
          the guarded code, makes the head parse.

        Both substitutions only replace characters in place, so every byte
        offset is preserved. Blanking every conditional file-wide is safe
        because the caller opts in per file via `blank_macros_for_ast_parsing`,
        and a directive guarding an alternative definition would leave two
        matches that the count check flags.
        """
        source = self._EXPORT_MACRO_RE.sub(
            lambda m: m.group(1) + ' ' * len(m.group(2)) + m.group(3),
            self._source)
        return self._CXX_CONDITIONAL_RE.sub(
            lambda line: ' ' * len(line.group(0)), source)

    def run(self, op: Operation) -> int:
        """Run one bound `Operation`, mutate content, return the change count.

        Locates nodes via the op's matcher template, applies the op's `replace`
        regex substitution (with `op.inputs` filled into the replacement
        template) to each matched node, and splices the results back into the
        held contents from the end so earlier byte offsets stay valid. Returns
        the total number of substitutions made.

        An op's `replace` may name `consume_before` / `consume_after` literals
        that sit immediately before / after each matched node; they are folded
        into the rewritten span when the node kind stops short of an adjacent
        token (e.g. the `:` after an access_specifier, or the space before a
        `final` specifier). An op that sets `first_match` rewrites only the
        first match (in source order), leaving any later matches untouched.
        """
        rewriter = self._rewriters.rewriter(op.op_id)
        matcher = self._rewriters.matcher(rewriter['matcher'])
        language = self._rewriters.language_of(op.op_id)
        rule_body = matcher['template'].format(**op.inputs)

        # `_prepare_cxx_for_parse` blanks C++-specific constructs, so it only
        # applies to `cxx.` ops.
        blank = self._blank_for_parse and op.op_id.startswith('cxx.')
        source_for_parse = (self._prepare_cxx_for_parse()
                            if blank else self._source)
        matches = run_ast_grep(language=language,
                               rule_body=rule_body,
                               source=source_for_parse)
        if rewriter.get('first_match'):
            matches = matches[:1]

        replace = rewriter['replace']
        pattern = replace['re_pattern']
        replacement = replace['replace'].format(**op.inputs)
        before = replace.get('consume_before', '').encode('utf-8')
        after = replace.get('consume_after', '').encode('utf-8')

        source = self._source.encode('utf-8')
        edits = []
        total = 0
        for match in matches:
            start = match.start - len(before)
            end = match.end + len(after)
            if (before and source[start:match.start]
                    != before) or (after and source[match.end:end] != after):
                # An assumed adjacent token isn't there; skip rather than
                # corrupt. The caller's count check will flag the shortfall.
                continue
            text = source[match.start:match.end].decode('utf-8')
            new_text, changes = re.subn(pattern, replacement, text)
            if changes:
                edits.append((start, end, new_text))
                total += changes

        # Match offsets are into the source's UTF-8 bytes, so splice on bytes.
        # Apply from the end so each splice leaves earlier offsets unchanged.
        for start, end, new_text in sorted(edits, reverse=True):
            source = source[:start] + new_text.encode('utf-8') + source[end:]
        self._source = source.decode('utf-8')
        return total


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
            stem = f'rewrite/{base.replace("-", "/")}'
            expected_plaster_files.add(f'{stem}{PLASTER_EXTENSION}')
        elif (filepath.startswith('rewrite/')
              and filepath.endswith(PLASTER_EXTENSION)):
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


# The tool's one-line description, shared by the argparse parser and the
# `plaster --help` overview.
_TOOL_DESCRIPTION = (
    '🩹 A tool to generate patches into Chromium using plaster files.')

# A command registry: command name -> (its argparse parser, one-line summary).
_CommandRegistry = dict[str, tuple[argparse.ArgumentParser, str]]


class Help(argparse.Action):
    """`plaster --help [topic]`, implemented as the `--help` action.
    """

    # The colour to be used to highlight each command/rewriter.
    _TOPIC_STYLE = 'bold green'

    def __init__(self, option_strings: list[str], dest: str,
                 commands: _CommandRegistry, **kwargs):
        super().__init__(option_strings,
                         dest,
                         nargs='?',
                         default=argparse.SUPPRESS,
                         metavar='TOPIC',
                         help='Print help, optionally for a command or '
                         'rewriter topic.',
                         **kwargs)
        self._commands = commands

        # Set from the parser argparse hands to `__call__`.
        self._parser: argparse.ArgumentParser | None = None

    def __call__(self,
                 parser: argparse.ArgumentParser,
                 namespace: argparse.Namespace,
                 values: str | None,
                 option_string: str | None = None) -> None:
        del namespace, option_string
        self._parser = parser
        parser.exit(self.render(values))

    def render(self, topic: str | None) -> int:
        """Print the overview, a single category, or a topic's docs.

        With no topic, prints an overview. When called for a specific rewriters
        or command, it prints that topic's own docs. Returns the process exit
        code.
        """
        if topic is None:
            self._print_overview()
            return 0
        if topic == 'commands':
            self._print_commands()
            return 0
        if topic == 'rewriters':
            self._print_rewriters()
            return 0
        if topic in self._commands:
            self._commands[topic][0].print_help()
            return 0
        if topic in _REWRITERS:
            self._print_rewriter_help(topic)
            return 0
        console.print(f'[red]Unknown help topic:[/] {topic}', highlight=False)
        console.print('Run "plaster --help" to list commands and rewriters.')
        return 1

    def _print_overview(self) -> None:
        """Print the full overview: usage, description, categories, options."""
        console.print(self._parser.format_usage().rstrip(), highlight=False)
        console.print()
        console.print(_TOOL_DESCRIPTION, highlight=False)
        console.print()
        self._print_commands()
        console.print()
        self._print_rewriters()
        options = self._option_entries()
        if options:
            console.print()
            self._print_option_index(options)

    def _print_commands(self) -> None:
        """Print the `Commands` category from the command registry."""
        entries = [(name, summary)
                   for name, (_, summary) in self._commands.items()]
        self._print_topic_index('Commands', 'command', entries)

    def _print_rewriters(self) -> None:
        """Print the `Rewriters` category from the rewriter registry."""
        entries = [(name, rewriter.SUMMARY)
                   for name, rewriter in _REWRITERS.items()]
        self._print_topic_index('Rewriters', 'rewriter', entries)

    def _print_topic_index(self, title: str, kind: str,
                           entries: list[tuple[str, str]]) -> None:
        """Print one help category: a header plus colourised `name summary`
        rows.

        `kind` is the noun used in the "type ... for more details" hint (e.g.
        "command"). `entries` are `(name, summary)` pairs listed in order.
        """
        console.print(
            f'{title} [dim](type "plaster --help <{kind}>" for more '
            f'details)[/]:',
            highlight=False)
        width = max((len(name) for name, _ in entries), default=0)
        for name, summary in entries:
            console.print(
                f'  [{self._TOPIC_STYLE}]{name:<{width}}[/]  {summary}',
                highlight=False)

    @staticmethod
    def _print_option_index(entries: list[tuple[str, str]]) -> None:
        """Print the global `Options` block: `flags summary` rows.
        """
        console.print('Options:', highlight=False)
        width = max((len(flags) for flags, _ in entries), default=0)
        for flags, summary in entries:
            console.print(f'  {flags:<{width}}  {summary}', highlight=False)

    def _print_rewriter_help(self, name: str) -> None:
        """Print the full docs for the rewriter registered under `name`."""
        rewriter = _REWRITERS[name]
        console.print(f'[{self._TOPIC_STYLE}]{name}[/] — {rewriter.SUMMARY}',
                      highlight=False)
        console.print()
        # Rewriter help is authored in Markdown; render it with rich so inline
        # code and the YAML example code block display nicely.
        console.print(Markdown(rewriter.help_text()))

    def _option_entries(self) -> list[tuple[str, str]]:
        """`(flags, help)` rows for the active parser's global optionals.

        Reads the parser's own actions so the list stays in step with whatever
        `main` registers.
        """
        entries = []
        for action in self._parser._actions:  # pylint: disable=protected-access
            if not action.option_strings or action.help == argparse.SUPPRESS:
                continue
            flags = ', '.join(action.option_strings)
            if action.metavar:
                flags += f' {action.metavar}'
            entries.append((flags, action.help or ''))
        return entries


def main():
    # `add_help=False`: we install our own `--help` below so it can take an
    # optional topic (`plaster --help regex`).
    parser = argparse.ArgumentParser(description=_TOOL_DESCRIPTION,
                                     add_help=False)
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging')
    parser.add_argument('--infra-mode',
                        action='store_true',
                        help='Enable infra mode')
    subparsers = parser.add_subparsers(dest='command', required=True)

    commands: dict[str, tuple[argparse.ArgumentParser, str]] = {}

    def add_command(name: str, summary: str,
                    **kwargs) -> argparse.ArgumentParser:
        command_parser = subparsers.add_parser(name, help=summary, **kwargs)
        commands[name] = (command_parser, summary)
        return command_parser

    # Add the 'apply' subparser
    apply_parser = add_command(
        'apply', 'Apply all plaster files to the sources in brave-core.')
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
    check_parser = add_command(
        'check', 'Check that plaster files are applied to sources.')
    check_parser.add_argument('filepaths',
                              nargs='*',
                              help='Filepaths to check')
    check_parser.set_defaults(func=check)

    # Our custom `--help` renderer.
    parser.add_argument('-h', '--help', action=Help, commands=commands)

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
