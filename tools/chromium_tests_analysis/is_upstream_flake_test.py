#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for is_upstream_flake.py."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

# pylint: disable=wrong-import-position
import is_upstream_flake as flake
from luci_analysis import LuciAnalysisError

BRANCH = flake.Source("checkout", "abc", "refs/branch-heads/8103", 1035, 157)


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


class TestSource(unittest.TestCase):
    def test_parse_commit_position(self):
        message = (
            "Subject\n\nCr-Commit-Position: refs/branch-heads/8103@{#1035}\n"
        )
        self.assertEqual(
            flake.parse_commit_position(message),
            ("refs/branch-heads/8103", 1035),
        )
        with self.assertRaises(LuciAnalysisError):
            flake.parse_commit_position("Subject\n")

    def test_parse_milestone(self):
        self.assertEqual(flake.parse_milestone("MAJOR=157\nMINOR=0\n"), 157)

    def test_project(self):
        self.assertEqual(BRANCH.project, "chromium-m157")
        main = flake.Source("main", "abc", "refs/heads/main", 1, 158)
        self.assertEqual(main.project, "chromium")
        with self.assertRaises(LuciAnalysisError):
            _ = flake.Source("x", "abc", "refs/heads/other", 1, 1).project


class TestStability(unittest.TestCase):
    def status(self, analysis: dict[str, Any]):
        return flake.Stability("t", {}, analysis).status

    def test_status(self):
        Status = flake.Status
        self.assertEqual(
            self.status({"flakeRate": {"isMet": True}}), Status.FLAKY
        )
        self.assertEqual(
            self.status(
                {"flakeRate": {"isMet": True}, "failureRate": {"isMet": True}}
            ),
            Status.FAILING,
        )
        self.assertEqual(
            self.status({"flakeRate": {"totalVerdicts": 10}}), Status.STABLE
        )
        self.assertEqual(self.status({}), Status.NO_DATA)

    def test_example_builds(self):
        stability = flake.Stability(
            "t",
            {},
            {
                "flakeRate": {
                    "flakeExamples": [
                        {"rootInvocations": ["build-1"]},
                        {"invocations": ["build-2", "task-x"]},
                    ]
                }
            },
        )
        self.assertEqual(
            stability.example_builds,
            ["https://ci.chromium.org/b/1", "https://ci.chromium.org/b/2"],
        )


class FakeRequest:
    def __init__(self, reply: Any) -> None:
        self._reply = reply

    def send(self) -> dict[str, Any]:
        return self._reply

    def gather(self, field: str) -> list[Any]:
        return self._reply.get(field, [])


class FakeClient:
    """Answers QueryTests, QueryVariants and QueryStability."""

    def __init__(self, test_ids: list[str], variants: list[dict[str, Any]]):
        self.test_ids = test_ids
        self.variants = variants
        self.stability_bodies: list[dict[str, Any]] = []

    def tests_matching(self, _substring: str) -> list[str]:
        return self.test_ids

    def paged_request(self, _service: str, _method: str, **_body: Any):
        return FakeRequest({"variants": self.variants})

    def request(self, _service: str, _method: str, **body: Any):
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
                for i in range(flake.MAX_STABILITY_BATCH + 1)
            ],
        )
        results, _ = flake.query(client, "S#T", flake.Filters(), BRANCH)
        self.assertEqual(len(results), flake.MAX_STABILITY_BATCH + 1)
        self.assertEqual(len(client.stability_bodies), 2)


if __name__ == "__main__":
    unittest.main()
