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
    python3 scripts/check-upstream-flake.py \\
        "TestSuite.TestName" [--days 30] [--json]
"""

import argparse
import json
import sys
import urllib.request
import urllib.error
from datetime import datetime, timedelta, timezone

LUCI_ANALYSIS_HOST = "https://analysis.api.luci.app"
PRPC_SERVICE = "luci.analysis.v1.TestHistory"
CHROMIUM_PROJECT = "chromium"
_ALLOWED_SCHEMES = ("https://", )

# pRPC responses are prefixed with )]}'\n (5 bytes) as XSSI protection
PRPC_PREFIX = b")]}'\\n"  # Will handle both literal and actual newline


def _safe_urlopen(req, **kwargs):
    """Wrapper around urllib.request.urlopen that validates URL scheme.

    Prevents file:// and other dangerous schemes from being used.
    """
    url = req.full_url if isinstance(req, urllib.request.Request) else req
    if not any(url.startswith(s) for s in _ALLOWED_SCHEMES):
        raise ValueError(f"URL scheme not allowed: {url}")
    return urllib.request.urlopen(req, **kwargs)  # nosemgrep


def prpc_request(method, body):
    """Make a pRPC request to the LUCI Analysis API.

    Args:
        method: RPC method name (e.g., "QueryTests")
        body: dict to send as JSON request body

    Returns:
        Parsed JSON response dict.

    Raises:
        SystemExit on fatal errors (auth, network).
    """
    url = f"{LUCI_ANALYSIS_HOST}/prpc/{PRPC_SERVICE}/{method}"
    data = json.dumps(body).encode("utf-8")

    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "application/json",
            "Accept": "application/json",
        },
        method="POST",
    )

    try:
        with _safe_urlopen(req, timeout=30) as resp:
            raw = resp.read()
    except urllib.error.HTTPError as e:
        if e.code == 403:
            print(
                "Error: 403 Forbidden from LUCI Analysis API. "
                "The API may require authentication for this query.",
                file=sys.stderr,
            )
            sys.exit(1)
        elif e.code == 404:
            print(
                f"Error: 404 Not Found for method {method}.",
                file=sys.stderr,
            )
            sys.exit(1)
        else:
            print(
                f"Error: HTTP {e.code} from LUCI Analysis API: {e.reason}",
                file=sys.stderr,
            )
            sys.exit(1)
    except urllib.error.URLError as e:
        print(f"Error: Could not connect to LUCI Analysis API: {e.reason}",
              file=sys.stderr)
        sys.exit(1)

    # Strip the pRPC XSSI prefix. The prefix is )]}' followed by a newline.
    # Find the first newline and skip everything up to and including it.
    newline_idx = raw.find(b"\n")
    if newline_idx >= 0:
        raw = raw[newline_idx + 1:]

    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        print("Error: Could not parse API response as JSON.", file=sys.stderr)
        print(f"Raw response (first 500 bytes): {raw[:500]}", file=sys.stderr)
        sys.exit(1)


def search_tests(test_name_substring):
    """Search for test IDs matching a substring.

    Returns:
        List of full test ID strings.
    """
    all_test_ids = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testIdSubstring": test_name_substring,
            "pageSize": 100,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request("QueryTests", body)
        test_ids = result.get("testIds", [])
        all_test_ids.extend(test_ids)

        page_token = result.get("nextPageToken")
        if not page_token:
            break

    return all_test_ids


def get_flakiness_stats(test_id, days):
    """Get flakiness statistics for a test over a time range.

    Args:
        test_id: Full LUCI test ID string.
        days: Number of days to look back.

    Returns:
        List of stat group dicts from the API.
    """
    now = datetime.now(timezone.utc)
    earliest = now - timedelta(days=days)

    all_groups = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "predicate": {
                "partitionTimeRange": {
                    "earliest": earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
                    "latest": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
                }
            },
            "pageSize": 1000,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request("QueryStats", body)
        groups = result.get("groups", [])
        all_groups.extend(groups)

        page_token = result.get("nextPageToken")
        if not page_token:
            break

    return all_groups


def get_test_verdicts(test_id, days):
    """Get individual test verdicts for a test over a time range.

    Useful when QueryStats returns empty but we want to check Query directly.

    Args:
        test_id: Full LUCI test ID string.
        days: Number of days to look back.

    Returns:
        List of verdict dicts from the API.
    """
    now = datetime.now(timezone.utc)
    earliest = now - timedelta(days=days)

    all_verdicts = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "predicate": {
                "partitionTimeRange": {
                    "earliest": earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
                    "latest": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
                }
            },
            "pageSize": 1000,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request("Query", body)
        verdicts = result.get("verdicts", [])
        all_verdicts.extend(verdicts)

        page_token = result.get("nextPageToken")
        if not page_token:
            break

    return all_verdicts


def analyze_stats(stats_groups):
    """Analyze raw stats groups into an aggregate summary.

    The API returns one entry per (partitionTime, variantHash) pair.
    We aggregate across all variants per day for the daily breakdown.

    Args:
        stats_groups: List of stat group dicts from QueryStats.

    Returns:
        Dict with aggregated statistics and verdict.
    """
    total_passed = 0
    total_failed = 0
    total_flaky = 0
    total_skipped = 0
    total_execution_errored = 0
    total_precluded = 0

    # Aggregate by date (across all variant hashes)
    daily_agg = {}

    for group in stats_groups:
        # Date is in partitionTime as ISO timestamp
        # (e.g., "2026-02-07T00:00:00Z")
        partition_time = group.get("partitionTime", "")
        date_str = partition_time[:10] if partition_time else "unknown"

        vc = group.get("verdictCounts", {})
        passed = int(vc.get("passed", 0))
        failed = int(vc.get("failed", 0))
        flaky = int(vc.get("flaky", 0))
        skipped = int(vc.get("skipped", 0))
        execution_errored = int(vc.get("executionErrored", 0))
        precluded = int(vc.get("precluded", 0))

        total_passed += passed
        total_failed += failed
        total_flaky += flaky
        total_skipped += skipped
        total_execution_errored += execution_errored
        total_precluded += precluded

        if date_str not in daily_agg:
            daily_agg[date_str] = {
                "date": date_str,
                "passed": 0,
                "failed": 0,
                "flaky": 0,
                "skipped": 0,
                "execution_errored": 0,
                "precluded": 0,
                "total": 0,
            }
        day = daily_agg[date_str]
        day["passed"] += passed
        day["failed"] += failed
        day["flaky"] += flaky
        day["skipped"] += skipped
        day["execution_errored"] += execution_errored
        day["precluded"] += precluded
        day["total"] += (passed + failed + flaky + skipped +
                         execution_errored + precluded)

    daily_data = [v for v in daily_agg.values() if v["total"] > 0]

    total_verdicts = (total_passed + total_failed + total_flaky +
                      total_skipped + total_execution_errored +
                      total_precluded)
    # Flake rate: count of (flaky + failed) vs total
    # meaningful verdicts (excluding skipped/precluded)
    meaningful_verdicts = total_passed + total_failed + total_flaky
    if meaningful_verdicts > 0:
        flake_rate = (total_flaky + total_failed) / meaningful_verdicts
    else:
        flake_rate = 0.0

    # Determine verdict
    if meaningful_verdicts < 10:
        verdict = "insufficient_data"
        recommendation = ("Cannot determine -- insufficient upstream"
                          " data for this test in the"
                          " lookback period.")
    elif flake_rate >= 0.05:
        verdict = "known_upstream_flake"
        recommendation = ("Safe to filter -- this test has a"
                          " confirmed flakiness pattern"
                          " in Chromium upstream.")
    elif flake_rate >= 0.01:
        verdict = "occasional_upstream_failures"
        recommendation = ("Consider filtering -- test shows some"
                          " upstream instability. Document"
                          " findings in filter comment.")
    else:
        verdict = "stable_upstream"
        recommendation = ("Investigate Brave-specific causes --"
                          " test appears stable in"
                          " Chromium upstream.")

    # Sort daily data by date
    daily_data.sort(key=lambda d: d["date"])

    return {
        "total_verdicts": total_verdicts,
        "meaningful_verdicts": meaningful_verdicts,
        "passed": total_passed,
        "failed": total_failed,
        "flaky": total_flaky,
        "skipped": total_skipped,
        "execution_errored": total_execution_errored,
        "precluded": total_precluded,
        "flake_rate": flake_rate,
        "verdict": verdict,
        "recommendation": recommendation,
        "daily_breakdown": daily_data,
    }


def analyze_verdicts(verdicts):
    """Analyze raw verdicts from the Query endpoint as a fallback.

    Args:
        verdicts: List of verdict dicts from Query.

    Returns:
        Dict with aggregated statistics and verdict.
    """
    total_passed = 0
    total_failed = 0
    total_flaky = 0
    total_skipped = 0
    total_other = 0

    for v in verdicts:
        status = v.get("status", "").upper()
        if status == "PASSED":
            total_passed += 1
        elif status == "FAILED":
            total_failed += 1
        elif status == "FLAKY":
            total_flaky += 1
        elif status == "SKIPPED":
            total_skipped += 1
        else:
            total_other += 1

    total_verdicts = (total_passed + total_failed + total_flaky +
                      total_skipped + total_other)
    meaningful_verdicts = total_passed + total_failed + total_flaky
    if meaningful_verdicts > 0:
        flake_rate = (total_flaky + total_failed) / meaningful_verdicts
    else:
        flake_rate = 0.0

    if meaningful_verdicts < 10:
        verdict = "insufficient_data"
        recommendation = ("Cannot determine -- insufficient upstream"
                          " data for this test in the"
                          " lookback period.")
    elif flake_rate >= 0.05:
        verdict = "known_upstream_flake"
        recommendation = ("Safe to filter -- this test has a"
                          " confirmed flakiness pattern"
                          " in Chromium upstream.")
    elif flake_rate >= 0.01:
        verdict = "occasional_upstream_failures"
        recommendation = ("Consider filtering -- test shows some"
                          " upstream instability. Document"
                          " findings in filter comment.")
    else:
        verdict = "stable_upstream"
        recommendation = ("Investigate Brave-specific causes --"
                          " test appears stable in"
                          " Chromium upstream.")

    return {
        "total_verdicts": total_verdicts,
        "meaningful_verdicts": meaningful_verdicts,
        "passed": total_passed,
        "failed": total_failed,
        "flaky": total_flaky,
        "skipped": total_skipped,
        "execution_errored": 0,
        "precluded": 0,
        "flake_rate": flake_rate,
        "verdict": verdict,
        "recommendation": recommendation,
        "daily_breakdown": [],
    }


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
                " scripts/check-upstream-flake.py"
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
    main()
