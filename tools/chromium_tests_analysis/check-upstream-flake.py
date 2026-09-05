#!/usr/bin/env python3
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
    python3 tools/chromium_tests_analysis/check-upstream-flake.py \\
        "TestSuite.TestName" [--days 30] [--json]
"""

import argparse
import json
import sys

from luci_analysis import (
    LuciAnalysisError,
    analyze_stats,
    analyze_verdicts,
    get_flakiness_stats,
    get_test_verdicts,
    search_tests,
)


def format_report_markdown(test_name, test_results, days):
    """Format the analysis results as human-readable markdown.

    Args:
        test_name: Original search string.
        test_results: List of (test_id, analysis_dict) tuples.
        days: Lookback window in days.

    Returns:
        Formatted markdown string.
    """
    lines = []
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

    for test_id, analysis in test_results:
        lines.append(f"## Test: `{test_id}`")
        lines.append("")

        verdict_display = {
            "known_upstream_flake": "KNOWN UPSTREAM FLAKE",
            "occasional_upstream_failures": "OCCASIONAL UPSTREAM FAILURES",
            "stable_upstream": "STABLE UPSTREAM",
            "insufficient_data": "INSUFFICIENT DATA",
        }
        lines.append(
            "### Verdict: "
            f"{verdict_display.get(analysis['verdict'], analysis['verdict'])}")
        lines.append("")
        lines.append(f"**Recommendation:** {analysis['recommendation']}")
        lines.append("")

        lines.append("### Statistics")
        lines.append("")
        lines.append("- Meaningful verdicts (pass+fail+flaky):"
                     f" {analysis['meaningful_verdicts']}")
        lines.append(f"- Passed: {analysis['passed']}")
        lines.append(f"- Failed: {analysis['failed']}")
        lines.append(f"- Flaky: {analysis['flaky']}")
        if analysis.get('skipped', 0) > 0:
            lines.append(f"- Skipped: {analysis['skipped']}")
        if analysis.get('execution_errored', 0) > 0:
            lines.append(
                f"- Execution errors: {analysis['execution_errored']}")
        lines.append(f"- Flake rate: {analysis['flake_rate']:.1%}")
        lines.append("")

        if analysis["daily_breakdown"]:
            lines.append("### Daily Breakdown")
            lines.append("")
            lines.append("| Date | Total | Pass | Fail | Flaky | Rate |")
            lines.append("|------|-------|------|------|-------|------|")
            for day in analysis["daily_breakdown"]:
                day_meaningful = day["passed"] + day["failed"] + day["flaky"]
                if day_meaningful > 0:
                    day_rate = (day["failed"] + day["flaky"]) / day_meaningful
                    rate_str = f"{day_rate:.0%}"
                else:
                    rate_str = "N/A"
                lines.append(
                    f"| {day['date']} | {day['total']} | {day['passed']} "
                    f"| {day['failed']} | {day['flaky']} | {rate_str} |")
            lines.append("")

    return "\n".join(lines)


def format_report_json(test_name, test_results, days):
    """Format the analysis results as machine-readable JSON.

    Args:
        test_name: Original search string.
        test_results: List of (test_id, analysis_dict) tuples.
        days: Lookback window in days.

    Returns:
        JSON string.
    """
    output = {
        "test_name": test_name,
        "lookback_days": days,
        "matched_tests": [],
    }

    for test_id, analysis in test_results:
        output["matched_tests"].append({
            "test_id": test_id,
            **analysis,
        })

    # Overall verdict: use the worst verdict across all matched tests
    if test_results:
        verdict_priority = {
            "known_upstream_flake": 0,
            "occasional_upstream_failures": 1,
            "insufficient_data": 2,
            "stable_upstream": 3,
        }
        worst = min(test_results,
                    key=lambda t: verdict_priority.get(t[1]["verdict"], 99))
        output["overall_verdict"] = worst[1]["verdict"]
        output["overall_recommendation"] = worst[1]["recommendation"]
    else:
        output["overall_verdict"] = "not_found"
        output["overall_recommendation"] = (
            "Cannot determine -- test not found"
            " in Chromium LUCI Analysis database.")

    return json.dumps(output, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description=("Check if a test is a known upstream"
                     " flake in Chromium's LUCI Analysis"
                     " database."),
        epilog=("Example: python3"
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
    test_ids = search_tests(test_name)

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
    def relevance_sort_key(tid):
        # Exact suffix match is most relevant
        if tid.endswith("/" + test_name):
            return (0, tid)
        if tid.endswith(test_name):
            return (1, tid)
        return (2, tid)

    test_ids.sort(key=relevance_sort_key)
    test_ids = test_ids[:5]

    # Step 2: Get flakiness stats for each matched test
    test_results = []
    for test_id in test_ids:
        print(f"Fetching stats for: {test_id}", file=sys.stderr)
        stats = get_flakiness_stats(test_id, days)

        if stats:
            analysis = analyze_stats(stats)
        else:
            # Fallback: try Query endpoint for individual verdicts
            print("  No stats data, trying verdict query...", file=sys.stderr)
            verdicts = get_test_verdicts(test_id, days)
            if verdicts:
                analysis = analyze_verdicts(verdicts)
            else:
                analysis = {
                    "total_verdicts": 0,
                    "meaningful_verdicts": 0,
                    "passed": 0,
                    "failed": 0,
                    "flaky": 0,
                    "skipped": 0,
                    "execution_errored": 0,
                    "precluded": 0,
                    "flake_rate": 0.0,
                    "verdict": "insufficient_data",
                    "recommendation": ("Cannot determine -- no data"
                                       " found for this test ID in"
                                       " the lookback period."),
                    "daily_breakdown": [],
                }

        test_results.append((test_id, analysis))

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
