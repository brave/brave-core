#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
from dataclasses import dataclass, field
import logging
from pathlib import Path, PurePath
import os
import re
import sys
import tomllib

from terminal import console, terminal
from incendiary_error_handler import IncendiaryErrorHandler
import repository
from repository import Repository

# The path to the directory containing overlay files in brave-core.
OVERLAY_FILES_PATH = repository.BRAVE_CORE_PATH / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = Path('patches/')


@dataclass
class PlasterFile:
    """ Class representing an overlay file.
    This class is used to apply overlay files to sources in other repositories.
    """

    # The path to the overlay file. This path is relative to
    # BRAVE_CORE_PATH.
    path: Path

    # The source file path. This path is relative to the source's own
    # repository.
    source: Path = field(init=False)

    def __post_init__(self):
        # TODO(https://github.com/brave/brave-browser/issues/45052): For now
        # there's only support for `src`, but eventually there's going to be
        # the need to support other repositories as well.
        object.__setattr__(
            self, 'source',
            self.path.relative_to(OVERLAY_FILES_PATH).with_suffix(''))

        # This is made as a path validation, as the path has to be under brave
        # to have its own path used as a reference.
        self.path = self.path.relative_to(repository.BRAVE_CORE_PATH)

    @property
    def patchfile(self) -> Path:
        """ Returns the path to the patch file in brave-core.
        """
        return PATCHES_PATH / f'{str(self.source).replace("/", "-")}.patch'

    @classmethod
    def find_all(cls) -> list["PlasterFile"]:
        """ Finds all overlay files in the overlay directory.
        Returns:
            A list of PlasterFile objects.
        """
        overlay_files = []

        for root, _, files in os.walk(OVERLAY_FILES_PATH):
            for file in files:
                if file.endswith('.toml'):
                    overlay_files.append(cls(Path(root) / file))

        return overlay_files

    def apply(self):
        overlay = tomllib.loads(self.path.read_text())
        contents = repository.chromium.read_file(self.source)

        for substitution in overlay.get('substitution', []):
            pattern = substitution.get('re_pattern', '')
            replace = substitution.get('replace', '')
            count = substitution.get('count', 0)
            flags = substitution.get('re_flags', '')

            re_flags = 0
            if 'IGNORECASE' in flags:
                re_flags |= re.IGNORECASE
            if 'MULTILINE' in flags:
                re_flags |= re.MULTILINE
            if 'DOTALL' in flags:
                re_flags |= re.DOTALL
            if 'VERBOSE' in flags:
                re_flags |= re.VERBOSE

            contents, num_changes = re.subn(pattern,
                                            replace,
                                            contents,
                                            flags=re_flags,
                                            count=count)

            if num_changes == 0:
                raise ValueError(
                    f'No matches found for pattern {pattern} in {self.source}')

        def save_if_has_changes(path: Path, content: str):
            """Saves the file content if it has changed.
            """
            if not path.parent.exists():
                path.parent.mkdir(parents=True, exist_ok=True)
            if path.exists() and path.read_text(encoding='utf-8') == content:
                return
            logging.debug('Saving: %s', path)
            path.write_text(content, encoding='utf-8')

        save_if_has_changes(Path(repository.chromium.from_brave(self.source)),
                            contents)

        patch_content = (repository.chromium.run_git('diff',
                                                     '--src-prefix=a/',
                                                     '--dst-prefix=b/',
                                                     '--default-prefix',
                                                     '--full-index',
                                                     self.source,
                                                     no_trim=True))
        save_if_has_changes(Path(self.patchfile), patch_content)


def main():
    parser = argparse.ArgumentParser(
        description=
        '🩹 A tool to generate patches into Chromium using overlay files.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging')
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    with terminal.with_status('Applying overlay files'):
        overlay_files = PlasterFile.find_all()
        for overlay_file in overlay_files:
            console.log(f'Applying overlay file: {overlay_file.path}')
            overlay_file.apply()

    return 0


if __name__ == '__main__':
    sys.exit(main())
