# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import re
import json

from typing import List
import components.git_tools as git_tools

from components.perf_test_utils import GetProcessOutput


class ChromiumVersion:
  _version: List[int]

  def __init__(self, v: str) -> None:
    super().__init__()
    assert re.match(r'\d+\.\d+\.\d+\.\d+', v)
    self._version = list(map(int, v.split('.')))

  def to_string(self) -> str:
    return '.'.join(map(str, self._version))

  def major(self) -> int:
    return self._version[0]


class BraveVersion:
  _is_tag: bool
  _git_hash: str
  _last_tag: str
  _revision_number: str
  _commit_date: str
  _chromium_version: ChromiumVersion

  def __init__(self, version_str: str) -> None:
    m = re.match(r'v\d+\.\d+\.\d+', version_str)
    if m is not None:  # Brave tag (v1.62.35)
      revision = f'refs/tags/{version_str}'
      self._is_tag = True
    else:  # could be git hash
      revision = version_str
      self._is_tag = False

    git_tools.EnsureRevision(revision)
    git_hash = git_tools.GetGitHash(revision)
    self._git_hash = git_hash

    content = git_tools.GetFileAtRevision('package.json', git_hash)
    assert content is not None
    package_json = json.loads(content)

    if self.is_tag:
      self._last_tag = version_str
    else:
      self._last_tag = 'v' + package_json['version']

    self._revision_number = git_tools.GetRevisionNumber(git_hash)
    self._commit_date = git_tools.GetCommitDate(git_hash)
    self._chromium_version = ChromiumVersion(
        package_json['config']['projects']['chrome']['tag'])

  @property
  def last_tag(self) -> str:
    return self._last_tag

  @property
  def is_tag(self) -> bool:
    return self._is_tag

  # Returns the revision in a human readable format:
  # the last tag + the current commit
  def to_string(self) -> str:
    if self._is_tag:
      return self._last_tag
    return f'{self._last_tag}+{self._git_hash[:8]}'

  # Returns tag or git sha1 hash
  @property
  def git_revision(self) -> str:
    if self._is_tag:
      return self._last_tag
    return self._git_hash

  @property
  def revision_number(self) -> str:
    return self._revision_number

  @property
  def commit_date(self) -> str:
    return self._commit_date

  @property
  def chromium_version(self) -> ChromiumVersion:
    return self._chromium_version

  # Returns a version like 108.1.48.1
  def combined_version(self) -> str:
    return f'{self._chromium_version.major()}.{self.last_tag[1:]}'
