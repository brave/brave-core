#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for check-upstream-flake.py."""

from __future__ import annotations

import contextlib
import importlib.util
import io
import json
import sys
import unittest
from pathlib import Path
from types import ModuleType
from typing import Any
from unittest import mock

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

# pylint: disable=wrong-import-position
from luci_analysis import (
    DailyCounts,
    Flakiness,
    StatsGroup,
    TestVerdict,
    Verdict,
    VerdictCounts,
)


def load_script(file_name: str) -> ModuleType:
    """Import a script whose file name is not a valid module name."""
    path = SCRIPT_DIR / file_name
    module_name = path.stem.replace("-", "_")
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


check_upstream_flake = load_script("check-upstream-flake.py")

# Counts that land on each verdict, so a test can ask for the reading it
# cares about without restating the thresholds.
COUNTS_FOR = {
    Verdict.KNOWN_FLAKE: VerdictCounts(passed=90, failed=10),
    Verdict.OCCASIONAL: VerdictCounts(passed=98, failed=2),
    Verdict.STABLE: VerdictCounts(passed=100),
    Verdict.INSUFFICIENT_DATA: VerdictCounts(),
}


def reading(verdict: Verdict = Verdict.STABLE, **counts: int) -> Flakiness:
    """A Flakiness that reads as `verdict`, or with exact counts."""
    if counts:
        return Flakiness(VerdictCounts(**counts))
    return Flakiness(COUNTS_FOR[verdict])


class FormatReportMarkdownTest(unittest.TestCase):

    def test_reports_the_lookback_window_and_source(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", reading())], 45)

        self.assertIn("# Upstream Flake Check: Suite.Test", report)
        self.assertIn("Lookback period: 45 days", report)
        self.assertIn("Source: Chromium LUCI Analysis", report)

    def test_reports_when_nothing_matched(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [], 30)

        self.assertIn("## Result: Not Found", report)
        self.assertIn("No matching test IDs found", report)
        self.assertNotIn("### Statistics", report)

    def test_renders_each_matched_test(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id-one", reading()), ("id-two", reading())], 30)

        self.assertIn("## Test: `id-one`", report)
        self.assertIn("## Test: `id-two`", report)

    def test_spells_out_the_verdict_and_recommendation(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", reading(Verdict.KNOWN_FLAKE))], 30)

        self.assertIn("### Verdict: KNOWN UPSTREAM FLAKE", report)
        self.assertIn(
            f"**Recommendation:** {Verdict.KNOWN_FLAKE.recommendation}",
            report)

    def test_every_verdict_has_a_heading_of_its_own(self) -> None:
        for verdict in Verdict:
            with self.subTest(verdict=verdict):
                report = check_upstream_flake.format_report_markdown(
                    "Suite.Test", [("id", reading(verdict))], 30)
                self.assertIn(f"### Verdict: {verdict.headline}", report)

    def test_reports_the_verdict_counts(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", reading(passed=90, failed=5, flaky=5))], 30)

        self.assertIn("- Meaningful verdicts (pass+fail+flaky): 100", report)
        self.assertIn("- Passed: 90", report)
        self.assertIn("- Failed: 5", report)
        self.assertIn("- Flaky: 5", report)
        self.assertIn("- Flake rate: 10.0%", report)

    def test_omits_skips_and_execution_errors_when_absent(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", reading())], 30)

        self.assertNotIn("- Skipped:", report)
        self.assertNotIn("- Execution errors:", report)

    def test_reports_skips_and_execution_errors_when_present(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test",
            [("id", reading(passed=10, skipped=4, execution_errored=2))], 30)

        self.assertIn("- Skipped: 4", report)
        self.assertIn("- Execution errors: 2", report)

    def test_renders_the_daily_breakdown(self) -> None:
        flakiness = Flakiness.of_groups([
            StatsGroup("2026-02-07", "h",
                       VerdictCounts(passed=8, failed=1, flaky=1))
        ])

        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", flakiness)], 30)

        self.assertIn("### Daily Breakdown", report)
        self.assertIn("| Date | Total | Pass | Fail | Flaky | Rate |", report)
        self.assertIn("| 2026-02-07 | 10 | 8 | 1 | 1 | 20% |", report)

    def test_a_day_without_meaningful_verdicts_has_no_rate(self) -> None:
        flakiness = Flakiness(counts=VerdictCounts(skipped=5),
                              daily=(DailyCounts("2026-02-07",
                                                 VerdictCounts(skipped=5)), ))

        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", flakiness)], 30)

        self.assertIn("| 2026-02-07 | 5 | 0 | 0 | 0 | N/A |", report)

    def test_omits_the_breakdown_when_empty(self) -> None:
        report = check_upstream_flake.format_report_markdown(
            "Suite.Test", [("id", reading())], 30)

        self.assertNotIn("### Daily Breakdown", report)


class FormatReportJsonTest(unittest.TestCase):

    def test_includes_the_query_and_every_match(self) -> None:
        report = json.loads(
            check_upstream_flake.format_report_json("Suite.Test",
                                                    [("id-one", reading()),
                                                     ("id-two", reading())],
                                                    60))

        self.assertEqual(report["test_name"], "Suite.Test")
        self.assertEqual(report["lookback_days"], 60)
        self.assertEqual([test["test_id"] for test in report["matched_tests"]],
                         ["id-one", "id-two"])
        self.assertEqual(report["matched_tests"][0]["flake_rate"], 0.0)

    def test_overall_verdict_is_the_worst_of_the_matches(self) -> None:
        report = json.loads(
            check_upstream_flake.format_report_json("Suite.Test", [
                ("stable", reading(Verdict.STABLE)),
                ("flaky", reading(Verdict.KNOWN_FLAKE)),
                ("thin", reading(Verdict.INSUFFICIENT_DATA)),
            ], 30))

        self.assertEqual(report["overall_verdict"], "known_upstream_flake")
        self.assertEqual(report["overall_recommendation"],
                         Verdict.KNOWN_FLAKE.recommendation)

    def test_occasional_failures_outrank_insufficient_data(self) -> None:
        report = json.loads(
            check_upstream_flake.format_report_json("Suite.Test", [
                ("thin", reading(Verdict.INSUFFICIENT_DATA)),
                ("some", reading(Verdict.OCCASIONAL)),
            ], 30))

        self.assertEqual(report["overall_verdict"],
                         "occasional_upstream_failures")

    def test_insufficient_data_outranks_stable(self) -> None:
        report = json.loads(
            check_upstream_flake.format_report_json("Suite.Test", [
                ("stable", reading(Verdict.STABLE)),
                ("thin", reading(Verdict.INSUFFICIENT_DATA)),
            ], 30))

        self.assertEqual(report["overall_verdict"], "insufficient_data")

    def test_reports_not_found_without_matches(self) -> None:
        report = json.loads(
            check_upstream_flake.format_report_json("Suite.Test", [], 30))

        self.assertEqual(report["matched_tests"], [])
        self.assertEqual(report["overall_verdict"], "not_found")
        self.assertIn("test not found", report["overall_recommendation"])


class MainTest(unittest.TestCase):
    """Drives main() with the client stubbed out."""

    def setUp(self) -> None:
        self.client = mock.Mock()
        self.client.tests_matching.return_value = []
        self.client.history.return_value = []
        self.client.verdicts.return_value = []
        self.enter_patch(
            mock.patch.object(check_upstream_flake, "client", self.client))

    def enter_patch(self, patcher: Any) -> Any:
        value = patcher.start()
        self.addCleanup(patcher.stop)
        return value

    def run_main(self, *argv: str) -> tuple[int, str]:
        """Run main(), returning (exit code, stdout)."""
        stdout = io.StringIO()
        with mock.patch.object(sys, "argv",
                               ["check-upstream-flake.py", *argv]):
            with contextlib.redirect_stdout(stdout):
                with contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as caught:
                        check_upstream_flake.main()
        return caught.exception.code, stdout.getvalue()

    def test_rejects_a_lookback_window_that_is_too_long(self) -> None:
        code, _ = self.run_main("Suite.Test", "--days", "91")

        self.assertEqual(code, 1)
        self.client.tests_matching.assert_not_called()

    def test_rejects_a_lookback_window_that_is_too_short(self) -> None:
        code, _ = self.run_main("Suite.Test", "--days", "0")

        self.assertEqual(code, 1)

    def test_exits_with_two_when_nothing_matched(self) -> None:
        code, stdout = self.run_main("Suite.Test")

        self.assertEqual(code, 2)
        self.assertIn("## Result: Not Found", stdout)

    def test_reports_stats_for_a_match(self) -> None:
        self.client.tests_matching.return_value = ["://t!gtest::Suite#Test"]
        self.client.history.return_value = [
            StatsGroup("2026-02-07", "h", VerdictCounts(passed=90, failed=10))
        ]

        code, stdout = self.run_main("Suite.Test", "--json")
        report = json.loads(stdout)

        self.assertEqual(code, 0)
        self.assertEqual(report["overall_verdict"], "known_upstream_flake")
        self.assertEqual(report["matched_tests"][0]["failed"], 10)

    def test_asks_for_the_requested_lookback_window(self) -> None:
        self.client.tests_matching.return_value = ["id"]

        self.run_main("Suite.Test", "--days", "60", "--json")

        window = self.client.history.call_args.args[1]
        self.assertAlmostEqual(window.days, 60, places=3)

    def test_every_match_shares_one_window(self) -> None:
        self.client.tests_matching.return_value = ["a", "b", "c"]

        self.run_main("Suite.Test", "--json")

        windows = {call.args[1] for call in self.client.history.call_args_list}
        self.assertEqual(len(windows), 1)

    def test_falls_back_to_individual_verdicts(self) -> None:
        self.client.tests_matching.return_value = ["id"]
        self.client.history.return_value = []
        self.client.verdicts.return_value = [TestVerdict("FAILED")] * 20

        _, stdout = self.run_main("Suite.Test", "--json")
        report = json.loads(stdout)

        self.client.verdicts.assert_called_once()
        self.assertEqual(report["matched_tests"][0]["failed"], 20)
        self.assertEqual(report["overall_verdict"], "known_upstream_flake")

    def test_reports_insufficient_data_when_both_queries_are_empty(
            self) -> None:
        self.client.tests_matching.return_value = ["id"]

        _, stdout = self.run_main("Suite.Test", "--json")
        report = json.loads(stdout)

        self.assertEqual(report["overall_verdict"], "insufficient_data")
        self.assertEqual(report["matched_tests"][0]["total_verdicts"], 0)

    def test_checks_at_most_five_matches(self) -> None:
        self.client.tests_matching.return_value = [
            f"id-{i}" for i in range(12)
        ]

        _, stdout = self.run_main("Suite.Test", "--json")
        report = json.loads(stdout)

        self.assertEqual(len(report["matched_tests"]), 5)
        self.assertEqual(self.client.history.call_count, 5)

    def test_prefers_the_closest_matching_ids(self) -> None:
        self.client.tests_matching.return_value = [
            "zzz_unrelated",
            "prefixBar",
            "some/Bar",
        ]

        _, stdout = self.run_main("Bar", "--json")
        report = json.loads(stdout)

        self.assertEqual([test["test_id"] for test in report["matched_tests"]],
                         ["some/Bar", "prefixBar", "zzz_unrelated"])

    def test_writes_markdown_by_default(self) -> None:
        self.client.tests_matching.return_value = ["id"]

        _, stdout = self.run_main("Suite.Test")

        self.assertIn("# Upstream Flake Check: Suite.Test", stdout)


if __name__ == "__main__":
    unittest.main()
