#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
from dataclasses import dataclass, field
import hashlib
import logging
from pathlib import Path, PurePath
import json
import os
import re
import sys
import tomllib
from typing import Optional

from terminal import console, terminal
from incendiary_error_handler import IncendiaryErrorHandler
import repository
from repository import Repository

# The path to the directory containing plaster files in brave-core.
PLASTER_FILES_PATH = repository.BRAVE_CORE_PATH / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = Path('patches/')

@dataclass
class PathChecksumPair:
    """ A path/checksum pair to check for changes in a file.

    This class is used to store the path to a file and its checksum, which
    provides a way to track changes in the file content.
    """

    # The file path relative to BRAVE_CORE_PATH.
    path: Path

    # Cached checksum of the file content. It will be set to None if the file
    # does not exist.
    checksum: Optional[str] = field(init=False)

    def __post_init__(self):
        """Initialize the PathChecksumPair and calculate the checksum."""
        self.checksum = self.calculate_file_checksum()

    def calculate_file_checksum(self) -> Optional[str]:
        """Calculate the SHA-256 checksum of the file's current content."""
        if not self.path.exists():
            return None
        checksum_generator = hashlib.sha256()
        with self.path.open('rb') as file:
            for chunk in iter(lambda: file.read(4096), b""):
                checksum_generator.update(chunk)
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
            self.path.write_text(new_content, encoding='utf-8')
        self.checksum = new_checksum
        return True


@dataclass
class PatchInfo:
    """ Manages a .patchinfo file contents.

    This class is used to manage the patchfile metadata, known as .patchinfo,
    which are used by our machinery when applying patches to determine if a file
    needs to be updated or not.

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

    This class extends the contents of patchinfo files to include the details
    of the plaster file, including path and checksum, under the "plaster" key.
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

    This class is used to generate a patchinfo file, and with its hash data,
    determine if we should make changes to the source, patch, and patchinfo
    files. These three files are supposed to be changed only when the resulting
    new content is different from what is in disk.

    The new additions to .patchinfo are not known yet to `apply_patches`, but
    this may change in the future.
    """

    # Path to the plaster file, relative to BRAVE_CORE_PATH.
    plaster_file: Path

    # Contents of the plaster file as a string (read during initialization).
    plaster_contents: str = field(init=False)

    # SHA-256 checksum of the plaster file contents.
    plaster_checksum: Optional[str] = field(init=False)

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
        """Initializes the PatchInfo data with checksums and paths."""
        self.plaster_contents = self.plaster_file.read_text(encoding='utf-8')
        self.plaster_checksum = hashlib.sha256(
            self.plaster_contents.encode()).hexdigest()

        # Setup source file (path derived from plaster file, no extension).
        self.source = self.plaster_file.relative_to(
            PLASTER_FILES_PATH).with_suffix('')
        self.source_with_checksum = PathChecksumPair(
            Path(repository.chromium.from_brave(self.source)))

        # Setup patch file (named based on source path, with dashes).
        self.patch = PathChecksumPair(
            PATCHES_PATH / f'{str(self.source).replace("/", "-")}.patch')

        # Setup patchinfo metadata file (same as the patch .patchinfo
        # extension).
        self.patchinfo = PathChecksumPair(
            self.patch.path.with_suffix('.patchinfo'))

        # This is set relative, so it gets validated to be under
        # BRAVE_CORE_PATH.
        self.plaster_file = self.plaster_file.relative_to(
            repository.BRAVE_CORE_PATH)

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
        content = json.dumps({
            'schemaVersion': 1,
            'patchChecksum': self.patch.checksum,
            'appliesTo': [{
                'path': str(self.source),
                'checksum': self.source_with_checksum.checksum,
            }],
            'plaster': {
                'path': str(self.plaster_file),
                'checksum': self.plaster_checksum,
            },
        })
        self.patchinfo.save_if_changed(content)


@dataclass
class PlasterFile:
    """ Class representing an plaster file.
    This class is used to apply plaster files to sources in other repositories.
    """

    # The path to the plaster file. This path is relative to
    # BRAVE_CORE_PATH.
    path: Path

    @classmethod
    def find_all(cls) -> list["PlasterFile"]:
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

    def apply(self, dry_run=False):
        info = PatchInfo(self.path)
        plaster_file = tomllib.loads(info.plaster_contents)
        contents = repository.chromium.read_file(info.source)

        for substitution in plaster_file.get('substitution'):
            description = substitution.get('description')
            re_pattern = substitution.get('re_pattern')
            pattern = substitution.get('pattern')
            replace = substitution.get('replace')
            expected_count = substitution.get('count', 1)
            flags = substitution.get('re_flags', [])

            if description is None:
                raise ValueError(f'No description specified in {info.source}')

            if re_pattern is not None and pattern is not None:
                raise ValueError(
                    f'Please specify either pattern or re_pattern '
                    f' in {info.source}')

            if re_pattern is None:
                if pattern is None:
                    raise ValueError(f'No pattern specified in {info.source}')
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
                        f'Invalid re flag specified: {flag} in {info.source}')

            contents, num_changes = re.subn(
                re_pattern,
                replace,
                contents,
                flags=re_flags,
                # We dont't want to explicitly limit the number of matches here,
                # we want to control what matches using the match pattern and
                # then ensure the output matches only what we expected
                count=0)

            # count == 0 means "replace all matches" and bypass count validation
            if expected_count not in (0, num_changes):
                raise ValueError(
                    f'Unexpected number of matches ({num_changes} vs '
                    f'{expected_count}) in {info.source}')

        has_changed = info.save_source_if_changed(contents, dry_run=dry_run)
        has_changed = info.save_patch_if_changed(
            dry_run=dry_run) or has_changed
        if dry_run:
            if has_changed:
                raise PlasterFileNeedsRegen(
                    f"Plaster file needs to be reapplied: {self.path}")
        else:
            info.save_patchinfo_if_changed()


class PlasterFileNeedsRegen(Exception):
    pass


def apply():
    """Applies all plaster files to brave.
    """
    with terminal.with_status('Applying plaster files'):
        # TODO(https://github.com/brave/brave-browser/issues/46880): Add support
        # to call `apply` with a list of plaster files to apply, the same way
        # `check` does.
        plaster_files = PlasterFile.find_all()
        for plaster_file in plaster_files:
            console.log(f'Applying plaster file: {plaster_file.path}')
            plaster_file.apply()


def check(args):
    """Collects filepaths into a set and logs them (dummy implementation).
    If a path is a patch file, convert it to its expected plaster file name.
    Throws if a file does not match expectations.
    If no filepaths are given, use all plaster files found.
    """
    expected_plaster_files = set()
    if hasattr(args, 'filepaths') and args.filepaths:
        for filepath in args.filepaths:
            filepath = PurePath(filepath).as_posix()
            if filepath.startswith('patches/') and filepath.endswith('.patch'):
                base = filepath[len('patches/'):-len('.patch')]
                plaster_relative = base.replace('-', '/') + '.toml'
                plaster_path = f"rewrite/{plaster_relative}"
                expected_plaster_files.add(plaster_path)
            elif filepath.startswith('rewrite/') and filepath.endswith(
                    '.toml'):
                expected_plaster_files.add(filepath)
            else:
                raise ValueError(f"Unexpected file path: {filepath}")
        plaster_parent = Path(PLASTER_FILES_PATH).parent

        # A set of candidate plaster files.
        candidate_paths = {
            plaster_parent / Path(p)
            for p in expected_plaster_files
        }

        # TODO(https://github.com/brave/brave-browser/issues/46880): For now we
        # discard any plaster file passed in that does not exist. At some point
        # we will need a good answer of what to do once a plaster file is
        # deleted, but for now no check is being implemented to not break
        # git revert on presubmit checks.
        plaster_files = [
            PlasterFile(path) for path in candidate_paths if path.exists()
        ]
    else:
        plaster_files = PlasterFile.find_all()

    has_failure = False
    for plaster_file in plaster_files:
        try:
            logging.debug('Checking plaster file: %s', plaster_file.path)
            plaster_file.apply(dry_run=True)
        except PlasterFileNeedsRegen as e:
            print(e, file=sys.stderr)
            has_failure = True

    if has_failure:
        sys.exit(1)


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
    apply_parser.set_defaults(func=lambda args: apply())

    # Add the 'check' subparser
    check_parser = subparsers.add_parser(
        'check', help='Check plaster files (dummy implementation)')
    check_parser.add_argument('filepaths',
                              nargs='*',
                              help='Filepaths to check')
    check_parser.set_defaults(func=check)

    args = parser.parse_args()

    if hasattr(args, 'infra_mode') and args.infra_mode:
        terminal.set_infra_mode()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    args.func(args)
    return 0


if __name__ == '__main__':
    sys.exit(main())
