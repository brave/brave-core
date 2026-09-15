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
"""

import argparse
import json
import sys

from luci_analysis import Flakiness, LuciAnalysis, LuciAnalysisError, Window

# The run's single connection to LUCI Analysis.
client = LuciAnalysis()

# What a test's history is called throughout this script.
Match = tuple[str, Flakiness]


def format_report_markdown(test_name: str, test_results: list[Match],
                           days: int) -> str:
    """Format the analysis results as human-readable markdown.

    Args:
        test_name: Original search string.
        test_results: List of (test_id, flakiness) pairs.
        days: Lookback window in days.

    Returns:
        Formatted markdown string.
    """
    lines: list[str] = []
    lines.append(f"# Upstream Flake Check: {test_name}")
    lines.append("")
    lines.append(f"Lookback period: {days} days")
    lines.append("Source: Chromium LUCI Analysis (analysis.api.luci.app)")
    lines.append("")

    if not test_results:
        lines.append("## Result: Not Found")
        lines.append("")
        lines.append(
            "No matching test IDs found in the Chromium LUCI Analysis database."
        )
        lines.append(
            "This test may be Brave-specific or use a different ID format.")
        return "\n".join(lines)

    for test_id, flakiness in test_results:
        counts = flakiness.counts
        lines.append(f"## Test: `{test_id}`")
        lines.append("")
        lines.append(f"### Verdict: {flakiness.verdict.headline}")
        lines.append("")
        lines.append(f"**Recommendation:** {flakiness.verdict.recommendation}")
        lines.append("")

        lines.append("### Statistics")
        lines.append("")
        lines.append("- Meaningful verdicts (pass+fail+flaky):"
                     f" {counts.meaningful}")
        lines.append(f"- Passed: {counts.passed}")
        lines.append(f"- Failed: {counts.failed}")
        lines.append(f"- Flaky: {counts.flaky}")
        if counts.skipped:
            lines.append(f"- Skipped: {counts.skipped}")
        if counts.execution_errored:
            lines.append(f"- Execution errors: {counts.execution_errored}")
        lines.append(f"- Flake rate: {flakiness.flake_rate:.1%}")
        lines.append("")

        if flakiness.daily:
            lines.append("### Daily Breakdown")
            lines.append("")
            lines.append("| Date | Total | Pass | Fail | Flaky | Rate |")
            lines.append("|------|-------|------|------|-------|------|")
            for day in flakiness.daily:
                rate = (f"{day.counts.flake_rate:.0%}"
                        if day.counts.meaningful else "N/A")
                lines.append(f"| {day.date} | {day.counts.total} "
                             f"| {day.counts.passed} | {day.counts.failed} "
                             f"| {day.counts.flaky} | {rate} |")
            lines.append("")

    return "\n".join(lines)


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
        help="Output in JSON format instead of markdown",
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
            print(format_report_markdown(test_name, [], days))
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
        print(format_report_markdown(test_name, test_results, days))

    sys.exit(0)


if __name__ == "__main__":
    try:
        main()
    except LuciAnalysisError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
