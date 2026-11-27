# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A gtest in a builder's targets spec."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class GTest:
    """A gtest a builder runs, as its targets spec lists it."""

    # What its step and its results are named after.
    name: str
    # The binary it runs.
    test: str
    # Its arguments, filter files included.
    args: tuple[str, ...] = ()

    @classmethod
    def from_json(cls, data: dict[str, Any]) -> GTest:
        return cls(
            name=data['name'],
            test=data['test'],
            args=tuple(data.get('args', ())),
        )
