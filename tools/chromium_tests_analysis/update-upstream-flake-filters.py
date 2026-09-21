#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Update the auto-generated filter files for flaky upstream tests.

Usage:
    vpython3 tools/chromium_tests_analysis/update-upstream-flake-filters.py \\
        [suite ...] [--days 30] [--min-flake-rate 1.0]

N.B.: The generated list of flaky tests is not exhaustive because the
discovery APIs we are using (see below) cap the number of result items.

The script works as follows:

For each upstream test suite that Brave runs (browser_tests, unit_tests, etc.),
the script finds tests whose flake rate in Chromium's LUCI Analysis data
exceeds the threshold over the lookback period and writes them to
test/filters/generated/<suite>-<config>.filter, where <config> is a platform
("linux") or a platform-sanitizer combination ("linux-asan"). Flake rates are
computed per config from the matching upstream bots, so a test only flaky on
e.g. Linux ASan bots is only filtered there. The files are picked up
automatically by `pnpm run test` (see build/commands/lib/testUtils.ts).

Candidate test discovery uses the following clustering APIs. For more
information on LUCI's data model, see README.md in this directory.

1.  `Clusters.QueryClusterSummaries` returns the clusters with the most
    failures, filtered to failures of the suite. The API caps this at the top
    200 clusters, so the script queries the whole lookback period plus each week
    of it separately to widen the net.
2.  For a single-test cluster, the test ID can be read off the cluster title.
    For clusters spanning several tests (parameterized tests, rule clusters),
    `Clusters.QueryClusterFailures` lists recent failures inside the cluster,
    and the script collects the test IDs with the most failures from them.

    Note that this second step always looks at the last 7 days, whatever
    `--days` says: the window is hardcoded upstream and the RPC takes no time
    range. A longer lookback therefore widens the flake rates computed in steps
    3 and 4, but not the discovery of candidates through multi-test clusters.

Any test with a meaningful flake rate must fail regularly, so it is expected to
surface in one of these clusters: in its test name cluster, or, if a bug is
already filed on it, in the bug's rule cluster.

The check then computes the flake rate of each test found during discovery:

3.  `TestHistory.QueryStats` returns the candidate's verdict counts over the
    lookback period, broken down by day and variant.
4.  `TestHistory.QueryVariants` maps each variant to its builder and OS. The
    script uses this to group the verdict counts into the configurations Brave
    tests (platform x sanitizer), dropping bots for configurations Brave never
    runs (ChromeOS, Android, iOS, ...).

For each configuration, the rate of failed or flaky verdicts among all
meaningful verdicts (passed, failed or flaky) is computed. Tests at or above the
threshold (default: 1% over 30 days, with at least 10 meaningful verdicts) are
written to `test/filters/generated/<suite>-<configuration>.filter`. Note that
this includes tests that consistently fail upstream, not only flaky ones.
"""

from __future__ import annotations

# Requests run as greenlets, so `socket` and `ssl` have to be cooperative
# before `http.client` is imported anywhere below. Nothing else may
# precede this.
from gevent import monkey

monkey.patch_all()

# pylint: disable=wrong-import-position
import argparse
import os
import sys
import time
from collections import Counter
from collections.abc import Callable, Iterable, Iterator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, TypeVar

from gevent.pool import Pool
from rich.console import Console, Group
from rich.live import Live
from rich.markup import escape
from rich.progress import (BarColumn, Progress, ProgressColumn, SpinnerColumn,
                           Task, TaskID, TextColumn, TimeElapsedColumn)
from rich.table import Column
from rich.text import Text

from luci_analysis import (
    MAX_CLUSTER_FAILURES,
    MIN_MEANINGFUL_VERDICTS,
    ClusterSummary,
    Flakiness,
    LuciAnalysis,
    LuciAnalysisError,
    RequestStats,
    StatsGroup,
    TestVariant,
    Window,
)

# The item a batch of parallel work is applied to, and what it yields.
T = TypeVar("T")
R = TypeVar("R")

BRAVE_CORE_ROOT = Path(__file__).resolve().parent.parent.parent

# Upstream test suites Brave runs on CI.
DEFAULT_SUITES = [
    "base_unittests",
    "browser_tests",
    "components_unittests",
    "content_unittests",
    "installer_util_unittests",
    "net_unittests",
    "services_unittests",
    "setup_unittests",
    "unit_tests",
]

GENERATED_FILTERS_DIR = BRAVE_CORE_ROOT / "test" / "filters" / "generated"

# Platforms Brave runs upstream test suites on, mapped to the "os"
# prefixes of the corresponding upstream bots. Bots for other platforms
# (e.g. ChromeOS, and Mac -- Brave doesn't run upstream tests on Mac) are
# ignored. The platform names must match those used by
# getApplicableFilters in build/commands/lib/testUtils.ts, which has no
# Android filters yet.
PLATFORM_OS_PREFIXES = {
    "linux": ("Ubuntu", "Linux"),
    "windows": ("Windows", ),
}

# Sanitizers Brave runs upstream test suites with, identified by
# substrings of upstream builder names. The names must match those used
# by getApplicableFilters in build/commands/lib/testUtils.ts.
SANITIZERS = ("asan", "msan", "ubsan")

# Bots whose builder name contains one of these don't correspond to any
# generated filter file: Brave doesn't build for ChromeOS/Fuchsia or run
# tests under TSan, and Android/iOS have no filter files. These bots can't
# be excluded via PLATFORM_OS_PREFIXES because Android emulator bots report
# the Linux host "os" and iOS bots a Mac "os".
EXCLUDED_BUILDER_KEYWORDS = ("android", "chromeos", "chromium os", "fuchsia",
                             "ios", "tsan")

# Requests in flight across the whole run. Every batch of work shares one
# pool.
#
# Raising this does not necessarily  means a faster run and in fact it can slow
# things as the service absorbs a deeper queue, making everyone wait longer.
REQUEST_CONCURRENCY = 64

# Suites worked on at once.
SUITE_CONCURRENCY = 3

# Everything the script renders goes to stderr, leaving stdout free. `log_path`
# is off because every log line comes from `log` below, so the source location
# rich would print is the same useless one every time.
console = Console(stderr=True, log_path=False)

# Width of the progress description column..
DESCRIPTION_WIDTH = 50

# Width of the completed/total column, right-aligned..
COUNT_WIDTH = len("9999/9999")

# How often the live display repaints, in Hz.
REFRESH_RATE = 10


class CountColumn(ProgressColumn):
    """A progress column showing "completed/total".

    `MofNCompleteColumn` pads `completed` out to the width of `total`, and this
    causes rows whose totals differ in length to not line up with each other.
    Leaving that padding out and right-aligning the column instead lines every
    row up.
    """

    def render(self, task: Task) -> Text:
        total = int(task.total) if task.total is not None else "?"
        return Text(f"{int(task.completed)}/{total}",
                    style="progress.download")


@dataclass(frozen=True)
class Phase:
    """One unit of work with a progress row of its own."""

    # The display's progress table, which owns the row.
    progress: Progress

    # Identifies this phase's row within it.
    task_id: TaskID

    def advance(self, count: int = 1) -> None:
        """Mark `count` more items of this phase as done."""
        self.progress.advance(self.task_id, count)


class Display:
    """Run-wide counters above a progress row per phase in flight.

    Laid out the way siso reports a build: one summary line that keeps counting
    while the work proceeds, and under it a row for each task currently running.
    """

    def __init__(self, stats: RequestStats) -> None:
        # The counters the summary line reports; a client's, in a run.
        self._stats = stats

        self._progress = Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}",
                       table_column=Column(width=DESCRIPTION_WIDTH,
                                           no_wrap=True,
                                           overflow="ellipsis")),
            BarColumn(),
            CountColumn(table_column=Column(
                min_width=COUNT_WIDTH, justify="right", no_wrap=True)),
            TimeElapsedColumn(),
            # The enclosing `Live` drives the repaints; a `Progress` that
            # refreshed itself would start a second live region.
            auto_refresh=False,
        )

        # When the run began, for the elapsed time and the rate. Reset by
        # `live`, so constructing a Display early costs nothing.
        self._started = time.monotonic()

        # Suites finished, and how many there are in total.
        self._suites_done = 0
        self._suites_total = 0

        # Totals of the phases that have finished and left the display, so the
        # summary keeps counting them.
        self._retired_done = 0
        self._retired_total = 0

    def __rich__(self) -> Group:
        return Group(self._summary(), self._progress)

    @property
    def tasks(self) -> list[Task]:
        """The progress rows currently on screen."""
        return self._progress.tasks

    def _units(self) -> tuple[int, int]:
        """Items done and known, over every phase of the run."""
        done = self._retired_done + sum(
            int(task.completed) for task in self.tasks)
        total = self._retired_total + sum(
            int(task.total or 0) for task in self.tasks)
        return done, total

    def _summary(self) -> Text:
        """The top line: how the run as a whole is getting on."""
        elapsed = time.monotonic() - self._started
        done, total = self._units()
        rate = self._stats.completed / elapsed if elapsed > 0 else 0.0
        parts = [
            f"[bold][{done}/{total}][/]",
            f"{elapsed:.2f}s",
            f"suites:{self._suites_done}/{self._suites_total}",
            f"active:{len(self.tasks)}",
            f"inflight:{self._stats.in_flight}",
            f"retries:{self._stats.retries}",
            f"{rate:.1f}/s",
        ]
        if self._stats.failures:
            parts.append(f"[red]errors:{self._stats.failures}[/]")
        return Text.from_markup(" ".join(parts))

    @contextmanager
    def live(self, suites_total: int) -> Iterator["Display"]:
        """Show the display until the run finishes."""
        self._started = time.monotonic()
        self._suites_total = suites_total
        with Live(self,
                  console=console,
                  refresh_per_second=REFRESH_RATE,
                  transient=False):
            yield self

    def suite_finished(self) -> None:
        """Count one suite off the summary line."""
        self._suites_done += 1

    @contextmanager
    def phase(self, suite: str, description: str,
              total: int) -> Iterator[Phase]:
        """Show a row for one phase of work on `suite`.

        The row goes away once the phase ends, but its counts stay in the
        summary line above.

        Args:
            suite: The test suite the phase belongs to.
            description: What the phase is doing, e.g. "fetching test
                history".
            total: How many items the phase has to get through.
        """
        task_id = self._progress.add_task(f"[bold]{suite}[/] {description}",
                                          total=total)
        try:
            yield Phase(self._progress, task_id)
        finally:
            task = self._progress.tasks[self._progress.task_ids.index(task_id)]
            self._retired_done += int(task.completed)
            self._retired_total += int(task.total or 0)
            self._progress.remove_task(task_id)


def log(message: str) -> None:
    """Log a line above the live display."""
    console.log(message)


def display_path(path: Path) -> str:
    """The path as written for a human, relative to the repo when it can be.

    A filters dir outside the checkout -- a temporary one under a test,
    say -- can sit on another Windows drive, which relpath refuses.
    """
    try:
        return os.path.relpath(path, BRAVE_CORE_ROOT)
    except ValueError:
        return str(path)


@contextmanager
def worker_pool(size: int = REQUEST_CONCURRENCY) -> Iterator[Pool]:
    """A greenlet pool that gives up promptly when the work is abandoned.

    Killing the pool unwinds its greenlets wherever they are, including out of a
    socket read, so an interrupt does not have to wait for the requests already
    on the wire.
    """
    pool = Pool(size)
    try:
        yield pool
    except BaseException:
        pool.kill(block=False)
        raise
    pool.join()


# Every API request of the run goes through this pool.
requests_pool = Pool(REQUEST_CONCURRENCY)


def run_in_parallel(function: Callable[[T], R],
                    items: Iterable[T]) -> Iterator[R]:
    """Apply `function` across `items` concurrently, yielding in order.

    The greenlet that feeds the pool sits outside it, so killing the pool alone
    would leave it spawning more work. Killing it when the caller stops
    consuming, on an interrupt, or any other error, shuts the fan-out down with
    them.
    """
    mapping = requests_pool.imap(function, items)
    try:
        yield from mapping
    finally:
        mapping.kill()


def get_candidate_windows(days: int) -> list[Window]:
    """Time windows used for candidate discovery.

    The full lookback window plus weekly slices, to work around the
    200-cluster cap of QueryClusterSummaries.
    """
    window = Window.last_days(days)
    return [window] + window.split_weekly()


def get_discovery_filters(suite: str) -> Iterator[str]:
    """Failure filters used for candidate discovery.

    One fleet-wide filter, plus filters scoped to each sanitizer and
    platform. QueryClusterSummaries caps each query at the top 200
    clusters by failure count, so without scoping, configs with few bots
    (e.g. linux-asan) would have to compete with the whole fleet and
    their flaky tests would rarely rank.
    """
    base = f'test_id:":{suite}!gtest"'
    yield base
    for sanitizer in SANITIZERS:
        yield f'{base} variant.builder:"{sanitizer}"'
    for os_prefixes in PLATFORM_OS_PREFIXES.values():
        for os_prefix in os_prefixes:
            yield f'{base} variant.os:"{os_prefix}"'


def normalize_test_id(test_id: str) -> str:
    """Map WebUI JS sub-result test IDs to their parent test case.

    WebUI browser tests report JS sub-results as separate
    "<case>__<sub_result>" test IDs. Only the parent "<case>" is an
    actual gtest case that can be filtered and has full history stats.
    """
    head, sep, fine = test_id.partition("#")
    if not sep:
        return test_id
    case, slash, param = fine.partition("/")
    case = case.split("__")[0]
    return head + "#" + case + (slash + param if slash else "")


def structured_id_to_gtest_name(test_id: str) -> str | None:
    """Convert a structured LUCI test ID to a gtest test name.

    Examples:
        "://chrome/test\\:browser_tests!gtest::Suite#Case"
            -> "Suite.Case"
        "://chrome/test\\:browser_tests!gtest::Suite#Case/Inst.Param"
            -> "Inst/Suite.Case/Param"

    Returns:
        The gtest name, or None if the ID is not a gtest test ID.
    """
    _, sep, fine = test_id.partition("!gtest::")
    if not sep:
        return None
    suite, sep, case = fine.partition("#")
    if not sep:
        return None
    case, slash, param = case.partition("/")
    if not slash:
        return f"{suite}.{case}"
    instantiation, dot, value = param.partition(".")
    if not dot:
        # Value-parameterized without instantiation prefix.
        return f"{suite}.{case}/{param}"
    return f"{instantiation}/{suite}.{case}/{value}"


def get_all_configs() -> Iterator[str]:
    """All test configs filter files are generated for."""
    for platform in PLATFORM_OS_PREFIXES:
        yield platform
        for sanitizer in SANITIZERS:
            yield f"{platform}-{sanitizer}"


def get_config_for_variant(variant: TestVariant) -> str | None:
    """Map an upstream bot variant to a Brave test config.

    Returns e.g. "linux" or "linux-asan", or None for configs Brave
    doesn't run (e.g. ChromeOS, Android or TSan bots).
    """
    platform = None
    for candidate, prefixes in PLATFORM_OS_PREFIXES.items():
        if variant.os.startswith(prefixes):
            platform = candidate
            break
    if platform is None:
        return None
    builder = variant.builder_description
    if any(keyword in builder for keyword in EXCLUDED_BUILDER_KEYWORDS):
        return None
    for sanitizer in SANITIZERS:
        if sanitizer in builder:
            return f"{platform}-{sanitizer}"
    if "lsan" in builder:
        # LSan runs as a mode of ASan bots.
        return f"{platform}-asan"
    return platform


def analyze_per_config(
        groups: list[StatsGroup],
        config_by_hash: dict[str, str | None]) -> dict[str, Flakiness]:
    """Read a test's history separately for each config it ran on.

    Args:
        groups: The test's per-day, per-variant history.
        config_by_hash: Which Brave config each variant hash belongs to,
            or None for bots Brave does not test.
    """
    return {
        config: Flakiness.of_groups(
            g for g in groups if config_by_hash.get(g.variant_hash) == config)
        for config in get_all_configs()
    }


class SuiteUpdater:
    """The work of bringing one suite's filter files up to date.

    A run is four phases: a) discover candidates, b) read their history, c) work
    out which config each ran on, and d) write the files. Each stage needs the
    same handful of things: a client to ask, a display to report to, and the
    suite and thresholds being worked to.
    """

    def __init__(self, suite: str, *, client: LuciAnalysis, display: Display,
                 days: int, min_flake_rate: float, filters_dir: Path) -> None:

        # The upstream test suite being brought up to date.
        self._suite = suite

        # Who to ask about LUCI; shared with every other suite in the run.
        self._client = client

        # Where to report progress; likewise shared.
        self._display = display

        # How far back to look. Note this does not widen cluster-based
        # discovery, which upstream fixes at 7 days.
        self._days = days

        # Flake rate, as a fraction, at or above which a test is
        # excluded on a given config.
        self._min_flake_rate = min_flake_rate

        # Directory the .filter files are written to.
        self._filters_dir = filters_dir

        # Matches the ":<suite>!gtest" portion of structured test IDs
        # like "://chrome/test\:browser_tests!gtest::Suite#Case".
        self._suite_marker = f":{suite}!gtest"

    def run(self) -> None:
        """Bring this suite's filter files up to date."""
        test_ids = self.discover_candidates()
        history = self.read_history(test_ids)
        config_by_hash = self.resolve_configs(history)
        self.write_filters(self.entries_by_config(history, config_by_hash))

    # -- reporting

    def _phase(self, description: str,
               total: int) -> AbstractContextManager[Phase]:
        """A progress row for this suite, over `total` items."""
        return self._display.phase(self._suite, description, total)

    def _log(self, message: str) -> None:
        """Log a line about this suite."""
        log(f"[bold]{self._suite}[/] {message}")

    # -- the phases

    def discover_candidates(self) -> list[str]:
        """Find test IDs in this suite with recent upstream failures."""
        clusters = self._query_clusters()

        test_ids: set[str] = set()
        multi_test_clusters: list[ClusterSummary] = []
        for _, summary in sorted(clusters.items()):
            if summary.names_one_test:
                if self._suite_marker in summary.title:
                    test_ids.add(normalize_test_id(summary.title))
                continue
            multi_test_clusters.append(summary)

        test_ids.update(self._enumerate_clusters(multi_test_clusters))
        return sorted(test_ids)

    def _query_clusters(self) -> dict[tuple[str, str], ClusterSummary]:
        """The failure clusters this suite shows up in."""
        # One query per (window, filter) pair. Run as a batch: they are
        # independent, and serially they dominate a suite's wall time.
        queries = [(failure_filter, window)
                   for window in get_candidate_windows(self._days)
                   for failure_filter in get_discovery_filters(self._suite)]

        clusters: dict[tuple[str, str], ClusterSummary] = {}
        with self._phase("querying failure clusters", len(queries)) as task:
            for summaries in run_in_parallel(
                    lambda query: self._client.cluster_summaries(*query),
                    queries):
                for summary in summaries:
                    # Failures in a "reason" cluster also count towards
                    # a test name cluster, so following both up would
                    # repeat work. Failures claimed by a bug ("rules"
                    # cluster) are excluded from every suggested cluster
                    # though, so a tracked flaky test only shows up
                    # through its rule.
                    if not summary.groups_by_failure_reason:
                        clusters[summary.key] = summary
                task.advance()
        return clusters

    def _enumerate_clusters(self, clusters: list[ClusterSummary]) -> set[str]:
        """Read the test IDs out of clusters that cover several tests.

        These are parameterised test families and bug rules, whose
        titles name no single test, so their recent failures have to be
        listed to find out what is in them.
        """
        test_ids: set[str] = set()
        # Every test a cluster names is checked. What is still missed is
        # upstream's doing, so report that instead.
        truncated_clusters = 0

        with self._phase("enumerating clusters", len(clusters)) as task:
            for failure_counts, truncated in run_in_parallel(
                    self._failures_by_test, clusters):
                truncated_clusters += truncated
                test_ids.update(failure_counts)
                task.advance()

        if truncated_clusters:
            self._log(f"{truncated_clusters} clusters hit the server's"
                      f" {MAX_CLUSTER_FAILURES}-failure limit, which does not"
                      " paginate: whatever else they hold cannot be reached"
                      " through this API.")
        return test_ids

    def _failures_by_test(
            self, cluster: ClusterSummary) -> tuple[Counter[str], bool]:
        """How often each of this suite's tests failed inside a cluster.

        Returns:
            The per-test failure counts, and whether the server returned
            its maximum -- in which case the cluster holds more than it
            was willing to say.
        """
        failures = self._client.cluster_failures(cluster)
        failure_counts: Counter[str] = Counter()
        for failure in failures:
            if self._suite_marker in failure.test_id:
                failure_counts[normalize_test_id(
                    failure.test_id)] += failure.count
        return failure_counts, len(failures) >= MAX_CLUSTER_FAILURES

    def read_history(self, test_ids: list[str]) -> dict[str, list[StatsGroup]]:
        """Fetch the per-day, per-variant history of each candidate."""
        window = Window.last_days(self._days)

        def fetch_one(test_id: str) -> tuple[str, list[StatsGroup]]:
            return test_id, self._client.history(test_id, window)

        history: dict[str, list[StatsGroup]] = {}
        with self._phase("fetching test history", len(test_ids)) as task:
            for test_id, groups in run_in_parallel(fetch_one, test_ids):
                history[test_id] = groups
                task.advance()
        return history

    def resolve_configs(
            self, history: dict[str,
                                list[StatsGroup]]) -> dict[str, str | None]:
        """Map each variant hash in the history to a Brave test config.

        Variant hashes are shared between tests that run on the same bot
        config, so one QueryVariants call typically resolves the hashes
        of most tests in a suite; further calls are only made for tests
        whose history holds still-unknown hashes.
        """
        config_by_hash: dict[str, str | None] = {}
        with self._phase("resolving bot variants", len(history)) as task:
            for test_id, groups in history.items():
                if not all(g.variant_hash in config_by_hash for g in groups):
                    for variant in self._client.variants(test_id):
                        config_by_hash[variant.variant_hash] = \
                            get_config_for_variant(variant)
                    # Don't re-query hashes QueryVariants didn't return.
                    for group in groups:
                        config_by_hash.setdefault(group.variant_hash, None)
                task.advance()
        return config_by_hash

    def entries_by_config(
        self, history: dict[str,
                            list[StatsGroup]], config_by_hash: dict[str,
                                                                    str | None]
    ) -> dict[str, list[tuple[str, Flakiness]]]:
        """Pick out which tests to exclude, on which config.

        Args:
            history: Each candidate's per-day, per-variant history.
            config_by_hash: Which Brave config each variant hash belongs
                to, or None for bots Brave does not test.
        """
        entries: dict[str, list[tuple[str, Flakiness]]] = {
            config: []
            for config in get_all_configs()
        }
        for test_id, groups in history.items():
            gtest_name = structured_id_to_gtest_name(test_id)
            if not gtest_name:
                continue
            for config, flakiness in analyze_per_config(
                    groups, config_by_hash).items():
                if flakiness.counts.meaningful < MIN_MEANINGFUL_VERDICTS:
                    continue
                if flakiness.flake_rate < self._min_flake_rate:
                    continue
                entries[config].append((gtest_name, flakiness))

        # Platform filter files also apply to sanitizer runs, so
        # sanitizer files only need the tests that aren't already
        # filtered for the platform in general.
        for platform in PLATFORM_OS_PREFIXES:
            platform_names = {name for name, _ in entries[platform]}
            for sanitizer in SANITIZERS:
                config = f"{platform}-{sanitizer}"
                entries[config] = [
                    entry for entry in entries[config]
                    if entry[0] not in platform_names
                ]
        return entries

    # -- writing the files

    def write_filters(
            self,
            entries_by_config: dict[str, list[tuple[str, Flakiness]]]) -> None:
        """Write this suite's filter files, and retire the stale ones."""
        self._filters_dir.mkdir(parents=True, exist_ok=True)
        written: set[str] = set()
        for config, entries in entries_by_config.items():
            # Platform files are always written. Sanitizer files only
            # when they have entries.
            if config not in PLATFORM_OS_PREFIXES and not entries:
                continue
            filename = f"{self._suite}-{config}.filter"
            path = self._filters_dir / filename
            # These files are committed, so they have to come out byte
            # for byte the same wherever the script is run: `newline=""`
            # keeps Windows from turning every "\n" into "\r\n".
            path.write_text(self.filter_content(config, entries),
                            encoding="utf-8",
                            newline="")
            written.add(filename)
            self._log(f"wrote {len(entries)} entries to"
                      f" {display_path(path)}")
        self._remove_stale_filters(written)

    def _remove_stale_filters(self, written: set[str]) -> None:
        """Drop files a previous run left behind.

        A sanitizer file whose tests all dropped below the threshold,
        for instance, is no longer regenerated and would otherwise keep
        excluding them for ever.
        """
        for path in self._filters_dir.iterdir():
            is_stale = (path.name.startswith(f"{self._suite}-")
                        or path.name == f"{self._suite}.filter")
            if is_stale and path.suffix == ".filter" \
                    and path.name not in written:
                path.unlink()
                self._log(f"removed stale {path.name}")

    def filter_content(self, config: str,
                       entries: list[tuple[str, Flakiness]]) -> str:
        """The text of one generated filter file.

        Args:
            config: Brave test config name, e.g. "linux" or "linux-asan".
            entries: The (gtest_name, flakiness) pairs to exclude.
        """
        lines = [
            "## AUTO-GENERATED FILE -- DO NOT EDIT.",
            "##",
            f"## Upstream {self._suite} tests with a flake rate >="
            f" {self._min_flake_rate:.1%} on",
            f"## {config} bots over the past {self._days} days per Chromium"
            " LUCI Analysis.",
            "## Regenerate with:",
            "##   vpython3 tools/chromium_tests_analysis/"
            "update-upstream-flake-filters.py",
        ]
        for gtest_name, flakiness in sorted(entries, key=lambda e: e[0]):
            counts = flakiness.counts
            lines.append("")
            lines.append(f"# {flakiness.flake_rate:.1%} flake rate over"
                         f" {self._days} days per LUCI Analysis"
                         f" ({counts.passed} passed,"
                         f" {counts.failed} failed,"
                         f" {counts.flaky} flaky).")
            lines.append(f"-{gtest_name}")
        return "\n".join(lines) + "\n"


def handle_interrupt(client: LuciAnalysis) -> NoReturn:
    """Leave straight away after Ctrl+C.

    The suite pool is killed as the interrupt unwinds through
    `worker_pool`, but the request greenlets those suites had already
    spawned belong to `requests_pool` and have to be killed here, along
    with telling the client to stop retrying. Killing a greenlet unwinds
    it wherever it is, a blocked socket read included, so nothing is
    left holding the process. Each filter file is written and closed
    before the next one starts, so none is left half-written.
    """
    client.request_shutdown()
    requests_pool.kill(block=False)
    console.print("[yellow]Interrupted.[/] Filter files already written are"
                  " left in place.")
    sys.exit(130)


def main() -> None:
    parser = argparse.ArgumentParser(
        description=("Update test/filters/generated/*.filter with upstream"
                     " tests that are flaky per Chromium LUCI Analysis."))
    parser.add_argument(
        "suites",
        nargs="*",
        default=DEFAULT_SUITES,
        help=f"Test suites to update (default: {' '.join(DEFAULT_SUITES)})",
    )
    parser.add_argument(
        "--days",
        type=int,
        default=30,
        help=("Number of days to look back (default: 30, max: 90). Note"
              " that discovery through multi-test clusters is fixed at 7"
              " days upstream and does not widen with this."),
    )
    parser.add_argument(
        "--min-flake-rate",
        type=float,
        default=1.0,
        help="Flake rate threshold in percent (default: 1.0)",
    )
    args = parser.parse_args()

    if args.days < 1 or args.days > 90:
        console.print("[red]Error:[/] --days must be between 1 and 90.")
        sys.exit(1)

    suites = args.suites
    min_flake_rate = args.min_flake_rate / 100.0

    # One connection to LUCI and one display for the whole run, shared
    # by every suite so the fan-out stays capped and the summary line
    # counts everything.
    client = LuciAnalysis()
    display = Display(client.stats)

    def update_one(suite: str) -> None:
        SuiteUpdater(suite,
                     client=client,
                     display=display,
                     days=args.days,
                     min_flake_rate=min_flake_rate,
                     filters_dir=GENERATED_FILTERS_DIR).run()

    try:
        with display.live(len(suites)):
            with worker_pool(SUITE_CONCURRENCY) as suite_pool:
                for _ in suite_pool.imap_unordered(update_one, suites):
                    display.suite_finished()
    except KeyboardInterrupt:
        handle_interrupt(client)


if __name__ == "__main__":
    try:
        main()
    except LuciAnalysisError as e:
        console.print(f"[red]Error:[/] {escape(str(e))}")
        sys.exit(1)
