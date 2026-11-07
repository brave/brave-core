# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `claude_cli`: simulate a run, and check what it was asked.

    api.claude_cli.failed('autolift')
    api.claude_cli.prompt_matches('autolift', r'Lift cr159\\b.*')

The module's own example records the full command once, so a recipe's tests
only check what the recipe decides, such as the prompt, rather than restating
how `claude` is invoked.
"""

from __future__ import annotations

import re

from recipe_test_api import RecipeTestApi, TestData


class ClaudeCliTestApi(RecipeTestApi):
    """Simulate and check a `claude_cli.run` step."""

    def failed(self, name: str) -> TestData:
        """Seed the run named *name* to fail, as Claude erroring out does."""
        return self.step_data(name, retcode=1)

    def prompt_matches(self, name: str, pattern: str) -> TestData:
        """Check that the prompt of the run named *name* fully matches
        *pattern*, across lines."""
        return self.post_process(self._check_prompt, name, pattern)

    @staticmethod
    def _check_prompt(check, step_odict, name: str, pattern: str) -> None:
        cmd = step_odict[name]['cmd']
        prompt = cmd[cmd.index('--print') + 1]
        check(
            f'prompt of {name} matches {pattern!r}',
            re.fullmatch(pattern, prompt, re.DOTALL) is not None,
        )
