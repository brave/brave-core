#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for is_upstream_flake.py."""

from __future__ import annotations

import sys
import unittest
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

# pylint: disable=wrong-import-position
import is_upstream_flake as flake
from luci_analysis import MAX_STABILITY_BATCH, LuciAnalysis, Source, Stability

BRANCH = Source("checkout", "abc", "refs/branch-heads/8103", 1035, 157)


class TestNames(unittest.TestCase):
    def test_gtest_name(self):
        self.assertEqual(flake.gtest_name("Suite.Test"), "Suite#Test")
        self.assertEqual(
            flake.gtest_name("All/Suite.Test/0"), "Suite#Test/All.0"
        )
        self.assertEqual(flake.gtest_name("Suite.Test/3"), "Suite#Test/3")
        self.assertEqual(flake.gtest_name("Suite#Test"), "Suite#Test")

    def test_matches_test(self):
        prefix = "://chrome/test\\:browser_tests!gtest::"
        self.assertTrue(flake.matches_test(prefix + "S#T", "S#T"))
        self.assertTrue(flake.matches_test(prefix + "S#T/All.0", "S#T"))
        self.assertFalse(flake.matches_test(prefix + "S#TLonger", "S#T"))


class FakeRequest:
    def __init__(self, reply: Any) -> None:
        self._reply = reply

    def send(self) -> dict[str, Any]:
        return self._reply

    def gather(self, field: str) -> list[Any]:
        return self._reply.get(field, [])


class FakeClient(LuciAnalysis):
    """Answers QueryTests, QueryVariants and QueryStability."""

    def __init__(self, test_ids: list[str], variants: list[dict[str, Any]]):
        super().__init__()
        self.test_ids = test_ids
        self.variant_entries = variants
        self.stability_bodies: list[dict[str, Any]] = []
        # QueryStats groups, and the test IDs they were asked for.
        self.groups: list[dict[str, Any]] = []
        self.stats_asked: list[str] = []

    def tests_matching(self, _substring: str) -> list[str]:
        return self.test_ids

    def paged_request(  # type: ignore[override]
        self, _service: str, method: str, **body: Any
    ):
        if method == "QueryStats":
            self.stats_asked.append(body["testId"])
            return FakeRequest({"groups": self.groups})
        return FakeRequest({"variants": self.variant_entries})

    def request(  # type: ignore[override]
        self, _service: str, _method: str, **body: Any
    ):
        self.stability_bodies.append(body)
        return FakeRequest(
            {
                "criteria": {"flakeRate": {"flakeThreshold": 3}},
                "testVariants": [
                    {
                        "variantHash": tv["variantHash"],
                        "flakeRate": {"isMet": True},
                    }
                    for tv in body["testVariants"]
                ],
            }
        )


class TestQuery(unittest.TestCase):
    def test_filters_and_sources(self):
        prefix = "://chrome/test\\:browser_tests!gtest::"
        client = FakeClient(
            [prefix + "S#T", prefix + "S#TOther"],
            [
                {
                    "variantHash": "a",
                    "variant": {"def": {"builder": "win-rel"}},
                },
                {
                    "variantHash": "b",
                    "variant": {"def": {"builder": "linux-rel"}},
                },
            ],
        )
        results, criteria = flake.query(
            client, "S#T", flake.Filters(builder="^WIN"), BRANCH
        )
        self.assertEqual(criteria, {"flakeRate": {"flakeThreshold": 3}})
        self.assertEqual([r.variant["builder"] for r in results], ["win-rel"])
        self.assertTrue(results[0].confirmed)
        (sent,) = client.stability_bodies[0]["testVariants"]
        self.assertEqual(sent["testId"], prefix + "S#T")
        self.assertEqual(
            sent["sources"]["gitilesCommit"]["ref"], "refs/branch-heads/8103"
        )
        self.assertEqual(sent["sources"]["gitilesCommit"]["position"], "1035")

    def test_batches(self):
        client = FakeClient(
            ["://x!gtest::S#T"],
            [
                {"variantHash": str(i), "variant": {"def": {}}}
                for i in range(MAX_STABILITY_BATCH + 1)
            ],
        )
        results, _ = flake.query(client, "S#T", flake.Filters(), BRANCH)
        self.assertEqual(len(results), MAX_STABILITY_BATCH + 1)
        self.assertEqual(len(client.stability_bodies), 2)


CRITERIA = {"flakeRate": {"flakeThreshold": 3, "flakeThreshold1wd": 1}}


def result(test_id: str, variant_hash: str, **definition: str) -> Stability:
    return Stability(test_id, definition, {"variantHash": variant_hash})


class TestPool(unittest.TestCase):
    def setUp(self):
        self.client = FakeClient([], [])
        today = datetime.now(timezone.utc).strftime("%Y-%m-%dT00:00:00Z")
        self.client.groups = [
            {
                "partitionTime": today,
                "variantHash": h,
                "verdictCounts": {"flaky": "2"},
            }
            for h in ("w10", "w11", "linux")
        ]

    def test_pools_ci_bots_by_bot_os(self):
        results = [
            result("t", "w10", bucket="ci", builder="Win10", os="Windows-10"),
            result("t", "w11", bucket="ci", builder="Win11", os="Windows-11"),
            result("t", "linux", bucket="ci", builder="LT", os="Ubuntu-22.04"),
        ]

        pools = flake.pool(self.client, results, CRITERIA)

        self.assertEqual(
            [
                (p.os_family, p.builders, p.flakes.flaky, p.flakes.met)
                for p in pools
            ],
            [
                ("Ubuntu", ("ci/LT",), 2, False),
                ("Windows", ("ci/Win10", "ci/Win11"), 4, True),
            ],
        )

    def test_trybots_are_not_pooled(self):
        results = [result("t", "w10", builder="win-rel", os="Windows-10")]

        self.assertEqual(flake.pool(self.client, results, CRITERIA), [])
        self.assertEqual(self.client.stats_asked, [])

    def test_each_test_is_asked_about_once(self):
        results = [
            result("t", "w10", bucket="ci", builder="Win10", os="Windows-10"),
            result("t", "linux", bucket="ci", builder="LT", os="Ubuntu-22.04"),
        ]

        flake.pool(self.client, results, CRITERIA)

        self.assertEqual(self.client.stats_asked, ["t"])


if __name__ == "__main__":
    unittest.main()
