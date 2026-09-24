#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Check if a test is a known upstream flake in the Chromium
LUCI Analysis database.

Queries the LUCI Analysis REST API (pRPC protocol) to retrieve
flakiness statistics for a given test in the Chromium project.

Usage:
    vpython3 tools/chromium_tests_analysis/check-upstream-flake.py \\
        "TestSuite.TestName" [--days 30] [--json]

The report goes to stdout as tables; progress goes to stderr, so the
two can be separated. `--json` publishes a machine-readable shape that
other tooling depends on.
"""

from __future__ import annotations

import argparse
import json
import sys

from rich import box
from rich.console import Console
from rich.table import Table
from rich.text import Text

from luci_analysis import (
    LUCI_ANALYSIS_HOST,
    Flakiness,
    LuciAnalysis,
    LuciAnalysisError,
    Window,
)

# The run's single connection to LUCI Analysis.
client = LuciAnalysis()

# The report is this tool's product, so it goes to stdout; progress
# chatter goes to stderr.
console = Console()

# What a test's history is called throughout this script.
Match = tuple[str, Flakiness]


def matches_table(test_results: list[Match], days: int) -> Table:
    """One row per matched test, so they can be read against each other.

    Args:
        test_results: The (test_id, flakiness) pairs to tabulate.
        days: Lookback window in days, named in the title.
    """
    table = Table(title=f"Upstream flakiness over the past {days} days",
                  caption=f"Source: Chromium LUCI Analysis "
                  f"({LUCI_ANALYSIS_HOST})",
                  box=box.SIMPLE_HEAD,
                  row_styles=["none", "dim"],
                  title_style="bold",
                  caption_justify="right")
    # Test IDs are long; let the column fold rather than push the
    # numbers out of alignment.
    table.add_column("Test", overflow="fold", ratio=1)
    table.add_column("Verdict", no_wrap=True)
    table.add_column("Rate", justify="right", no_wrap=True)
    table.add_column("Pass", justify="right", no_wrap=True)
    table.add_column("Fail", justify="right", no_wrap=True)
    table.add_column("Flaky", justify="right", no_wrap=True)
    table.add_column("Meaningful", justify="right", no_wrap=True)

    for test_id, flakiness in test_results:
        counts = flakiness.counts
        table.add_row(
            test_id,
            Text(flakiness.verdict.headline, style=flakiness.verdict.style),
            f"{flakiness.flake_rate:.1%}",
            f"{counts.passed:,}",
            f"{counts.failed:,}",
            f"{counts.flaky:,}",
            f"{counts.meaningful:,}",
        )
    return table


def daily_table(flakiness: Flakiness) -> Table:
    """A test's day-by-day history.

    The test it belongs to is printed above rather than titling the
    table: an ID is far wider than these columns and would wrap.

    Args:
        flakiness: The test's history; only `daily` is read.
    """
    table = Table(box=box.SIMPLE_HEAD, row_styles=["none", "dim"])
    table.add_column("Date", no_wrap=True)
    for heading in ("Total", "Pass", "Fail", "Flaky", "Rate"):
        table.add_column(heading, justify="right", no_wrap=True)

    for day in flakiness.daily:
        counts = day.counts
        table.add_row(
            day.date,
            f"{counts.total:,}",
            f"{counts.passed:,}",
            f"{counts.failed:,}",
            f"{counts.flaky:,}",
            f"{counts.flake_rate:.0%}" if counts.meaningful else "--",
        )
    return table


def render_report(test_name: str, test_results: list[Match],
                  days: int) -> None:
    """Print the human-readable report.

    Args:
        test_name: Original search string.
        test_results: List of (test_id, flakiness) pairs.
        days: Lookback window in days.
    """
    if not test_results:
        console.print(
            f"No test matching [bold]{test_name}[/] was found in Chromium"
            " LUCI Analysis. It may be Brave-specific, or use a different"
            " ID format.")
        return

    console.print()
    console.print(matches_table(test_results, days))

    # One line per verdict present, rather than repeating the same
    # advice under every test that shares it.
    console.print()
    for verdict in sorted({f.verdict
                           for _, f in test_results},
                          key=lambda v: v.severity):
        console.print(
            Text(verdict.headline,
                 style=verdict.style).append(f": {verdict.recommendation}",
                                             style="none"))

    for test_id, flakiness in test_results:
        if flakiness.daily:
            console.print()
            console.print(Text(test_id, style="italic"), overflow="fold")
            console.print(daily_table(flakiness))


def format_report_json(test_name: str, test_results: list[Match],
                       days: int) -> str:
    """Format the analysis results as machine-readable JSON.

    Args:
        test_name: Original search string.
        test_results: List of (test_id, flakiness) pairs.
        days: Lookback window in days.

    Returns:
        JSON string.
    """
    output = {
        "test_name": test_name,
        "lookback_days": days,
        "matched_tests": [{
            "test_id": test_id,
            **flakiness.as_json(),
        } for test_id, flakiness in test_results],
    }

    if test_results:
        worst = min(test_results, key=lambda m: m[1].verdict.severity)[1]
        output["overall_verdict"] = worst.verdict.value
        output["overall_recommendation"] = worst.verdict.recommendation
    else:
        output["overall_verdict"] = "not_found"
        output["overall_recommendation"] = (
            "Cannot determine -- test not found"
            " in Chromium LUCI Analysis database.")

    return json.dumps(output, indent=2)


def main() -> None:
    parser = argparse.ArgumentParser(
        description=("Check if a test is a known upstream"
                     " flake in Chromium's LUCI Analysis"
                     " database."),
        epilog=("Example: vpython3"
                " tools/chromium_tests_analysis/check-upstream-flake.py"
                " 'WebUIURLLoaderFactoryTest"
                ".RangeRequest'"),
    )
    parser.add_argument(
        "test_name",
        help=
        "Test name or substring to search for (e.g., 'TestSuite.TestMethod')",
    )
    parser.add_argument(
        "--days",
        type=int,
        default=30,
        help="Number of days to look back (default: 30, max: 90)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="json_output",
        help="Output in JSON format instead of tables",
    )
    args = parser.parse_args()

    if args.days < 1 or args.days > 90:
        print("Error: --days must be between 1 and 90.", file=sys.stderr)
        sys.exit(1)

    test_name = args.test_name
    days = args.days

    # Step 1: Search for matching test IDs
    print(f"Searching for '{test_name}' in Chromium LUCI Analysis...",
          file=sys.stderr)
    test_ids = client.tests_matching(test_name)

    if not test_ids:
        print("No matching test IDs found.", file=sys.stderr)
        if args.json_output:
            print(format_report_json(test_name, [], days))
        else:
            render_report(test_name, [], days)
        sys.exit(2)

    print(f"Found {len(test_ids)} matching test ID(s).", file=sys.stderr)

    # Limit to top 5 most relevant matches
    # Prefer exact matches (test name at the end of the ID)
    def relevance_sort_key(tid: str) -> tuple[int, str]:
        # Exact suffix match is most relevant
        if tid.endswith("/" + test_name):
            return (0, tid)
        if tid.endswith(test_name):
            return (1, tid)
        return (2, tid)

    test_ids.sort(key=relevance_sort_key)
    test_ids = test_ids[:5]

    # Step 2: read each matched test's history
    window = Window.last_days(days)
    test_results: list[Match] = []
    for test_id in test_ids:
        print(f"Fetching stats for: {test_id}", file=sys.stderr)
        groups = client.history(test_id, window)
        if groups:
            flakiness = Flakiness.of_groups(groups)
        else:
            # Fallback: ask for individual verdicts instead. An empty
            # Flakiness reads as "insufficient data" on its own.
            print("  No stats data, trying verdict query...", file=sys.stderr)
            flakiness = Flakiness.of_verdicts(client.verdicts(test_id, window))

        test_results.append((test_id, flakiness))

    # Step 3: Output report
    if args.json_output:
        print(format_report_json(test_name, test_results, days))
    else:
        render_report(test_name, test_results, days)

    sys.exit(0)


if __name__ == "__main__":
    try:
        main()
    except LuciAnalysisError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
