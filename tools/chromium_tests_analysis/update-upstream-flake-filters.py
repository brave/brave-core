#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Update the auto-generated filter files for flaky upstream tests.

Usage:
    python3 tools/chromium_tests_analysis/update-upstream-flake-filters.py \\
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

import argparse
import os
import sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone

from luci_analysis import (
    LuciAnalysisError,
    MIN_MEANINGFUL_VERDICTS,
    analyze_stats,
    get_flakiness_stats,
    get_test_variants,
    query_cluster_failures,
    query_cluster_summaries,
)

BRAVE_CORE_ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

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

GENERATED_FILTERS_DIR = os.path.join(BRAVE_CORE_ROOT, "test", "filters",
                                     "generated")

# Platforms Brave runs upstream test suites on, mapped to the "os"
# prefixes of the corresponding upstream bots. Bots for other platforms
# (e.g. ChromeOS) are ignored. The platform names must match those used by
# getApplicableFilters in build/commands/lib/testUtils.ts, which has no
# Android filters yet.
PLATFORM_OS_PREFIXES = {
    "linux": ("Ubuntu", "Linux"),
    "macos": ("Mac", ),
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

# For clusters spanning many tests (parameterized families, bug rules),
# only the test IDs with the most recent failures are checked
# individually.
MAX_VARIANTS_PER_CLUSTER = 100

STATS_WORKERS = 32


def log(message):
    print(message, file=sys.stderr)


def candidate_windows(days):
    """Time windows used for candidate discovery.

    The full lookback window plus weekly slices, to work around the
    200-cluster cap of QueryClusterSummaries.
    """
    now = datetime.now(timezone.utc)
    windows = [(now - timedelta(days=days), now)]
    start = 0
    while start < days:
        end = min(start + 7, days)
        windows.append(
            (now - timedelta(days=end), now - timedelta(days=start)))
        start = end
    return windows


def discovery_filters(suite):
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


def is_like_pattern(title):
    """Return True if a testname cluster title is a LIKE pattern.

    Titles of clusters that group multiple tests (e.g. parameterized
    variants) are SQL LIKE patterns with literals escaped by backslash.
    Titles of single-test clusters are verbatim test IDs, which never
    contain "\\\\", "\\_" or "%" (only "\\:" from flat test ID encoding).
    """
    return "%" in title or "\\\\" in title or "\\_" in title


def normalize_test_id(test_id):
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


def structured_id_to_gtest_name(test_id):
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


def collect_candidate_test_ids(suite, days):
    """Discover test IDs in a suite with recent upstream failures."""
    # Matches the ":<suite>!gtest" portion of structured test IDs like
    # "://chrome/test\:browser_tests!gtest::Suite#Case".
    suite_marker = f":{suite}!gtest"

    clusters = {}
    for earliest, latest in candidate_windows(days):
        summaries = []
        for failure_filter in discovery_filters(suite):
            summaries.extend(
                query_cluster_summaries(failure_filter, earliest, latest))
        for summary in summaries:
            cluster = summary["clusterId"]
            # "reason" clusters can be skipped because their failures
            # also count towards "testname" clusters. Failures claimed
            # by a bug ("rules" cluster) however are excluded from all
            # suggested clusters, so tracked flaky tests only show up
            # through their rules cluster.
            if cluster["algorithm"].startswith("reason"):
                continue
            clusters[(cluster["algorithm"], cluster["id"])] = \
                summary["title"]

    test_ids = set()
    multi_test_clusters = []
    for (algorithm, cluster_id), title in sorted(clusters.items()):
        if algorithm.startswith("testname") and not is_like_pattern(title):
            # The title is the verbatim test ID.
            if suite_marker in title:
                test_ids.add(normalize_test_id(title))
            continue
        multi_test_clusters.append((algorithm, cluster_id, title))

    # The remaining clusters group multiple test IDs (a parameterized
    # test family, or a bug rule matching failures from any number of
    # tests). Enumerate their recent failures to get exact IDs.
    def enumerate_cluster(cluster):
        algorithm, cluster_id, title = cluster
        failure_counts = Counter()
        for failure in query_cluster_failures(algorithm, cluster_id):
            test_id = failure.get("testId", "")
            if suite_marker in test_id:
                failure_counts[normalize_test_id(test_id)] += int(
                    failure.get("count", 1))
        return title, failure_counts

    with ThreadPoolExecutor(max_workers=STATS_WORKERS) as executor:
        for title, failure_counts in executor.map(enumerate_cluster,
                                                  multi_test_clusters):
            top = failure_counts.most_common(MAX_VARIANTS_PER_CLUSTER)
            dropped = len(failure_counts) - len(top)
            if dropped > 0:
                log(f"  Note: cluster '{title[:80]}' has "
                    f"{len(failure_counts)} recently failing variants; only "
                    f"checking the top {len(top)} by failure count.")
            test_ids.update(test_id for test_id, _ in top)

    return sorted(test_ids)


def fetch_candidate_stats(test_ids, days):
    """Fetch raw per-variant flakiness stats for each candidate test ID."""

    def fetch_one(test_id):
        return test_id, get_flakiness_stats(test_id, days)

    with ThreadPoolExecutor(max_workers=STATS_WORKERS) as executor:
        return dict(executor.map(fetch_one, test_ids))


def all_configs():
    """All test configs filter files are generated for."""
    for platform in PLATFORM_OS_PREFIXES:
        yield platform
        for sanitizer in SANITIZERS:
            yield f"{platform}-{sanitizer}"


def config_for_variant(variant_def):
    """Map an upstream bot variant to a Brave test config.

    Returns e.g. "linux" or "linux-asan", or None for configs Brave
    doesn't run (e.g. ChromeOS, Android or TSan bots).
    """
    platform = None
    os_name = variant_def.get("os", "")
    for candidate, prefixes in PLATFORM_OS_PREFIXES.items():
        if os_name.startswith(prefixes):
            platform = candidate
            break
    if platform is None:
        return None
    # Chromium's flake-retry bots run as builder "runner" in bucket
    # "reviver"; the bot whose failures they retry is in "reviver_builder".
    builder_names = (variant_def.get("builder", ""),
                     variant_def.get("reviver_builder", ""))
    builder = " ".join(filter(None, builder_names)).lower()
    if any(keyword in builder for keyword in EXCLUDED_BUILDER_KEYWORDS):
        return None
    for sanitizer in SANITIZERS:
        if sanitizer in builder:
            return f"{platform}-{sanitizer}"
    if "lsan" in builder:
        # LSan runs as a mode of ASan bots.
        return f"{platform}-asan"
    return platform


def resolve_variant_configs(stats_by_test_id):
    """Map each variant hash seen in the stats to a Brave test config.

    Variant hashes are shared between tests that run on the same bot
    config, so one QueryVariants call typically resolves the hashes of
    most tests in a suite; further calls are only made for tests whose
    stats contain still-unknown hashes.
    """
    config_by_hash = {}
    for test_id, groups in stats_by_test_id.items():
        if all(g.get("variantHash") in config_by_hash for g in groups):
            continue
        for entry in get_test_variants(test_id):
            variant_def = entry.get("variant", {}).get("def", {})
            config_by_hash[entry["variantHash"]] = \
                config_for_variant(variant_def)
        # Don't re-query for hashes QueryVariants didn't return.
        for group in groups:
            config_by_hash.setdefault(group.get("variantHash"), None)
    return config_by_hash


def analyze_per_config(groups, config_by_hash):
    """Compute per-config flakiness analyses from raw stats groups."""
    analyses = {}
    for config in all_configs():
        config_groups = [
            g for g in groups
            if config_by_hash.get(g.get("variantHash")) == config
        ]
        analyses[config] = analyze_stats(config_groups)
    return analyses


def build_filter_content(suite, config, entries, days, min_flake_rate):
    """Build the content of a generated filter file.

    Args:
        suite: Test suite name.
        config: Brave test config name (e.g. "linux" or "linux-asan").
        entries: List of (gtest_name, analysis_dict) tuples.
        days: Lookback window in days.
        min_flake_rate: Flake rate threshold (fraction).

    Returns:
        The filter file content string.
    """
    lines = [
        "## AUTO-GENERATED FILE -- DO NOT EDIT.",
        "##",
        f"## Upstream {suite} tests with a flake rate >="
        f" {min_flake_rate:.1%} on",
        f"## {config} bots over the past {days} days per Chromium LUCI"
        " Analysis.",
        "## Regenerate with:",
        "##   python3 tools/chromium_tests_analysis/"
        "update-upstream-flake-filters.py",
    ]
    for gtest_name, analysis in sorted(entries):
        lines.append("")
        lines.append(f"# {analysis['flake_rate']:.1%} flake rate over"
                     f" {days} days per LUCI Analysis"
                     f" ({analysis['passed']} passed,"
                     f" {analysis['failed']} failed,"
                     f" {analysis['flaky']} flaky).")
        lines.append(f"-{gtest_name}")
    return "\n".join(lines) + "\n"


def update_suite_filters(suite, days, min_flake_rate):
    """Regenerate the per-config filter files for one suite."""
    log(f"[{suite}] Discovering candidate flaky tests...")
    test_ids = collect_candidate_test_ids(suite, days)
    log(f"[{suite}] Checking flake rate of {len(test_ids)} candidates...")
    stats_by_test_id = fetch_candidate_stats(test_ids, days)
    config_by_hash = resolve_variant_configs(stats_by_test_id)

    entries_by_config = {config: [] for config in all_configs()}
    for test_id, groups in stats_by_test_id.items():
        gtest_name = structured_id_to_gtest_name(test_id)
        if not gtest_name:
            continue
        analyses = analyze_per_config(groups, config_by_hash)
        for config, analysis in analyses.items():
            if analysis["meaningful_verdicts"] < MIN_MEANINGFUL_VERDICTS:
                continue
            if analysis["flake_rate"] < min_flake_rate:
                continue
            entries_by_config[config].append((gtest_name, analysis))

    # Platform filter files also apply to sanitizer runs, so sanitizer
    # files only need the tests that aren't already filtered for the
    # platform in general.
    for platform in PLATFORM_OS_PREFIXES:
        platform_names = {name for name, _ in entries_by_config[platform]}
        for sanitizer in SANITIZERS:
            config = f"{platform}-{sanitizer}"
            entries_by_config[config] = [
                entry for entry in entries_by_config[config]
                if entry[0] not in platform_names
            ]

    os.makedirs(GENERATED_FILTERS_DIR, exist_ok=True)
    written = set()
    for config, entries in entries_by_config.items():
        # Platform files are always written; sanitizer files only when
        # they have entries.
        if config not in PLATFORM_OS_PREFIXES and not entries:
            continue
        filename = f"{suite}-{config}.filter"
        path = os.path.join(GENERATED_FILTERS_DIR, filename)
        with open(path, "w", encoding="utf-8") as f:
            f.write(
                build_filter_content(suite, config, entries, days,
                                     min_flake_rate))
        written.add(filename)
        log(f"[{suite}] Wrote {len(entries)} entries to"
            f" {os.path.relpath(path, BRAVE_CORE_ROOT)}")

    # Remove files from previous runs that were not regenerated, e.g. a
    # sanitizer file whose tests all dropped below the threshold.
    for filename in os.listdir(GENERATED_FILTERS_DIR):
        is_stale = (filename.startswith(f"{suite}-")
                    or filename == f"{suite}.filter")
        if is_stale and filename.endswith(".filter") \
                and filename not in written:
            os.remove(os.path.join(GENERATED_FILTERS_DIR, filename))
            log(f"[{suite}] Removed stale {filename}")


def main():
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
        help="Number of days to look back (default: 30, max: 90)",
    )
    parser.add_argument(
        "--min-flake-rate",
        type=float,
        default=1.0,
        help="Flake rate threshold in percent (default: 1.0)",
    )
    args = parser.parse_args()

    if args.days < 1 or args.days > 90:
        print("Error: --days must be between 1 and 90.", file=sys.stderr)
        sys.exit(1)

    for suite in args.suites:
        update_suite_filters(suite, args.days, args.min_flake_rate / 100.0)


if __name__ == "__main__":
    try:
        main()
    except LuciAnalysisError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
