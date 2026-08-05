# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Locates the `vpython3` interpreter for tools/cr scripts.

Use `VPYTHON3_PATH` when spawning vpython3 from tools/cr code so callers
work whether or not depot_tools is on the user's PATH.
"""

from pathlib import Path
import platform
import shutil

import repository


def _compute_vpython3_path() -> Path:
    """Resolves the `vpython3` interpreter for use by tools/cr scripts.

    Precedence:
      1. POSIX with `vpython3` on `$PATH` — `Path('vpython3')` so callers
         do a fresh `$PATH` lookup at invocation time.
      2. Windows with `vpython3` on `$PATH` — the absolute path returned
         by `shutil.which`, because git's bundled bash (used to execute
         `!`-prefixed aliases) doesn't reliably resolve `.bat` wrappers
         like `vpython3.bat` via `$PATH`.
      3. Neither — the chromium-bundled `third_party/depot_tools/vpython3`,
         resolved to absolute at module import so later cwd changes (e.g.
         `FakeChromiumRepo.setup()`) don't invalidate it.
    """
    found = shutil.which('vpython3')
    if found is None:
        return (repository.chromium.root / 'third_party' / 'depot_tools' /
                'vpython3').resolve()
    if platform.system() == 'Windows':
        return Path(found)
    return Path('vpython3')


VPYTHON3_PATH: Path = _compute_vpython3_path()


def is_found_in_path_variable() -> bool:
    """Returns True if `VPYTHON3_PATH` is resolved via the shell's `$PATH`.

    A PATH-resolved value is a bare command name like `Path('vpython3')`,
    which has no parent directory (`parent == Path('.')`); shell and
    subprocess machinery look it up via `$PATH` at invocation time. The
    chromium-bundled fallback, by contrast, is an absolute path with a
    real parent directory.
    """
    return VPYTHON3_PATH.parent == Path('.')
