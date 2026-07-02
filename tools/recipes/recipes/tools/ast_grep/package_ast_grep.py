# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

from recipe_properties import Property

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = [
    'path', 'step', 'depot_tools', 'chromium_checkout', 'brave_core_shallow'
]


@dataclass(frozen=True)
class InputProperties:
    chromium_ref: str
    git_cache: str | None = Property(default=None, from_environ='GIT_CACHE')


PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref,
                                          git_cache=properties.git_cache)

    brave_root = api.brave_core_shallow.deploy('third_party/ast-grep')

    vpython3 = api.depot_tools.vpython3()
    api.step('package ast-grep', [
        vpython3,
        brave_root / 'third_party/ast-grep/package_ast_grep.py',
        '--clean',
        '--out-dir',
        api.path.out,
    ])
