#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for update-upstream-flake-filters.py."""

from __future__ import annotations

import importlib.util
import io
import os
import sys
import tempfile
import unittest
from datetime import timedelta
from pathlib import Path
from types import ModuleType
from typing import Any
from unittest import mock

import gevent
from rich.console import Console

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))


def load_script(file_name: str) -> ModuleType:
    """Import a script whose file name is not a valid module name."""
    path = SCRIPT_DIR / file_name
    module_name = path.stem.replace("-", "_")
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


from luci_analysis import (  # pylint: disable=wrong-import-position
    ClusterFailure, ClusterSummary, Flakiness, LuciAnalysis, StatsGroup,
    TestVariant, VerdictCounts,
)

ufm = load_script("update-upstream-flake-filters.py")

# A structured test ID as LUCI reports it, with the ":" inside the build
# target escaped.
UNIT_TESTS_TARGET = "://chrome/test\\:unit_tests!gtest::"


def stats_group(variant_hash: str,
                date: str = "2026-02-07",
                **counts: int) -> StatsGroup:
    """A day of one variant's verdicts."""
    return StatsGroup(date=date,
                      variant_hash=variant_hash,
                      counts=VerdictCounts(**counts))


def variant(variant_hash: str, os_name: str, *builders: str) -> TestVariant:
    """A bot configuration a test ran on."""
    return TestVariant(variant_hash=variant_hash,
                       os=os_name,
                       builders=builders)


class QuietConsoleTestCase(unittest.TestCase):
    """Fixture that captures what the script renders instead of printing it.

    The display is replaced as well as the console, so that rows and
    counters from one test cannot leak into the next.
    """

    def setUp(self) -> None:
        self.output = io.StringIO()
        console = Console(file=self.output, width=200)
        self.enter_patch(mock.patch.object(ufm, "console", console))
        # A display and a client of their own, so rows and counters from
        # one test cannot leak into the next.
        self.client = LuciAnalysis()
        self.display = ufm.Display(self.client.stats)

    def rendered(self) -> str:
        """Paint the display once and return the text."""
        self.output.truncate(0)
        self.output.seek(0)
        ufm.console.print(self.display)
        return self.output.getvalue()

    def enter_patch(self, patcher: Any) -> Any:
        value = patcher.start()
        self.addCleanup(patcher.stop)
        return value

    def logged(self) -> str:
        return self.output.getvalue()


class SuiteUpdaterTestCase(QuietConsoleTestCase):
    """A SuiteUpdater whose collaborators are all stand-ins.

    Nothing here patches a module global: the updater is handed its
    client, its display and its output directory.
    """

    def setUp(self) -> None:
        super().setUp()
        self.filters_dir = Path(
            self.enter_context(tempfile.TemporaryDirectory()))
        self.client = mock.Mock()
        self.client.cluster_summaries.return_value = []
        self.client.cluster_failures.return_value = []
        self.client.history.return_value = []
        self.client.variants.return_value = []

    def enter_context(self, context_manager: Any) -> Any:
        value = context_manager.__enter__()
        self.addCleanup(context_manager.__exit__, None, None, None)
        return value

    def updater(self,
                suite: str = "unit_tests",
                days: int = 30,
                min_flake_rate: float = 0.01) -> Any:
        return ufm.SuiteUpdater(suite,
                                client=self.client,
                                display=self.display,
                                days=days,
                                min_flake_rate=min_flake_rate,
                                filters_dir=self.filters_dir)


class GetCandidateWindowsTest(unittest.TestCase):

    def test_full_window_comes_first(self) -> None:
        windows = ufm.get_candidate_windows(30)

        self.assertAlmostEqual(windows[0].days, 30, places=3)

    def test_adds_one_slice_per_week(self) -> None:
        # The full window, plus 7+7+7+7+2 days of slices.
        self.assertEqual(len(ufm.get_candidate_windows(30)), 6)
        self.assertEqual(len(ufm.get_candidate_windows(10)), 3)

    def test_a_window_shorter_than_a_week_gets_one_slice(self) -> None:
        windows = ufm.get_candidate_windows(7)

        self.assertEqual(len(windows), 2)
        self.assertEqual(windows[0], windows[1])

    def test_slices_are_contiguous_and_cover_the_window(self) -> None:
        windows = ufm.get_candidate_windows(30)
        full = windows[0]
        slices = windows[1:]

        self.assertEqual(slices[0].earliest, full.earliest)
        self.assertEqual(slices[-1].latest, full.latest)
        for before, after in zip(slices, slices[1:]):
            self.assertEqual(before.latest, after.earliest)

    def test_a_single_day_window(self) -> None:
        windows = ufm.get_candidate_windows(1)

        self.assertEqual(len(windows), 2)
        for window in windows:
            self.assertAlmostEqual(window.days, 1, places=3)


class GetDiscoveryFiltersTest(unittest.TestCase):

    def test_scopes_the_fleet_wide_filter_by_sanitizer_and_os(self) -> None:
        filters = list(ufm.get_discovery_filters("unit_tests"))

        self.assertEqual(filters, [
            'test_id:":unit_tests!gtest"',
            'test_id:":unit_tests!gtest" variant.builder:"asan"',
            'test_id:":unit_tests!gtest" variant.builder:"msan"',
            'test_id:":unit_tests!gtest" variant.builder:"ubsan"',
            'test_id:":unit_tests!gtest" variant.os:"Ubuntu"',
            'test_id:":unit_tests!gtest" variant.os:"Linux"',
            'test_id:":unit_tests!gtest" variant.os:"Mac"',
            'test_id:":unit_tests!gtest" variant.os:"Windows"',
        ])


class NormalizeTestIdTest(unittest.TestCase):

    def test_an_id_without_a_case_is_unchanged(self) -> None:
        self.assertEqual(ufm.normalize_test_id("://target!gtest::Suite"),
                         "://target!gtest::Suite")

    def test_a_plain_case_is_unchanged(self) -> None:
        test_id = UNIT_TESTS_TARGET + "Suite#Case"

        self.assertEqual(ufm.normalize_test_id(test_id), test_id)

    def test_a_webui_sub_result_maps_to_its_parent_case(self) -> None:
        self.assertEqual(
            ufm.normalize_test_id(UNIT_TESTS_TARGET + "Suite#Case__SubTest"),
            UNIT_TESTS_TARGET + "Suite#Case")

    def test_a_parameter_survives_normalisation(self) -> None:
        self.assertEqual(
            ufm.normalize_test_id(UNIT_TESTS_TARGET +
                                  "Suite#Case__SubTest/All.0"),
            UNIT_TESTS_TARGET + "Suite#Case/All.0")

    def test_a_parameterised_case_is_unchanged(self) -> None:
        test_id = UNIT_TESTS_TARGET + "Suite#Case/All.0"

        self.assertEqual(ufm.normalize_test_id(test_id), test_id)


class StructuredIdToGtestNameTest(unittest.TestCase):

    def test_a_plain_case(self) -> None:
        self.assertEqual(
            ufm.structured_id_to_gtest_name(UNIT_TESTS_TARGET + "Suite#Case"),
            "Suite.Case")

    def test_an_instantiated_parameterised_case(self) -> None:
        self.assertEqual(
            ufm.structured_id_to_gtest_name(UNIT_TESTS_TARGET +
                                            "Suite#Case/All.0"),
            "All/Suite.Case/0")

    def test_a_parameterised_case_without_an_instantiation(self) -> None:
        self.assertEqual(
            ufm.structured_id_to_gtest_name(UNIT_TESTS_TARGET +
                                            "Suite#Case/3"), "Suite.Case/3")

    def test_a_non_gtest_scheme_is_rejected(self) -> None:
        self.assertIsNone(
            ufm.structured_id_to_gtest_name(
                "://chrome/test\\:junit!junit::org.Foo#bar"))

    def test_an_id_without_a_case_is_rejected(self) -> None:
        self.assertIsNone(
            ufm.structured_id_to_gtest_name(UNIT_TESTS_TARGET + "Suite"))


class GetAllConfigsTest(unittest.TestCase):

    def test_every_platform_gets_a_plain_and_a_sanitizer_config(self) -> None:
        self.assertEqual(list(ufm.get_all_configs()), [
            "linux",
            "linux-asan",
            "linux-msan",
            "linux-ubsan",
            "macos",
            "macos-asan",
            "macos-msan",
            "macos-ubsan",
            "windows",
            "windows-asan",
            "windows-msan",
            "windows-ubsan",
        ])


class GetConfigForVariantTest(unittest.TestCase):

    def test_maps_the_bot_os_to_a_platform(self) -> None:
        self.assertEqual(
            ufm.get_config_for_variant(
                variant("h", "Ubuntu-22.04", "linux-rel")), "linux")
        self.assertEqual(
            ufm.get_config_for_variant(variant("h", "Linux", "linux-rel")),
            "linux")
        self.assertEqual(
            ufm.get_config_for_variant(variant("h", "Mac-15", "mac-rel")),
            "macos")
        self.assertEqual(
            ufm.get_config_for_variant(variant("h", "Windows-10",
                                               "win10-rel")), "windows")

    def test_an_unknown_os_has_no_config(self) -> None:
        self.assertIsNone(
            ufm.get_config_for_variant(variant("h", "ChromeOS", "cros-rel")))

    def test_a_variant_without_an_os_has_no_config(self) -> None:
        self.assertIsNone(ufm.get_config_for_variant(variant("h", "")))

    def test_reads_the_sanitizer_off_the_builder_name(self) -> None:
        for sanitizer in ("asan", "msan", "ubsan"):
            with self.subTest(sanitizer=sanitizer):
                self.assertEqual(
                    ufm.get_config_for_variant(
                        variant("h", "Ubuntu-22.04",
                                f"linux_chromium_{sanitizer}_rel_ng")),
                    f"linux-{sanitizer}")

    def test_lsan_bots_count_as_asan(self) -> None:
        self.assertEqual(
            ufm.get_config_for_variant(
                variant("h", "Ubuntu-22.04", "linux-lsan-rel")), "linux-asan")

    def test_builder_names_are_matched_case_insensitively(self) -> None:
        self.assertEqual(
            ufm.get_config_for_variant(
                variant("h", "Mac-15", "Mac ASan 64 Tests")), "macos-asan")

    def test_platforms_brave_does_not_build_are_dropped(self) -> None:
        for builder in ("android-x86-rel", "chromeos-amd64-generic-rel",
                        "Linux Chromium OS ASan LSan Tests", "fuchsia-x64-rel",
                        "linux-tsan-rel"):
            with self.subTest(builder=builder):
                self.assertIsNone(
                    ufm.get_config_for_variant(
                        variant("h", "Ubuntu-22.04", builder)))

    def test_ios_bots_are_dropped_despite_their_mac_os(self) -> None:
        self.assertIsNone(
            ufm.get_config_for_variant(variant("h", "Mac-15",
                                               "ios-simulator")))

    def test_a_reviver_run_inherits_the_builder_it_retries(self) -> None:
        self.assertEqual(
            ufm.get_config_for_variant(
                variant("h", "Ubuntu-22.04", "runner",
                        "linux_chromium_asan_rel_ng")), "linux-asan")

    def test_a_reviver_run_of_an_excluded_builder_is_dropped(self) -> None:
        self.assertIsNone(
            ufm.get_config_for_variant(
                variant("h", "Ubuntu-22.04", "runner", "android-x86-rel")))


class AnalyzePerConfigTest(unittest.TestCase):

    def test_splits_the_verdict_counts_by_config(self) -> None:
        groups = [
            stats_group("h-linux", passed=90, failed=10),
            stats_group("h-asan", passed=50, flaky=50),
        ]
        config_by_hash = {"h-linux": "linux", "h-asan": "linux-asan"}

        analyses = ufm.analyze_per_config(groups, config_by_hash)

        self.assertAlmostEqual(analyses["linux"].flake_rate, 0.1)
        self.assertAlmostEqual(analyses["linux-asan"].flake_rate, 0.5)
        self.assertEqual(analyses["macos"].counts.meaningful, 0)

    def test_covers_every_config(self) -> None:
        analyses = ufm.analyze_per_config([], {})

        self.assertEqual(sorted(analyses), sorted(ufm.get_all_configs()))

    def test_unmapped_variants_are_ignored(self) -> None:
        groups = [stats_group("h-unknown", passed=10, failed=90)]

        analyses = ufm.analyze_per_config(groups, {"h-unknown": None})

        for config, flakiness in analyses.items():
            with self.subTest(config=config):
                self.assertEqual(flakiness.counts.meaningful, 0)


class FilterContentTest(SuiteUpdaterTestCase):

    def test_header_states_the_threshold_and_how_to_regenerate(self) -> None:
        content = self.updater().filter_content("linux-asan", [])

        self.assertTrue(
            content.startswith("## AUTO-GENERATED FILE -- DO NOT EDIT.\n"))
        self.assertIn(
            "## Upstream unit_tests tests with a flake rate >= 1.0% on",
            content)
        self.assertIn(
            "## linux-asan bots over the past 30 days per Chromium LUCI"
            " Analysis.", content)
        self.assertIn(
            "##   vpython3 tools/chromium_tests_analysis/"
            "update-upstream-flake-filters.py", content)

    def test_documents_the_rate_behind_each_exclusion(self) -> None:
        entries = [("Suite.Case",
                    Flakiness(VerdictCounts(passed=95, failed=3, flaky=2)))]

        content = self.updater().filter_content("linux", entries)

        self.assertIn(
            "# 5.0% flake rate over 30 days per LUCI Analysis"
            " (95 passed, 3 failed, 2 flaky).\n-Suite.Case\n", content)

    def test_entries_are_sorted_by_test_name(self) -> None:
        flakiness = Flakiness(VerdictCounts(passed=9, failed=1))
        entries = [("Suite.Zeta", flakiness), ("Suite.Alpha", flakiness)]

        content = self.updater().filter_content("linux", entries)

        self.assertLess(content.index("-Suite.Alpha"),
                        content.index("-Suite.Zeta"))

    def test_the_file_ends_with_a_newline(self) -> None:
        content = self.updater().filter_content("linux", [])

        self.assertTrue(content.endswith("\n"))


class CountColumnTest(QuietConsoleTestCase):

    def render(self, completed: int, total: int | None) -> str:
        with self.display.phase("a suite", "a phase", total) as task:
            task.advance(completed)
            return ufm.CountColumn().render(self.display.tasks[-1]).plain

    def test_does_not_pad_the_completed_count(self) -> None:
        # Rich's own column would render this as " 1/48", putting it one
        # column to the right of a "0/9" row above it.
        self.assertEqual(self.render(1, 48), "1/48")

    def test_renders_a_count_of_any_width(self) -> None:
        self.assertEqual(self.render(0, 9), "0/9")
        self.assertEqual(self.render(10, 48), "10/48")
        self.assertEqual(self.render(1430, 1430), "1430/1430")

    def test_renders_an_unknown_total(self) -> None:
        self.assertEqual(self.render(3, None), "3/?")


class DisplayTest(QuietConsoleTestCase):

    def test_a_phase_adds_a_row_and_takes_it_away_again(self) -> None:
        with self.display.phase("unit_tests", "doing things", 7) as task:
            self.assertEqual(len(self.display.tasks), 1)
            row = self.display.tasks[0]
            self.assertEqual(row.total, 7)
            self.assertEqual(row.completed, 0)
            task.advance()
            self.assertEqual(row.completed, 1)

        self.assertEqual(len(self.display.tasks), 0)

    def test_a_phase_row_is_removed_even_when_the_work_raises(self) -> None:
        with self.assertRaises(ValueError):
            with self.display.phase("unit_tests", "doing things", 1):
                raise ValueError("boom")

        self.assertEqual(len(self.display.tasks), 0)

    def test_a_row_names_its_suite_and_phase(self) -> None:
        with self.display.phase("unit_tests", "doing things", 1):
            self.assertIn("unit_tests doing things", self.rendered())

    def test_concurrent_phases_each_get_a_row(self) -> None:
        with self.display.phase("unit_tests", "fetching", 2):
            with self.display.phase("net_unittests", "querying", 3):
                painted = self.rendered()

        self.assertIn("unit_tests fetching", painted)
        self.assertIn("net_unittests querying", painted)

    def test_the_summary_totals_the_phases_in_flight(self) -> None:
        with self.display.phase("unit_tests", "fetching", 10) as first:
            first.advance(4)
            with self.display.phase("net_unittests", "querying", 20) as second:
                second.advance(1)

                self.assertIn("[5/30]", self.rendered())

    def test_a_finished_phase_still_counts_towards_the_summary(self) -> None:
        with self.display.phase("unit_tests", "fetching", 10) as task:
            task.advance(10)

        self.assertIn("[10/10]", self.rendered())

    def test_the_summary_counts_suites_off(self) -> None:
        with self.display.live(3):
            self.display.suite_finished()
            self.display.suite_finished()

            self.assertIn("suites:2/3", self.rendered())

    def test_the_summary_reports_the_client_counters(self) -> None:
        self.client.stats.started = 70
        self.client.stats.completed = 64
        self.client.stats.retries = 5

        painted = self.rendered()

        self.assertIn("inflight:6", painted)
        self.assertIn("retries:5", painted)

    def test_the_summary_only_mentions_errors_when_there_are_some(
            self) -> None:
        self.assertNotIn("errors:", self.rendered())

        self.client.stats.failures = 2

        self.assertIn("errors:2", self.rendered())


class WorkerPoolTest(unittest.TestCase):

    def test_runs_the_work_concurrently(self) -> None:
        with ufm.worker_pool(4) as pool:
            self.assertEqual(list(pool.imap(str, [1, 2, 3])), ["1", "2", "3"])

    def test_an_interrupt_kills_the_work_in_flight(self) -> None:
        # Ctrl+C must not wait for requests that are already on the wire.
        done: list[int] = []

        def work() -> None:
            gevent.sleep(0.05)
            done.append(1)

        with self.assertRaises(KeyboardInterrupt):
            with ufm.worker_pool(8) as pool:
                for _ in range(8):
                    pool.spawn(work)
                gevent.sleep(0)  # Let the pool get started.
                raise KeyboardInterrupt

        pool.join(timeout=5)
        self.assertEqual(len(pool), 0)
        self.assertEqual(done, [])

    def test_an_ordinary_failure_also_tears_the_pool_down(self) -> None:
        with self.assertRaises(ValueError):
            with ufm.worker_pool(8) as pool:
                for _ in range(8):
                    pool.spawn(gevent.sleep, 10)
                gevent.sleep(0)
                raise ValueError("boom")

        pool.join(timeout=5)
        self.assertEqual(len(pool), 0)


class RunInParallelTest(unittest.TestCase):

    def test_abandoning_the_results_stops_the_fan_out(self) -> None:
        # The caller giving up has to stop the work being queued behind
        # it, not just the items already running.
        started: list[int] = []

        def work(item: int) -> int:
            started.append(item)
            gevent.sleep(0.01)
            return item

        with self.assertRaises(RuntimeError):
            for _ in ufm.run_in_parallel(work, range(1000)):
                raise RuntimeError("the caller gives up")

        gevent.sleep(0.05)  # Let the kill land.
        settled = len(started)
        gevent.sleep(0.2)
        self.assertEqual(len(started), settled, "work kept being queued")
        self.assertLess(settled, 1000)

    def test_yields_results_in_the_order_of_the_input(self) -> None:
        # Later items finish first, but the caller still sees input order.
        def work(item: int) -> int:
            gevent.sleep((10 - item) / 1000)
            return item

        self.assertEqual(list(ufm.run_in_parallel(work, range(10))),
                         list(range(10)))

    def test_runs_the_items_at_the_same_time(self) -> None:
        running = []
        peak = []

        def work(_item: int) -> None:
            running.append(1)
            peak.append(len(running))
            gevent.sleep(0.01)
            running.pop()

        list(ufm.run_in_parallel(work, range(8)))

        self.assertGreater(max(peak), 1)


class HandleInterruptTest(QuietConsoleTestCase):

    def setUp(self) -> None:
        super().setUp()
        self.client = mock.Mock()
        self.kill = self.enter_patch(
            mock.patch.object(ufm.requests_pool, "kill"))

    def test_leaves_with_the_interrupt_exit_code(self) -> None:
        with self.assertRaises(SystemExit) as caught:
            ufm.handle_interrupt(self.client)

        self.assertEqual(caught.exception.code, 130)

    def test_kills_the_requests_still_on_the_wire(self) -> None:
        with self.assertRaises(SystemExit):
            ufm.handle_interrupt(self.client)

        self.kill.assert_called_once_with(block=False)

    def test_tells_the_client_to_stop_retrying(self) -> None:
        with self.assertRaises(SystemExit):
            ufm.handle_interrupt(self.client)

        self.client.request_shutdown.assert_called_once_with()

    def test_says_the_written_files_are_kept(self) -> None:
        with self.assertRaises(SystemExit):
            ufm.handle_interrupt(self.client)

        self.assertIn("left in place", self.logged())


class DiscoverCandidatesTest(SuiteUpdaterTestCase):

    def setUp(self) -> None:
        super().setUp()
        self.summaries: list[ClusterSummary] = []
        self.failures_by_cluster: dict[tuple[str, str],
                                       list[ClusterFailure]] = {}
        self.client.cluster_summaries.side_effect = self.fake_summaries
        self.client.cluster_failures.side_effect = self.fake_failures

    def fake_summaries(self, _failure_filter: str,
                       _window: Any) -> list[ClusterSummary]:
        return self.summaries

    def fake_failures(self, cluster: ClusterSummary) -> list[ClusterFailure]:
        return self.failures_by_cluster.get(cluster.key, [])

    def discover(self, days: int = 1) -> list[str]:
        return self.updater(days=days).discover_candidates()

    @staticmethod
    def summary(algorithm: str, cluster_id: str, title: str) -> ClusterSummary:
        return ClusterSummary(algorithm=algorithm,
                              cluster_id=cluster_id,
                              title=title)

    def test_a_single_test_cluster_yields_its_title(self) -> None:
        self.summaries = [
            self.summary("testname-v4", "c1", UNIT_TESTS_TARGET + "S#Case")
        ]

        self.assertEqual(self.discover(), [UNIT_TESTS_TARGET + "S#Case"])

    def test_reason_clusters_are_skipped(self) -> None:
        self.summaries = [
            self.summary("reason-v3", "c1", "Assertion failed: %s")
        ]

        self.assertEqual(self.discover(), [])

    def test_tests_from_other_suites_are_ignored(self) -> None:
        self.summaries = [
            self.summary("testname-v4", "c1",
                         "://chrome/test\\:browser_tests!gtest::S#Case")
        ]

        self.assertEqual(self.discover(), [])

    def test_webui_sub_results_collapse_into_one_candidate(self) -> None:
        self.summaries = [
            self.summary("testname-v4", "c1",
                         UNIT_TESTS_TARGET + "S#Case__StepOne"),
            self.summary("testname-v4", "c2",
                         UNIT_TESTS_TARGET + "S#Case__StepTwo"),
        ]

        self.assertEqual(self.discover(), [UNIT_TESTS_TARGET + "S#Case"])

    def test_a_pattern_cluster_is_expanded_into_its_failures(self) -> None:
        self.summaries = [
            self.summary("testname-v4", "c1",
                         UNIT_TESTS_TARGET + "S#Case/All.%")
        ]
        self.failures_by_cluster[("testname-v4", "c1")] = [
            ClusterFailure(UNIT_TESTS_TARGET + "S#Case/All.0", 3),
            ClusterFailure(UNIT_TESTS_TARGET + "S#Case/All.1", 1),
        ]

        self.assertEqual(self.discover(), [
            UNIT_TESTS_TARGET + "S#Case/All.0",
            UNIT_TESTS_TARGET + "S#Case/All.1",
        ])

    def test_a_rule_cluster_is_expanded_into_its_failures(self) -> None:
        # Failures claimed by a filed bug only surface through their rule.
        self.summaries = [self.summary("rules", "r1", "crbug.com/1234567")]
        self.failures_by_cluster[("rules", "r1")] = [
            ClusterFailure(UNIT_TESTS_TARGET + "S#Tracked", 1),
            ClusterFailure("://chrome/test\\:browser_tests!gtest::S#Other", 1),
        ]

        self.assertEqual(self.discover(), [UNIT_TESTS_TARGET + "S#Tracked"])

    def test_every_test_a_cluster_names_is_checked(self) -> None:
        # Nothing of our own is dropped any more; only upstream limits.
        self.summaries = [self.summary("rules", "r1", "crbug.com/1234567")]
        self.failures_by_cluster[("rules", "r1")] = [
            ClusterFailure(UNIT_TESTS_TARGET + f"S#Case/All.{index}", index)
            for index in range(4)
        ]

        self.assertEqual(
            self.discover(),
            [UNIT_TESTS_TARGET + f"S#Case/All.{index}" for index in range(4)])
        self.assertNotIn("limit", self.logged())

    def test_a_cluster_at_the_server_limit_is_reported(self) -> None:
        self.enter_patch(mock.patch.object(ufm, "MAX_CLUSTER_FAILURES", 3))
        self.summaries = [self.summary("rules", "r1", "crbug.com/1234567")]
        self.failures_by_cluster[("rules", "r1")] = [
            ClusterFailure(UNIT_TESTS_TARGET + f"S#Case/All.{index}", 1)
            for index in range(3)
        ]

        self.discover()

        self.assertIn("1 clusters hit the server's 3-failure limit",
                      self.logged())

    def test_truncated_clusters_are_reported_once_for_the_suite(self) -> None:
        # One line per cluster would bury everything else the run says.
        self.enter_patch(mock.patch.object(ufm, "MAX_CLUSTER_FAILURES", 2))
        self.summaries = [
            self.summary("rules", f"r{cluster}", f"crbug.com/{cluster}")
            for cluster in range(5)
        ]
        for cluster in range(5):
            self.failures_by_cluster[("rules", f"r{cluster}")] = [
                ClusterFailure(
                    UNIT_TESTS_TARGET + f"S#Case{cluster}/All.{index}", 1)
                for index in range(2)
            ]

        self.discover()

        logged = self.logged()
        self.assertEqual(logged.count("limit"), 1)
        self.assertIn("5 clusters hit the server's 2-failure limit", logged)

    def test_a_cluster_under_the_server_limit_is_not_reported(self) -> None:
        self.enter_patch(mock.patch.object(ufm, "MAX_CLUSTER_FAILURES", 10))
        self.summaries = [self.summary("rules", "r1", "crbug.com/1234567")]
        self.failures_by_cluster[("rules", "r1")] = [
            ClusterFailure(UNIT_TESTS_TARGET + "S#Case/All.0", 1)
        ]

        self.discover()

        self.assertNotIn("limit", self.logged())

    def test_the_same_cluster_seen_in_several_windows_is_queried_once(
            self) -> None:
        self.summaries = [self.summary("rules", "r1", "crbug.com/1234567")]

        self.discover(30)

        self.assertEqual(self.client.cluster_failures.call_count, 1)

    def test_no_clusters_means_no_candidates(self) -> None:
        self.assertEqual(self.discover(), [])


class ReadHistoryTest(SuiteUpdaterTestCase):

    def test_collects_the_history_of_every_candidate(self) -> None:
        history = {"t1": [stats_group("h", passed=1)], "t2": []}
        self.client.history = mock.Mock(
            side_effect=lambda test_id, _window: history[test_id])

        self.assertEqual(self.updater().read_history(["t1", "t2"]), history)

    def test_every_candidate_shares_one_window(self) -> None:
        self.client.history = mock.Mock(return_value=[])

        self.updater().read_history(["t1", "t2", "t3"])

        windows = {c.args[1] for c in self.client.history.call_args_list}
        self.assertEqual(len(windows), 1)
        self.assertAlmostEqual(windows.pop().days, 30, places=3)

    def test_no_candidates_means_no_queries(self) -> None:
        self.client.history = mock.Mock()

        self.assertEqual(self.updater().read_history([]), {})
        self.client.history.assert_not_called()


class ResolveConfigsTest(SuiteUpdaterTestCase):

    def test_maps_each_variant_hash_to_a_config(self) -> None:
        self.client.variants = mock.Mock(return_value=[
            variant("h-linux", "Ubuntu-22.04", "linux-rel"),
            variant("h-asan", "Ubuntu-22.04", "linux-asan-rel"),
        ])

        config_by_hash = self.updater().resolve_configs(
            {"t1": [stats_group("h-linux", passed=1)]})

        self.assertEqual(config_by_hash, {
            "h-linux": "linux",
            "h-asan": "linux-asan"
        })

    def test_a_hash_is_only_looked_up_once(self) -> None:
        self.client.variants = mock.Mock(
            return_value=[variant("h-linux", "Ubuntu-22.04", "linux-rel")])

        self.updater().resolve_configs({
            "t1": [stats_group("h-linux", passed=1)],
            "t2": [stats_group("h-linux", passed=1)],
        })

        self.client.variants.assert_called_once_with("t1")

    def test_a_hash_the_api_does_not_return_is_not_looked_up_again(
            self) -> None:
        self.client.variants = mock.Mock(return_value=[])

        config_by_hash = self.updater().resolve_configs({
            "t1": [stats_group("h-gone", passed=1)],
            "t2": [stats_group("h-gone", passed=1)],
        })

        self.assertEqual(config_by_hash, {"h-gone": None})
        self.client.variants.assert_called_once_with("t1")


class RunTest(SuiteUpdaterTestCase):
    """A whole suite end to end, with the first three phases stubbed."""

    CONFIG_BY_HASH = {
        "h-linux": "linux",
        "h-asan": "linux-asan",
        "h-mac": "macos",
    }

    def setUp(self) -> None:
        super().setUp()
        self.stats_by_test_id: dict[str, list[StatsGroup]] = {}

    def update(self,
               suite: str = "unit_tests",
               days: int = 30,
               min_flake_rate: float = 0.01) -> None:
        """Run the suite, short-circuiting the phases that fetch."""
        updater = self.updater(suite, days, min_flake_rate)
        with mock.patch.multiple(
                updater,
                discover_candidates=mock.Mock(
                    side_effect=lambda: list(self.stats_by_test_id)),
                read_history=mock.Mock(
                    side_effect=lambda _ids: self.stats_by_test_id),
                resolve_configs=mock.Mock(
                    side_effect=lambda _history: self.CONFIG_BY_HASH)):
            updater.run()

    def written_files(self) -> list[str]:
        return sorted(path.name for path in self.filters_dir.iterdir())

    def read_filter(self, name: str) -> str:
        """Read a generated file without translating its line endings."""
        return (self.filters_dir / name).read_bytes().decode("utf-8")

    def excluded_tests(self, name: str) -> list[str]:
        return [
            line[1:] for line in self.read_filter(name).splitlines()
            if line.startswith("-")
        ]

    def test_platform_files_are_always_written(self) -> None:
        self.update()

        self.assertEqual(self.written_files(), [
            "unit_tests-linux.filter",
            "unit_tests-macos.filter",
            "unit_tests-windows.filter",
        ])

    def test_a_flaky_test_is_excluded_on_the_config_it_flakes_on(self) -> None:
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Flaky": [
                stats_group("h-linux", passed=90, failed=10),
                stats_group("h-mac", passed=100),
            ]
        }

        self.update()

        self.assertEqual(self.excluded_tests("unit_tests-linux.filter"),
                         ["S.Flaky"])
        self.assertEqual(self.excluded_tests("unit_tests-macos.filter"), [])

    def test_a_test_below_the_threshold_stays_enabled(self) -> None:
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Rare": [
                stats_group("h-linux", passed=999, failed=1)
            ]
        }

        self.update()

        self.assertEqual(self.excluded_tests("unit_tests-linux.filter"), [])

    def test_a_test_with_too_little_history_stays_enabled(self) -> None:
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Thin": [
                stats_group("h-linux", failed=ufm.MIN_MEANINGFUL_VERDICTS - 1)
            ]
        }

        self.update()

        self.assertEqual(self.excluded_tests("unit_tests-linux.filter"), [])

    def test_a_sanitizer_file_only_lists_what_the_platform_file_misses(
            self) -> None:
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Everywhere": [
                stats_group("h-linux", passed=90, failed=10),
                stats_group("h-asan", passed=90, failed=10),
            ],
            UNIT_TESTS_TARGET + "S#AsanOnly": [
                stats_group("h-linux", passed=100),
                stats_group("h-asan", passed=90, failed=10),
            ],
        }

        self.update()

        self.assertEqual(self.excluded_tests("unit_tests-linux.filter"),
                         ["S.Everywhere"])
        self.assertEqual(self.excluded_tests("unit_tests-linux-asan.filter"),
                         ["S.AsanOnly"])

    def test_an_empty_sanitizer_file_is_not_written(self) -> None:
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Flaky": [
                stats_group("h-linux", passed=90, failed=10)
            ]
        }

        self.update()

        self.assertNotIn("unit_tests-linux-asan.filter", self.written_files())

    def test_non_gtest_candidates_are_skipped(self) -> None:
        self.stats_by_test_id = {
            "://chrome/test\\:unit_tests!junit::org.Foo#bar": [
                stats_group("h-linux", failed=100)
            ]
        }

        self.update()

        self.assertEqual(self.excluded_tests("unit_tests-linux.filter"), [])

    def test_stale_files_of_the_same_suite_are_removed(self) -> None:
        (self.filters_dir / "unit_tests-linux-msan.filter").write_text(
            "old", encoding="utf-8", newline="")
        (self.filters_dir / "unit_tests.filter").write_text("older",
                                                            encoding="utf-8",
                                                            newline="")

        self.update()

        self.assertNotIn("unit_tests-linux-msan.filter", self.written_files())
        self.assertNotIn("unit_tests.filter", self.written_files())
        self.assertIn("removed stale unit_tests-linux-msan.filter",
                      self.logged())

    def test_files_of_other_suites_are_left_alone(self) -> None:
        (self.filters_dir / "browser_tests-linux.filter").write_text(
            "keep", encoding="utf-8", newline="")
        (self.filters_dir / "notes.txt").write_text("keep",
                                                    encoding="utf-8",
                                                    newline="")

        self.update()

        self.assertIn("browser_tests-linux.filter", self.written_files())
        self.assertIn("notes.txt", self.written_files())

    def test_the_output_directory_is_created_on_demand(self) -> None:
        nested = self.filters_dir / "generated"
        self.filters_dir = nested

        self.update()

        self.assertTrue(nested.is_dir())

    def test_generated_files_use_unix_line_endings_everywhere(self) -> None:
        # These files are committed, so a run on Windows has to produce
        # the same bytes as a run on Linux.
        self.stats_by_test_id = {
            UNIT_TESTS_TARGET + "S#Flaky": [
                stats_group("h-linux", passed=90, failed=10)
            ]
        }

        self.update()

        raw = (self.filters_dir / "unit_tests-linux.filter").read_bytes()
        self.assertIn(b"\n", raw)
        self.assertNotIn(b"\r\n", raw)

    def test_the_threshold_and_window_reach_the_file_header(self) -> None:
        self.update(days=45, min_flake_rate=0.025)

        header = self.read_filter("unit_tests-linux.filter")
        self.assertIn("flake rate >= 2.5% on", header)
        self.assertIn("past 45 days", header)


class MainTest(QuietConsoleTestCase):
    """main() wiring, with SuiteUpdater itself stubbed out."""

    def setUp(self) -> None:
        super().setUp()
        self.runs: list[Any] = []
        # What running a suite costs; replaced by tests that care.
        self.on_run: Any = lambda: None
        self.updater = self.enter_patch(
            mock.patch.object(ufm, "SuiteUpdater", side_effect=self.record))
        # main() builds its own display; hand it the one this fixture
        # can read back.
        self.enter_patch(
            mock.patch.object(ufm, "Display", return_value=self.display))

    def record(self, suite: str, **kwargs: Any) -> Any:
        """Stand in for a SuiteUpdater, remembering how it was built."""
        self.runs.append({"suite": suite, **kwargs})
        return mock.Mock(run=mock.Mock(side_effect=self.on_run))

    def run_main(self, *argv: str) -> None:
        with mock.patch.object(sys, "argv",
                               ["update-upstream-flake-filters.py", *argv]):
            return ufm.main()

    def updated_suites(self) -> list[str]:
        # Suites are worked on concurrently, so only the set is defined.
        return [run["suite"] for run in self.runs]

    def test_updates_every_suite_by_default(self) -> None:
        self.run_main()

        self.assertCountEqual(self.updated_suites(), ufm.DEFAULT_SUITES)

    def test_updates_only_the_requested_suites(self) -> None:
        self.run_main("unit_tests", "net_unittests")

        self.assertCountEqual(self.updated_suites(),
                              ["unit_tests", "net_unittests"])

    def test_converts_the_threshold_from_percent_to_a_fraction(self) -> None:
        self.run_main("unit_tests", "--days", "45", "--min-flake-rate", "2.5")

        self.assertEqual(len(self.runs), 1)
        self.assertEqual(self.runs[0]["suite"], "unit_tests")
        self.assertEqual(self.runs[0]["days"], 45)
        self.assertEqual(self.runs[0]["min_flake_rate"], 0.025)

    def test_every_suite_shares_one_client_and_display(self) -> None:
        self.run_main("unit_tests", "net_unittests", "base_unittests")

        self.assertEqual(len({id(run["client"]) for run in self.runs}), 1)
        self.assertEqual(len({id(run["display"]) for run in self.runs}), 1)

    def test_rejects_a_lookback_window_outside_the_supported_range(
            self) -> None:
        for days in ("0", "91"):
            with self.subTest(days=days):
                with self.assertRaises(SystemExit) as caught:
                    self.run_main("unit_tests", "--days", days)
                self.assertEqual(caught.exception.code, 1)
                self.assertEqual(self.runs, [])

    def test_counts_every_suite_off_the_summary(self) -> None:
        self.run_main("unit_tests", "net_unittests", "base_unittests")

        self.assertIn("suites:3/3", self.rendered())

    def test_works_on_several_suites_at_once(self) -> None:
        in_flight = []
        peak = []

        def on_run() -> None:
            in_flight.append(1)
            peak.append(len(in_flight))
            gevent.sleep(0.01)
            in_flight.pop()

        self.on_run = on_run

        self.run_main(*ufm.DEFAULT_SUITES)

        self.assertEqual(max(peak), ufm.SUITE_CONCURRENCY)


if __name__ == "__main__":
    unittest.main()
