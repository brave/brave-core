#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
from dataclasses import dataclass, field
import importlib.util
import logging
from pathlib import Path, PurePath
import os
import re
import sys
from typing import Optional

from terminal import console, terminal
from incendiary_error_handler import IncendiaryErrorHandler
from repository import Repository, BRAVE_CORE_PATH, CHROMIUM_SRC_PATH
import cxx_templates

# The path to the directory containing overlay files in brave-core.
OVERLAY_FILES_PATH = BRAVE_CORE_PATH / 'rewrite'

# The path to the directory where patch files are stored in brave-core.
PATCHES_PATH = PurePath('patches/')

# TODO(https://github.com/brave/brave-browser/issues/45061): These functions
# and templates must be put on their own modules that get loaded based on the
# overlay file's language.

spec = importlib.util.spec_from_file_location(
    "add_header_tools",
    os.path.join(CHROMIUM_SRC_PATH, "tools", "add_header.py"))
add_header_tools = importlib.util.module_from_spec(spec)
sys.modules["add_header_tools"] = add_header_tools
spec.loader.exec_module(add_header_tools)


def AddHeaderToSource(filename: str,
                      source: str,
                      decorated_name: str,
                      remove=False) -> str:
    """ Adds a header to the source file.

    This function adds a header inclusion to another file using Chromium's
    add_header script.
    """
    lines = source.splitlines()
    begin, end = add_header_tools.FindIncludes(lines)

    if begin < 0:
        raise ValueError(f'Could not find the include section in {filename}.')

    includes = add_header_tools.ParseIncludes(lines[begin:end])
    if not includes:
        raise ValueError(f'Could not parse the include section in {filename}.')

    if remove:
        for i in includes:
            if decorated_name == i.decorated_name:
                includes.remove(i)
                break

            raise ValueError(
                f'Could not find {decorated_name} in the include section '
                f'in {filename}.')
    else:
        if decorated_name in [i.decorated_name for i in includes]:
            raise ValueError(
                f'{decorated_name} already exists in the include section '
                f'in {filename}.')

        includes.append(
            add_header_tools.Include(decorated_name, 'include', [], None))

    add_header_tools.MarkPrimaryInclude(includes, filename)

    lines[begin:end] = add_header_tools.SerializeIncludes(includes)
    lines.append('')
    return '\n'.join(lines)


@dataclass
class OverlayFile:
    """ Class representing an overlay file.

    This class is used to apply overlay files to sources in other repositories.
    """

    # The path to the overlay file. This path is relative to
    # BRAVE_CORE_PATH.
    path: PurePath

    # The source file path. This path is relative to the source's own
    # repository.
    source: PurePath = field(init=False)

    def __post_init__(self):
        # TODO(https://github.com/brave/brave-browser/issues/45052): For now
        # there's only support for `src`, but eventually there's going to be
        # the need to support other repositories as well.
        object.__setattr__(
            self, 'source',
            self.path.relative_to(OVERLAY_FILES_PATH).with_suffix(''))

        # This is made as a path validation, as the path has to be under brave
        # to have its own path used as a reference.
        self.path = self.path.relative_to(BRAVE_CORE_PATH)

    @property
    def patchfile(self) -> PurePath:
        """ Returns the path to the patch file in brave-core.
        """
        return PATCHES_PATH / f'{str(self.source).replace("/", "-")}.patch'

    @classmethod
    def find_all(cls) -> list["OverlayFile"]:
        """ Finds all overlay files in the overlay directory.

        Returns:
            A list of OverlayFile objects.
        """
        overlay_files = []

        for root, _, files in os.walk(OVERLAY_FILES_PATH):
            for file in files:
                if file.endswith('.overlay'):
                    overlay_files.append(cls(PurePath(root) / file))

        return overlay_files

    def apply(self):
        """ Applies the overlay file to the source file.

        This method loads an overlay file, executes it to get the
        plasters, and applies them to the source file. The result is
        written to the source file, and a patch file is created in the
        patches directory.
        """

        class Template:
            """ A helper class to load a template into an overlay file.

            This class loads a template from its module into the overlay file
            and provides methods to format the template's pattern and
            replacement strings.
            """

            def __init__(self, name):
                # Using a shallow copy since we only modify top-level keys.
                self.template = cxx_templates.TEMPLATES[name].copy()

            def with_args(self, **kwargs):
                self.template["pattern"] = self.template["pattern"].format(
                    **kwargs)
                return self

            def replacement(self, **kwargs):
                self.template["replacement"] = self.template[
                    "replacement"].format(**kwargs)
                return self

            def serialise(self):
                return self.template

        overlay_globals = {
            'Template': Template,
            'escape': re.escape,  # Provide the escape function
        }

        with open(self.path, 'r', encoding='utf-8') as f:
            exec(f.read(), overlay_globals)

        contents = Repository.chromium().read_file(self.source)

        plasters = overlay_globals.get('plasters', [])
        for change in plasters:
            # For now these ops are here for basic experimentation for tasks
            # that in rare cases may be complex enough to handle with regexes.
            if 'op' in change:
                if change['op'] == 'AddHeader':
                    contents = AddHeaderToSource(str(self.source), contents,
                                                 change['value'])
            else:
                pattern = change['pattern']
                replacement = change.get('replacement', '')
                flags = change.get('flags', [])
                count = change.get('count', 0)

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
                                                replacement,
                                                contents,
                                                flags=re_flags,
                                                count=count)

                if num_changes == 0:
                    raise ValueError(
                        f'No matches found for pattern {pattern} in {self.source}'
                    )

        def save_if_has_changes(path: Path, content: str):
            """Saves the file content if it has changed.
            """
            if path.exists():
                with path.open('r', encoding='utf-8') as f:
                    if f.read() == content:
                        return
            logging.debug('Saving: %s', path)
            with path.open('w', encoding='utf-8') as f:
                f.write(content)

        save_if_has_changes(
            Path(Repository.chromium().from_brave(self.source)), contents)

        patch_content = (Repository.chromium().run_git('diff',
                                                       '--src-prefix=a/',
                                                       '--dst-prefix=b/',
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
        overlay_files = OverlayFile.find_all()
        for overlay_file in overlay_files:
            console.log(f'Applying overlay file: {overlay_file.path}')
            overlay_file.apply()

    return 0


if __name__ == '__main__':
    sys.exit(main())
