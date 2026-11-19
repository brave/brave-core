# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `isolate`: enable it, and seed the isolates `mb` writes."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class IsolateTestApi(RecipeTestApi):
    """Seeds what `isolate_tests()` reads."""

    def enabled(self) -> TestData:
        """Opts the builder in, as `archive_isolates` does in infra/config."""
        return self.properties(**{'$isolate': {'enabled': True}})

    @staticmethod
    def isolate_file(target: str) -> dict:
        """The `.isolate` `mb isolate` writes for *target*."""
        return {
            'variables': {
                'command': [
                    'vpython3',
                    '../../testing/test_env.py',
                    f'./{target}',
                ],
                'files': [f'./{target}', '../../testing/test_env.py'],
            }
        }
