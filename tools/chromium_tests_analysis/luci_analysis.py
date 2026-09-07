# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Client for the Chromium LUCI Analysis REST API (pRPC protocol).

Provides helpers to query test history and flakiness statistics for
tests in the Chromium project, and to analyze the results.
"""

import http.client
import json
import threading
import time
from datetime import datetime, timedelta, timezone

LUCI_ANALYSIS_HOST = "analysis.api.luci.app"
TEST_HISTORY_SERVICE = "luci.analysis.v1.TestHistory"
CLUSTERS_SERVICE = "luci.analysis.v1.Clusters"
CHROMIUM_PROJECT = "chromium"

_thread_local = threading.local()


class LuciAnalysisError(Exception):
    """Raised when a LUCI Analysis API request fails."""


def _get_connection():
    """Return this thread's persistent API connection.

    Reusing connections avoids a TLS handshake per request, which adds
    up over the thousands of requests of a filter update.
    """
    connection = getattr(_thread_local, "connection", None)
    if connection is None:
        connection = http.client.HTTPSConnection(LUCI_ANALYSIS_HOST,
                                                 timeout=30)
        _thread_local.connection = connection
    return connection


def _drop_connection():
    """Close this thread's connection so the next request opens a new one."""
    connection = getattr(_thread_local, "connection", None)
    if connection is not None:
        connection.close()
        _thread_local.connection = None


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
    path = f"/prpc/{service}/{method}"
    data = json.dumps(body).encode("utf-8")
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json",
    }

    max_attempts = 3
    for attempt in range(1, max_attempts + 1):
        connection = _get_connection()
        try:
            connection.request("POST", path, body=data, headers=headers)
            response = connection.getresponse()
            status = response.status
            reason = response.reason
            raw = response.read()
        # OSError covers timeouts and connection resets; HTTPException
        # covers e.g. a keep-alive connection that the server has
        # meanwhile closed (RemoteDisconnected) or truncated reads.
        except (OSError, http.client.HTTPException) as e:
            _drop_connection()
            if attempt < max_attempts:
                time.sleep(2**attempt)
                continue
            raise LuciAnalysisError(
                f"Could not reach LUCI Analysis API: {e}") from e

        if status == 200:
            break
        if status == 403:
            raise LuciAnalysisError(
                "403 Forbidden from LUCI Analysis API. The API may"
                " require authentication for this query.")
        if status == 404:
            raise LuciAnalysisError(f"404 Not Found for method {method}.")
        # Retry transient server errors and rate limiting.
        if (status >= 500 or status == 429) and attempt < max_attempts:
            time.sleep(2**attempt)
            continue
        raise LuciAnalysisError(
            f"HTTP {status} from LUCI Analysis API: {reason}")

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
    partition_time_range = _partition_time_range(days)

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "predicate": {
                "partitionTimeRange": partition_time_range,
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


def get_test_variants(test_id):
    """Get the builder variants a test has run on.

    Args:
        test_id: Full LUCI test ID string.

    Returns:
        List of dicts with "variantHash" and "variant" (the definition,
        including the bot "os") from the API.
    """
    all_variants = []
    page_token = None

    while True:
        body = {
            "project": CHROMIUM_PROJECT,
            "testId": test_id,
            "pageSize": 1000,
        }
        if page_token:
            body["pageToken"] = page_token

        result = prpc_request(TEST_HISTORY_SERVICE, "QueryVariants", body)
        all_variants.extend(result.get("variants", []))

        page_token = result.get("nextPageToken")
        if not page_token:
            break

    return all_variants


def query_cluster_summaries(failure_filter, earliest, latest):
    """Query top failure clusters, ordered by failure count.

    Note: the API returns at most 200 clusters and does not paginate.

    Args:
        failure_filter: AIP-160 filter applied to each failure, e.g.
            'test_id:":browser_tests!gtest"' (substring match).
        earliest: datetime, start of the time range.
        latest: datetime, end of the time range.

    Returns:
        List of cluster summary dicts from the API.
    """
    body = {
        "project": CHROMIUM_PROJECT,
        "failureFilter": failure_filter,
        "orderBy": "metrics.`failures`.value desc",
        "metrics": [f"projects/{CHROMIUM_PROJECT}/metrics/failures"],
        "timeRange": {
            "earliest": earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "latest": latest.strftime("%Y-%m-%dT%H:%M:%SZ"),
        },
        "pageSize": 1000,
    }
    result = prpc_request(CLUSTERS_SERVICE, "QueryClusterSummaries", body)
    return result.get("clusterSummaries", [])


def query_cluster_failures(algorithm, cluster_id):
    """Query recent failure examples for a cluster.

    Args:
        algorithm: Clustering algorithm name (e.g., "testname-v4").
        cluster_id: Cluster ID string.

    Returns:
        List of failure example dicts (with exact test IDs) from the API.
    """
    parent = (f"projects/{CHROMIUM_PROJECT}/clusters/{algorithm}/"
              f"{cluster_id}/failures")
    result = prpc_request(CLUSTERS_SERVICE, "QueryClusterFailures",
                          {"parent": parent})
    return result.get("failures", [])


# Flake rate thresholds used to classify upstream flakiness. See
# docs/best-practices/testing-upstream-failures.md.
KNOWN_FLAKE_RATE = 0.05
OCCASIONAL_FLAKE_RATE = 0.01
MIN_MEANINGFUL_VERDICTS = 10


def classify_flakiness(flake_rate, meaningful_verdicts):
    """Classify a flake rate into a verdict and recommendation.

    Args:
        flake_rate: (failed + flaky) / meaningful verdicts.
        meaningful_verdicts: Number of passed + failed + flaky verdicts.

    Returns:
        Tuple of (verdict, recommendation) strings.
    """
    if meaningful_verdicts < MIN_MEANINGFUL_VERDICTS:
        return ("insufficient_data",
                "Cannot determine -- insufficient upstream"
                " data for this test in the"
                " lookback period.")
    if flake_rate >= KNOWN_FLAKE_RATE:
        return ("known_upstream_flake", "Safe to filter -- this test has a"
                " confirmed flakiness pattern"
                " in Chromium upstream.")
    if flake_rate >= OCCASIONAL_FLAKE_RATE:
        return ("occasional_upstream_failures",
                "Consider filtering -- test shows some"
                " upstream instability. Document"
                " findings in filter comment.")
    return ("stable_upstream", "Investigate Brave-specific causes --"
            " test appears stable in"
            " Chromium upstream.")


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

    verdict, recommendation = classify_flakiness(flake_rate,
                                                 meaningful_verdicts)

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

    verdict, recommendation = classify_flakiness(flake_rate,
                                                 meaningful_verdicts)

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
