# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `file`: seed a `fileutil.py` step's simulated result.

Each helper is a fragment for `api.step_data`:

    api.step_data('read greeting', api.file.read_text('hello\\n'))
    api.step_data('read greeting', api.file.errno('ENOENT'))

Most of the time nothing needs seeding at all: `api.file` methods take a
`test_data` argument that supplies the step's default simulated result, and an
unseeded step reports success. These helpers are for the cases a test wants to
steer -- notably `errno`, to exercise what a recipe does when an operation
fails.

Void operations (`copy`, `write_text`, `move`, `chmod`, `remove`, `rmtree`,
`rmcontents`, `rmglob`, `ensure_directory`, `symlink`, `truncate`,
`flatten_single_directories`) have nothing to return, so `errno` is the only
helper that applies to them.
"""

from __future__ import annotations

import json
from typing import Any, Sequence

from google.protobuf.message import Message

from recipe_test_api import RecipeTestApi, StepTestData


class FileTestApi(RecipeTestApi):
    """Seed the simulated result of a `fileutil.py` step."""

    def errno(self, errno_name: str | None = None) -> StepTestData:
        """Seed the shared `{ok, errno_name, message}` result of any operation.

        With no *errno_name* the operation reports success -- which is what an
        unseeded `file` step defaults to. Pass one (`'ENOENT'`, `'EPERM'`, ...)
        to make the operation raise `file.Error` instead.
        """
        result: dict[str, Any] = {'ok': True}
        if errno_name:
            result = {
                'ok': False,
                'errno_name': errno_name,
                # A real run's message comes from the OS and usually carries
                # more detail than this.
                'message': f'file command encountered system error {errno_name}'
            }
        return self.m.json.output(result)

    def read_raw(self,
                 content: bytes = b'',
                 errno_name: str | None = None) -> StepTestData:
        """Seed the bytes a `read_raw` step returns."""
        return self.m.raw_io.output(content) + self.errno(errno_name)

    def read_text(self,
                  text_content: str = '',
                  errno_name: str | None = None) -> StepTestData:
        """Seed the text a `read_text` step returns."""
        return self.m.raw_io.output_text(text_content) + self.errno(errno_name)

    def read_json(self,
                  json_content: Any = None,
                  errno_name: str | None = None) -> StepTestData:
        """Seed the value a `read_json` step returns."""
        text = json.dumps(json_content, indent=2, sort_keys=True)
        return self.m.raw_io.output_text(text) + self.errno(errno_name)

    def read_proto(self,
                   proto_msg: Message,
                   errno_name: str | None = None) -> StepTestData:
        """Seed the message a `read_proto` step returns."""
        return self.m.proto.output(proto_msg) + self.errno(errno_name)

    def listdir(self,
                paths: Sequence[str] = (),
                errno_name: str | None = None) -> StepTestData:
        """Seed the entries a `listdir` step finds, relative to its source."""
        return (self.m.raw_io.stream_output_text('\n'.join(
            sorted(str(p) for p in paths))) + self.errno(errno_name))

    def glob_paths(self,
                   names: Sequence[str] = (),
                   errno_name: str | None = None) -> StepTestData:
        """Seed the paths a `glob_paths` step matches, relative to its source."""
        return (self.m.raw_io.stream_output_text('\n'.join(
            sorted(str(n) for n in names))) + self.errno(errno_name))

    def filesizes(self,
                  sizes: Sequence[int] = (),
                  errno_name: str | None = None) -> StepTestData:
        """Seed the sizes a `filesizes` step reports."""
        return (
            self.m.raw_io.stream_output_text('\n'.join(str(s)
                                                       for s in sizes)) +
            self.errno(errno_name))

    def compute_hash(self,
                     digest: str = '',
                     errno_name: str | None = None) -> StepTestData:
        """Seed the hash a `compute_hash` step reports."""
        return (self.m.raw_io.stream_output_text(digest) +
                self.errno(errno_name))

    def file_hash(self,
                  digest: str = '',
                  errno_name: str | None = None) -> StepTestData:
        """Seed the hash a `file_hash` step reports."""
        return (self.m.raw_io.stream_output_text(digest) +
                self.errno(errno_name))

    def is_executable(self,
                      executable: bool = True,
                      errno_name: str | None = None) -> StepTestData:
        """Seed the answer an `is_executable` step reports."""
        return (self.m.raw_io.stream_output_text(str(executable)) +
                self.errno(errno_name))
