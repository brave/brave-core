# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The legacy `Result` form of a recipe's result.

It is what `engine.py --output-result-json` writes, and what simulation tests
record as their `$result`.
"""

from __future__ import annotations

from PB.recipe_engine import result as result_pb


def to_legacy_result(
    result: result_pb.RawResult | None,
) -> result_pb.Result | None:
    """Convert from result_pb.RawResult to result_pb.Result."""
    if not result:
        return None
    legacy_result = result_pb.Result()
    if result.status != result_pb.SUCCESS:
        legacy_result.failure.human_reason = result.summary_markdown
        if result.status != result_pb.INFRA_FAILURE:
            legacy_result.failure.failure.SetInParent()
    return legacy_result
