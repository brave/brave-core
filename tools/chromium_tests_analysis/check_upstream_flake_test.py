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

from rich.console import Console

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


class RenderReportTest(unittest.TestCase):
    """The tables, rendered to a fixed-width console and read back."""

    def setUp(self) -> None:
        self.output = io.StringIO()
        patcher = mock.patch.object(
            check_upstream_flake, "console",
            Console(file=self.output, width=200, no_color=True))
        patcher.start()
        self.addCleanup(patcher.stop)

    def render(self,
               test_results: list[Any],
               days: int = 30,
               test_name: str = "Suite.Test") -> str:
        check_upstream_flake.render_report(test_name, test_results, days)
        return self.output.getvalue()

    def test_titles_the_table_with_the_lookback_window(self) -> None:
        report = self.render([("id", reading())], days=45)

        self.assertIn("Upstream flakiness over the past 45 days", report)

    def test_names_its_source(self) -> None:
        report = self.render([("id", reading())])

        self.assertIn("analysis.api.luci.app", report)

    def test_says_so_when_nothing_matched(self) -> None:
        report = self.render([], test_name="Suite.Test")

        self.assertIn("No test matching", report)
        self.assertIn("Suite.Test", report)

    def test_gives_every_match_a_row(self) -> None:
        report = self.render([("id-one", reading()), ("id-two", reading())])

        self.assertIn("id-one", report)
        self.assertIn("id-two", report)

    def test_shows_the_verdict_and_its_advice(self) -> None:
        report = self.render([("id", reading(Verdict.KNOWN_FLAKE))])

        self.assertIn("KNOWN UPSTREAM FLAKE", report)
        self.assertIn(Verdict.KNOWN_FLAKE.recommendation, report)

    def test_advice_is_given_once_per_verdict_not_per_test(self) -> None:
        report = self.render([
            ("a", reading(Verdict.KNOWN_FLAKE)),
            ("b", reading(Verdict.KNOWN_FLAKE)),
            ("c", reading(Verdict.KNOWN_FLAKE)),
        ])

        self.assertEqual(report.count(Verdict.KNOWN_FLAKE.recommendation), 1)

    def test_advice_is_ordered_worst_first(self) -> None:
        report = self.render([
            ("stable", reading(Verdict.STABLE)),
            ("flaky", reading(Verdict.KNOWN_FLAKE)),
        ])

        self.assertLess(report.index(Verdict.KNOWN_FLAKE.recommendation),
                        report.index(Verdict.STABLE.recommendation))

    def test_tabulates_the_counts(self) -> None:
        report = self.render([("id", reading(passed=9000, failed=5, flaky=5))])

        # Thousands are grouped so long columns stay readable.
        self.assertIn("9,000", report)
        self.assertIn("0.1%", report)

    def test_renders_the_daily_breakdown(self) -> None:
        flakiness = Flakiness.of_groups([
            StatsGroup("2026-02-07", "h",
                       VerdictCounts(passed=8, failed=1, flaky=1))
        ])

        report = self.render([("id", flakiness)])

        self.assertIn("Date", report)
        self.assertIn("2026-02-07", report)
        self.assertIn("20%", report)

    def test_a_day_without_meaningful_verdicts_has_no_rate(self) -> None:
        flakiness = Flakiness(counts=VerdictCounts(skipped=5),
                              daily=(DailyCounts("2026-02-07",
                                                 VerdictCounts(skipped=5)), ))

        report = self.render([("id", flakiness)])

        self.assertIn("2026-02-07", report)
        self.assertIn("--", report)

    def test_omits_the_breakdown_when_there_is_none(self) -> None:
        report = self.render([("id", reading())])

        # The matches table is still there; only the daily one is gone.
        self.assertIn("Upstream flakiness", report)
        self.assertNotIn("Date", report)

    def test_the_daily_table_is_labelled_with_its_test(self) -> None:
        flakiness = Flakiness.of_groups(
            [StatsGroup("2026-02-07", "h", VerdictCounts(passed=10))])

        report = self.render([("a-very-distinctive-test-id", flakiness)])

        # Once in the matches table, once labelling the daily table.
        self.assertEqual(report.count("a-very-distinctive-test-id"), 2)


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
        self.assertIn("No test matching", stdout)

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

    def test_writes_tables_by_default(self) -> None:
        self.client.tests_matching.return_value = ["id"]

        _, stdout = self.run_main("Suite.Test")

        self.assertIn("Upstream flakiness over the past", stdout)


if __name__ == "__main__":
    unittest.main()
