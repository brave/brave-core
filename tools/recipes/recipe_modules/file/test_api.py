# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `file`: seed a `fileutil.py` step's simulated result.

Each helper is merged with `api.step_data(name, ...)` at the call site;
*name* is a required first argument, matching this engine's name-keyed
`step_data`.

Void operations (`copy`, `move`, `chmod`, `remove`, `rmtree`, `rmcontents`,
`rmglob`, `ensure_directory`, `symlink`, `truncate`,
`flatten_single_directories`) need no dedicated helper: they default to
success with nothing to return, so an unseeded step is already a passing
test.
"""

from __future__ import annotations

import json

from recipe_test_api import RecipeTestApi, TestData


class FileTestApi(RecipeTestApi):
    """Seed the simulated `{ok, errno_name, message}` result of a step."""

    def _ok(self, name: str, stdout: str = '') -> TestData:
        return self.step_data(name,
                              stdout=stdout,
                              stderr=json.dumps({
                                  'ok': True,
                                  'errno_name': '',
                                  'message': '',
                              }))

    def read_text(self, name: str, content: str = '') -> TestData:
        """Seed *name* to succeed, returning *content* as the file's text."""
        return self._ok(name, content)

    def read_json(self, name: str, content=None) -> TestData:
        """Seed *name* to succeed, returning *content* as parsed JSON."""
        return self._ok(name, json.dumps(content))

    def listdir(self, name: str, paths: tuple[str, ...] = ()) -> TestData:
        """Seed *name* to succeed, returning *paths* as directory entries."""
        return self._ok(name, '\n'.join(paths))

    def glob_paths(self, name: str, paths: tuple[str, ...] = ()) -> TestData:
        """Seed *name* to succeed, returning *paths* as glob matches."""
        return self._ok(name, '\n'.join(paths))

    def filesizes(self, name: str, sizes: tuple[int, ...] = ()) -> TestData:
        """Seed *name* to succeed, returning *sizes* as file sizes."""
        return self._ok(name, '\n'.join(str(s) for s in sizes))

    def compute_hash(self, name: str, sha256: str = '') -> TestData:
        """Seed *name* to succeed, returning *sha256* as the computed hash."""
        return self._ok(name, sha256)

    def file_hash(self, name: str, sha256: str = '') -> TestData:
        """Seed *name* to succeed, returning *sha256* as the file's hash."""
        return self._ok(name, sha256)

    def is_executable(self, name: str, executable: bool = True) -> TestData:
        """Seed *name* to succeed, returning *executable* as the result."""
        return self._ok(name, str(executable))

    def error(self,
              name: str,
              errno_name: str = 'ENOENT',
              message: str = '') -> TestData:
        """Seed *name* to fail with *errno_name* (and optional *message*)."""
        return self.step_data(name,
                              stderr=json.dumps({
                                  'ok': False,
                                  'errno_name': errno_name,
                                  'message': message or errno_name,
                              }))
