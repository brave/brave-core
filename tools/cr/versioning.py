# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from dataclasses import dataclass
from functools import total_ordering
import json

import repository

# The path to the package.json file in brave-core
PACKAGE_FILE = 'package.json'

# The link to the Chromium source code.
GOOGLESOURCE_LINK = 'https://chromium.googlesource.com/chromium/src'

# Link with the log of changes between two versions.
GOOGLESOURCE_LOG_LINK = (
    f'{GOOGLESOURCE_LINK}'
    '/+log/{from_version}..{to_version}?pretty=fuller&n=10000')

# The file in Chromium with the version written in it.
CHROMIUM_VERSION_FILE = 'chrome/VERSION'


def load_package_file(branch):
    """Retrieves the json content of package.json for a given revision

  Args:
    branch:
      A branch or hash to load the file from.
  """
    package = repository.brave.read_file(PACKAGE_FILE, commit=branch)
    return json.loads(package)


def read_chromium_version_file():
    """Retrieves the Chromium version from the VERSION file.

    This function reads directly from git, as VERSION gets patched during
    `apply_patches`.
    """
    version_parts = {}
    file = repository.chromium.read_file(CHROMIUM_VERSION_FILE)
    for line in file.splitlines():
        key, value = line.strip().split('=')
        version_parts[key] = value
    return Version('{MAJOR}.{MINOR}.{BUILD}.{PATCH}'.format(**version_parts))


def get_uplift_branch_name_from_package() -> str:
    """Generates the name of the uplift branch from `package.json`.
    """
    version = load_package_file('HEAD')['version'].split('.')
    return f'{version[0]}.{version[1]}.x'


@total_ordering
@dataclass(frozen=True)
class Version:
    """A class to hold the version information.
    """

    # The version data in the format of 'MAJOR.MINOR.BUILD.PATCH'
    value: str

    def __post_init__(self):
        if len(self.parts) != 4:
            raise ValueError(
                'Version required format: MAJOR.MINOR.BUILD.PATCH. '
                f'version={self.value}')

    def __str__(self):
        return self.value

    @property
    def parts(self) -> tuple[int, int, int, int]:
        """The version parts as integers."""
        return tuple(map(int, self.value.split('.')))

    @property
    def major(self) -> int:
        """The major version part.
        """
        return self.parts[0]

    @classmethod
    def from_value(cls, value: str) -> "Version" | None:
        """Returns a Version only when the input has a valid format."""
        try:
            return cls(value)
        except ValueError:
            return None

    @classmethod
    def from_git(cls, branch: str) -> "Version":
        """Retrieves the version from the git repository.
        """
        return cls(
            load_package_file(branch).get('config').get('projects').get(
                'chrome').get('tag'))

    @classmethod
    def get_latest_googlesource_tag_version(cls,
                                            major: int | None = None
                                            ) -> "Version" | None:
        args = [
            'ls-remote', '--tags', '--refs', '--sort=-v:refname',
            GOOGLESOURCE_LINK
        ]
        if major is not None:
            args.append(f'refs/tags/{major}.*')
        # It doesn't matter which repo we are in when doing this command.
        output = repository.brave.run_git(*args)

        for line in output.splitlines():
            line = line.strip()
            if not line:
                continue

            git_tag = line.rsplit('/', 1)[-1].strip()
            version = Version.from_value(git_tag)
            if version is not None:
                return version

        return None

    def __eq__(self, other):
        if not isinstance(other, Version):
            return NotImplemented
        return self.parts == other.parts

    def __lt__(self, other):
        if not isinstance(other, Version):
            return NotImplemented
        return self.parts < other.parts

    def get_googlesource_diff_link(self, from_version: "Version") -> str:
        """Generates a link to the diff of the upgrade.
        """
        return GOOGLESOURCE_LOG_LINK.format(from_version=from_version,
                                            to_version=self)
