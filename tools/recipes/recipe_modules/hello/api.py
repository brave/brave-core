# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `hello` module API: greets its configured TARGET with its configured tool.
"""

from __future__ import annotations

from recipe_api import RecipeApi


class HelloApi(RecipeApi):
    """Greets the configured TARGET, running the configured tool as a step."""

    def greet(self) -> None:
        """Greet `self.c.TARGET`, invoking `self.c.tool` as the step command."""
        self.m.step('Greet Admired Individual', [
            self.m.path.workspace / self.c.tool,
            self.c.verb % self.c.TARGET,
        ])
