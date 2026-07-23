# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `hello` module API: greets its configured TARGET with its configured tool.
"""

from __future__ import annotations

from PB.recipe_modules.brave.hello.properties import InputProperties
from recipe_api import RecipeApi


class HelloApi(RecipeApi):
    """Greets the configured TARGET, running the configured tool as a step."""

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        # A module's declared PROPERTIES message is injected here by the engine.
        # DEPS are NOT yet available in __init__, so we only stash the value.
        self._target = properties.target or None

    def get_config_defaults(self) -> dict:
        # Thread the per-module property into the config schema: when a target
        # was supplied it becomes the default TARGET, otherwise the schema's own
        # default ('Bob') applies. A per-invocation set_config(TARGET=...) still
        # overrides this.
        defaults = {}
        if self._target is not None:
            defaults['TARGET'] = self._target
        return defaults

    def greet(self) -> None:
        """Greet `self.c.TARGET`, invoking `self.c.tool` as the step command."""
        self.m.step('Greet Admired Individual', [
            self.m.path.workspace / self.c.tool,
            self.c.verb % self.c.TARGET,
        ])
