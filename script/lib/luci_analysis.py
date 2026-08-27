#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Client for the Chromium LUCI Analysis REST API (pRPC protocol).

Provides helpers to query test history and flakiness statistics for
tests in the Chromium project, and to analyze the results.
"""

import json
import urllib.request
import urllib.error
from datetime import datetime, timedelta, timezone

LUCI_ANALYSIS_HOST = "https://analysis.api.luci.app"
TEST_HISTORY_SERVICE = "luci.analysis.v1.TestHistory"
CHROMIUM_PROJECT = "chromium"
_ALLOWED_SCHEMES = ("https://", )


class LuciAnalysisError(Exception):
    """Raised when a LUCI Analysis API request fails."""


def _safe_urlopen(req, **kwargs):
    """Wrapper around urllib.request.urlopen that validates URL scheme.

    Prevents file:// and other dangerous schemes from being used.
    """
    url = req.full_url if isinstance(req, urllib.request.Request) else req
    if not any(url.startswith(s) for s in _ALLOWED_SCHEMES):
        raise ValueError(f"URL scheme not allowed: {url}")
    return urllib.request.urlopen(req, **kwargs)  # nosemgrep


def prpc_request(service, method, body):
    """Make a pRPC request to the LUCI Analysis API.

    Args:
        service: Full pRPC service name
            (e.g., "luci.analysis.v1.TestHistory").
        method: RPC method name (e.g., "QueryTests").
        body: dict to send as JSON request body.

    Returns:
        Parsed JSON response dict.

    Raises:
        LuciAnalysisError on auth, network or parse errors.
    """
    url = f"{LUCI_ANALYSIS_HOST}/prpc/{service}/{method}"
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
            raise LuciAnalysisError(
                "403 Forbidden from LUCI Analysis API. The API may require"
                " authentication for this query.") from e
        if e.code == 404:
            raise LuciAnalysisError(f"404 Not Found for method {method}.") \
                from e
        raise LuciAnalysisError(
            f"HTTP {e.code} from LUCI Analysis API: {e.reason}") from e
    except urllib.error.URLError as e:
        raise LuciAnalysisError(
            f"Could not connect to LUCI Analysis API: {e.reason}") from e

    # Strip the pRPC XSSI prefix. The prefix is )]}' followed by a newline.
    # Find the first newline and skip everything up to and including it.
    newline_idx = raw.find(b"\n")
    if newline_idx >= 0:
        raw = raw[newline_idx + 1:]

    try:
        return json.loads(raw)
    except json.JSONDecodeError as e:
        raise LuciAnalysisError(
            "Could not parse API response as JSON. Raw response (first 500"
            f" bytes): {raw[:500]}") from e


def _partition_time_range(days):
    """Build a partitionTimeRange dict covering the last `days` days."""
    now = datetime.now(timezone.utc)
    earliest = now - timedelta(days=days)
    return {
        "earliest": earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "latest": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
    }


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

        result = prpc_request(TEST_HISTORY_SERVICE, "QueryTests", body)
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
    all_groups = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "predicate": {
                "partitionTimeRange": _partition_time_range(days),
            },
            "pageSize": 1000,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request(TEST_HISTORY_SERVICE, "QueryStats", body)
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
    all_verdicts = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "predicate": {
                "partitionTimeRange": _partition_time_range(days),
            },
            "pageSize": 1000,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request(TEST_HISTORY_SERVICE, "Query", body)
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
