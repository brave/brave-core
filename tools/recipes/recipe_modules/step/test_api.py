# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for the `step` module: seed a step's simulated result.

`api.step_data(...)` on the root api is the usual entry point; this exposes the
same thing as `api.step.data(...)` for symmetry with recipes_py, so a step's
retcode/output can be seeded next to other `api.step.*` fragments.
"""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class StepTestApi(RecipeTestApi):
    """Seed the simulated result (retcode / captured output) of a step."""

    def data(self,
             name: str,
             retcode: int = 0,
             stdout: str | None = None,
             stderr: str | None = None) -> TestData:
        return self.step_data(name,
                              retcode=retcode,
                              stdout=stdout,
                              stderr=stderr)
