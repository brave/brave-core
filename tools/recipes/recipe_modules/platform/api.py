# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `platform` module API: mockable host OS identification.

Modules branch on `api.platform.is_win` (etc.) instead of `sys.platform` /
`platform.system()` directly, so a `GenTests` case can simulate any OS with
`api.platform.name('mac')`. In production the name is derived from the real
host; in test mode it comes from the run's `TestContext`.
"""

from __future__ import annotations

import sys

from recipe_api import RecipeApi

# Canonical short platform names, matching recipes_py.
_VALID = ('linux', 'mac', 'win')


def _host_name() -> str:
    if sys.platform == 'win32':
        return 'win'
    if sys.platform == 'darwin':
        return 'mac'
    return 'linux'


class PlatformApi(RecipeApi):
    """The host platform as a short name plus convenience predicates."""

    def __init__(self) -> None:
        super().__init__()
        # Default to the real host; refined in initialise() once the engine has
        # seeded `_test`. Accessors below just read `self._name`. No per-call
        # test-mode branching (mirrors recipes_py's PlatformApi).
        self._name = _host_name()

    def initialise(self) -> None:
        if self._test is not None:
            self._name = self._test.platform
        assert self._name in _VALID, (
            f'unknown simulated platform {self._name!r}; use one of {_VALID}')

    @property
    def name(self) -> str:
        """`'linux'`, `'mac'`, or `'win'`."""
        return self._name

    @property
    def is_win(self) -> bool:
        return self.name == 'win'

    @property
    def is_mac(self) -> bool:
        return self.name == 'mac'

    @property
    def is_linux(self) -> bool:
        return self.name == 'linux'
