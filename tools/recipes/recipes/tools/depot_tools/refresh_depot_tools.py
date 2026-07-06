# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Clone Chromium's depot_tools and mirror it to brave-experiments."""

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'depot_tools', 'brave_core_shallow']


def RunSteps(api: RecipeScriptApi, _: object) -> None:
    brave_root = api.brave_core_shallow.deploy('tools/cr')

    vpython3 = api.depot_tools.vpython3()
    api.step('refresh depot_tools', [
        vpython3,
        brave_root / 'tools/cr/refresh_depot_tools.py',
        '--clone-dir',
        api.path.workspace,
    ])
