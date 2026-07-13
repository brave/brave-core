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

    # One-line summary shown in the `Rewriters` help index.
    SUMMARY: ClassVar[str] = ''

    # Full Markdown help shown by `plaster --help <NAME>`. Falls back to the
    # docstring.
    HELP: ClassVar[str] = ''

    @abc.abstractmethod
    def apply(self, contents: str) -> tuple[str, int]:
        """Transform `contents`, returning (new_contents, num_changes)."""

    @classmethod
    @abc.abstractmethod
    def parse(cls, body: object, *, description: str) -> Rewriter:
        """Build a rewriter from its YAML body, validating its fields."""

    @classmethod
    def help_text(cls) -> str:
        """The full, user-facing help for this rewriter as dedented Markdown."""
        return textwrap.dedent(cls.HELP or cls.__doc__ or '').strip('\n')


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

    def apply(self, contents: str) -> tuple[str, int]:
        """Apply the composed rewriter, returning (new_contents, changes)."""
        return self.rewriter.apply(contents)

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
    def from_yaml(contents: str) -> list[Substitution]:
        """Parse all substitutions from the contents of a YAML plaster file.

        YAML plasters use a top-level `substitutions:` list.
        """
        data = yaml.load(contents, Loader=Substitution._NoDupSafeLoader)
        if data is None:
            data = {}
        if not isinstance(data, dict):
            raise ValueError('Plaster YAML must be a mapping at the top level')
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
        return [Substitution._from_item(entry) for entry in raw]

    @staticmethod
    def _from_item(data: object) -> Substitution:
        """Validate one `substitutions:` entry and build its rewriter.

        An entry names a rewriter when it carries one of `_REWRITERS` as a key.
        `description` and the optional `count` are item-level for every form.

        Raises:
            ValueError: on a malformed entry (missing/typo'd keys, mixed
                forms, or invalid rewriter-specific fields).
        """
        if not isinstance(data, dict):
            raise ValueError(f'substitution entry must be a mapping, got '
                             f'{type(data).__name__}')
        keys = set(data)

        description = data.get('description')
        if not isinstance(description, str):
            raise ValueError('No description specified for substitution entry')

        expected_count = Substitution._parse_count(data, description)

        rewriter_keys = sorted(keys & _REWRITERS.keys())
        if len(rewriter_keys) > 1:
            raise ValueError(
                f'Only one rewriter allowed per entry, got '
                f'{", ".join(rewriter_keys)} (in "{description}")')
        # TODO(brave.dev/bug/56854): the bare regex form (regex fields placed
        # directly on the item, without a `regex:` key) is deprecated. Once all
        # plasters are migrated, delete this mixing check -- with no bare form
        # there is nothing to mix.
        if rewriter_keys and (keys & Regex._FIELD_KEYS):
            raise ValueError(f'Cannot mix the "{rewriter_keys[0]}" rewriter '
                             f'with bare regex fields (in "{description}")')

        if rewriter_keys:
            name = rewriter_keys[0]
            unknown = sorted(keys - {'description', 'count', name})
            if unknown:
                raise ValueError(f'Unrecognised key(s) for the "{name}" '
                                 f'rewriter: '
                                 f'{", ".join(repr(k) for k in unknown)} '
                                 f'(in "{description}")')
            rewriter = _REWRITERS[name].parse(data[name],
                                              description=description)
            return Substitution(description=description,
                                expected_count=expected_count,
                                rewriter=rewriter)

        # No rewriter key: tolerated bare regex (the legacy form).
        #
        # TODO(brave.dev/bug/56854): the bare regex form (regex fields placed
        # directly on the item, without a `regex:` key) is deprecated. Once all
        # plasters are migrated, delete this whole block and require a rewriter
        # key above, so an entry with no rewriter key becomes an error.
        unknown = sorted(keys - ({'description', 'count'} | Regex._FIELD_KEYS))
        if unknown:
            # Every rewriter body is a mapping, so an unknown key carrying one
            # is almost certainly an attempt to use a rewriter that is not
            # registered -- point the user at the ones that are.
            bad_rewriters = [k for k in unknown if isinstance(data[k], dict)]
            if bad_rewriters:
                available = ', '.join(sorted(_REWRITERS)) or '(none)'
                raise ValueError(
                    f'Unknown rewriter(s): '
                    f'{", ".join(repr(k) for k in bad_rewriters)}; available '
                    f'rewriters: {available} (in "{description}")')
            raise ValueError('Unrecognised substitution key(s): '
                             f'{", ".join(repr(k) for k in unknown)}')
        # Compose a `regex:` body from the bare fields and hand it to Regex.
        fields = {k: data[k] for k in keys & Regex._FIELD_KEYS}
        rewriter = Regex.parse(fields, description=description)
        return Substitution(description=description,
                            expected_count=expected_count,
                            rewriter=rewriter)

    @staticmethod
    def _parse_count(data: dict[str, object], description: str) -> int:
        expected_count = data.get('count', 1)
        # bool is a subclass of int; reject it explicitly.
        if (not isinstance(expected_count, int)
                or isinstance(expected_count, bool)):
            raise ValueError(f'count must be an integer (in "{description}")')
        return expected_count


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

    # The pattern/replacement fields, valid either bare on the item or nested
    # under a `regex:` op key.
    #
    # TODO(brave.dev/bug/56854): the bare placement is deprecated; this set
    # stays for the `regex:` form, but once all plasters are migrated its bare
    # use in `Substitution._from_item` should go.
    _FIELD_KEYS = frozenset(('pattern', 're_pattern', 'replace', 're_flags'))

    def __init__(self, *, re_pattern: str, replace: str, re_flags: int = 0):
        self._re_pattern = re_pattern
        self._replace = replace
        self._re_flags = re_flags

    def apply(self, contents: str) -> tuple[str, int]:
        # `count=0` is provided here so all substitutions are applied, and then
        # the number of them gets validated by the callers.
        return re.subn(self._re_pattern,
                       self._replace,
                       contents,
                       flags=self._re_flags,
                       count=0)

    @classmethod
    def parse(cls, body: object, *, description: str) -> Regex:
        """Build from a regex field mapping (a `regex:` body or bare keys)."""
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


# A set with all the rewriters available in plaster.
_REWRITERS: MappingProxyType[str, type[Rewriter]] = MappingProxyType(
    {rewriter.NAME: rewriter
     for rewriter in (Regex, )})


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
            substitutions = Substitution.from_yaml(info.plaster_contents)
        else:
            raise ValueError(f'Unsupported plaster file extension: {suffix}')
        contents = repository.chromium.read_file(info.source)
        errors = []

        try:
            for substitution in substitutions:
                contents, num_changes = substitution.apply(contents)

                # count == 0 means "one or more matches", but zero matches still
                # result in a failure.
                if substitution.expected_count == 0:
                    if num_changes == 0:
                        errors.append(
                            'Expected at least one match but found none in '
                            f'{self.path}')
                elif num_changes != substitution.expected_count:
                    errors.append(
                        f'Unexpected number of matches ({num_changes} vs '
                        f'{substitution.expected_count}) in {self.path}')

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


def _template_args_match(spec: dict) -> dict:
    """schema validator: a matcher's template uses exactly its declared args.

    Returns the spec unchanged on success or raises schema.SchemaError if the
    template references a placeholder that is not a declared arg, or declares
    an arg the template never uses, catching any typos.
    """
    placeholders = {
        name
        for _, name, _, _ in string.Formatter().parse(spec['template']) if name
    }
    declared = set(spec['args'])
    undeclared = sorted(placeholders - declared)
    if undeclared:
        raise schema.SchemaError(
            f'template uses undeclared placeholder(s): {", ".join(undeclared)}'
        )
    unused = sorted(declared - placeholders)
    if unused:
        raise schema.SchemaError(
            f'declared arg(s) never used in template: {", ".join(unused)}')
    return spec


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

# A matcher op: a templated ast-grep query plus its result shape.
_MATCHER_SCHEMA = schema.And(
    {
        'args': [str],
        'template': _NON_EMPTY_STR,
        'result': _RESULT_SCHEMA,
    }, _template_args_match)

# A rewriter op: locates nodes through a matcher op and edits each via a regex
# substitution.
_REWRITER_SCHEMA = {
    'matcher': _NON_EMPTY_STR,
    'replace': {
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

        Each rewriter must reference a matcher that exists, and must declare the
        same result node as that matcher (it replaces the node the matcher
        locates, so the two must agree).
        """
        for op_id, spec in self._rewriters.items():
            ref = spec['matcher']
            if ref not in self._matchers:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} references unknown '
                    f'matcher {ref!r}')
            matcher_node = self._matchers[ref]['result']['node']
            if spec['result']['node'] != matcher_node:
                raise RewritersSchemaError(
                    f'{self._source}: rewriter {op_id!r} result node '
                    f'{spec["result"]["node"]!r} does not match matcher '
                    f'{ref!r} node {matcher_node!r}')


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
    contents. `apply` runs one rewriter op (by id) over the current contents,
    mutating them in place and returning how many places changed; call it
    repeatedly to accumulate edits. Op-specific conveniences (which op id and
    which adjacent tokens to consume) belong with the `Rewriter` classes that
    drive this engine, not here.
    """

    def __init__(self, rewriters: RewritersEval, content: str):
        self._rewriters = rewriters
        self._content = content

    @property
    def content(self) -> str:
        """The current file contents, reflecting every applied rewrite."""
        return self._content

    def apply(self,
              op_id: str,
              args: dict[str, str],
              *,
              consume_before: str = '',
              consume_after: str = '') -> int:
        """Run rewriter `op_id` with `args`, mutate content, return count.

        Locates nodes via the op's matcher template, applies the op's `replace`
        regex substitution (with `args` filled into the replacement template)
        to each matched node, and splices the results back into the held
        contents from the end so earlier byte offsets stay valid. Returns the
        total number of substitutions made.

        `consume_before` / `consume_after` are literals the op assumes
        immediately precede / follow each matched node. They are folded into the
        rewritten span used when the node kind stops short of an adjacent
        token (e.g. the `:` after an access_specifier, or the space before a
        `final` specifier). They are engine details, not part of the rewriters
        spec.
        """
        rewriter = self._rewriters.rewriter(op_id)
        matcher = self._rewriters.matcher(rewriter['matcher'])
        language = self._rewriters.language_of(op_id)
        rule_body = matcher['template'].format(**args)

        matches = run_ast_grep(language=language,
                               rule_body=rule_body,
                               source=self._content)

        pattern = rewriter['replace']['re_pattern']
        replacement = rewriter['replace']['replace'].format(**args)
        before = consume_before.encode('utf-8')
        after = consume_after.encode('utf-8')

        source = self._content.encode('utf-8')
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
        self._content = source.decode('utf-8')
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
