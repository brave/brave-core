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
import sys

import yaml

from terminal import IncendiaryErrorHandler, console, is_verbose, terminal
import repository

# The path to the directory containing plaster files in brave-core.
PLASTER_FILES_PATH = repository.brave.root / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = repository.brave.root / 'patches'

# The plaster file extension.
PLASTER_EXTENSION = '.yaml'

# A particular gitattributes file that is used to ensure we get deterministic
# patch output across platforms and git versions.
PLASTER_GITATTRIBUTES_PATH = Path(__file__).parent / 'plaster_gitattributes'


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


@dataclass(frozen=True)
class Substitution:
    """A single substitution entry parsed from a plaster file.

    `from_yaml` instantiates all the substitutions from a YAML plaster
    file.
    """

    # Human-readable description, used as a prefix in error messages.
    description: str

    # Regex pattern passed verbatim to `re.subn`. When the plaster entry
    # uses the literal `pattern` form, this field holds its `re.escape`'d
    # form.
    re_pattern: str

    # Replacement string passed to `re.subn`.
    replace: str

    # Expected number of substitutions. `0` means "one or more matches";
    # zero matches is still a failure.
    expected_count: int = 1

    # Combined bitmask of `re` flags built from the `re_flags` plaster
    # field.
    re_flags: int = 0

    # Every key accepted on a substitution mapping. Anything else is
    # rejected by `_from_dict` so typos surface immediately instead of
    # being silently ignored.
    _ALLOWED_KEYS = frozenset(('description', 'pattern', 're_pattern',
                               'replace', 'count', 're_flags'))

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
        return [Substitution._from_dict(entry) for entry in raw]

    @staticmethod
    def _from_dict(data: object) -> Substitution:
        """Validate a single substitution mapping and build a Substitution.

        `from_yaml` dispatches through this helper after parsing produces a
        Python `dict` for each entry.

        Raises:
            ValueError: if required fields are missing, a field has the
                wrong type, or `pattern` and `re_pattern` are both set.
        """
        if not isinstance(data, dict):
            raise ValueError(f'substitution entry must be a mapping, got '
                             f'{type(data).__name__}')

        unknown = sorted(set(data.keys()) - Substitution._ALLOWED_KEYS)
        if unknown:
            raise ValueError('Unrecognised substitution key(s): '
                             f'{", ".join(repr(k) for k in unknown)}')

        description = data.get('description')
        if not isinstance(description, str):
            raise ValueError('No description specified for substitution entry')

        pattern = data.get('pattern')
        re_pattern = data.get('re_pattern')
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

        replace = data.get('replace')
        if not isinstance(replace, str):
            raise ValueError(
                f'No replace value specified (in "{description}")')

        expected_count = data.get('count', 1)
        # bool is a subclass of int; reject it explicitly.
        if (not isinstance(expected_count, int)
                or isinstance(expected_count, bool)):
            raise ValueError(f'count must be an integer (in "{description}")')

        flags_raw = data.get('re_flags', [])
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

        return Substitution(
            description=description,
            re_pattern=re_pattern,
            replace=replace,
            expected_count=expected_count,
            re_flags=re_flags,
        )


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
                contents, num_changes = re.subn(
                    substitution.re_pattern,
                    substitution.replace,
                    contents,
                    flags=substitution.re_flags,
                    # We don't want to explicitly limit the number of matches
                    # here, we want to control what matches using the match
                    # pattern and then ensure the output matches only what we
                    # expected.
                    count=0)

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
