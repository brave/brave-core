#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for luci_analysis.py.

Split the way the module is: the domain half needs no fixture at all,
while the client half runs against a scripted transport.
"""

from __future__ import annotations

import http.client
import json
import ssl
import sys
import unittest
from collections import deque
from datetime import date, datetime, timezone
from pathlib import Path
from typing import Any
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

# pylint: disable=wrong-import-position
from luci_analysis import (
    MAX_PAGE_SIZE,
    MAX_STABILITY_BATCH,
    ClusterFailure,
    ClusterSummary,
    ConnectionPool,
    FlakyDay,
    JsonDict,
    LuciAnalysis,
    LuciAnalysisError,
    PooledFlakes,
    TLS_CONTEXT,
    Request,
    RequestStats,
    Source,
    Stability,
    Status,
    TestVariant,
    Window,
    parse_commit_position,
    parse_milestone,
    weekdays_back,
)

BRANCH = Source("checkout", "abc", "refs/branch-heads/8103", 1035, 157)

# -- The domain half -----------------------------------------------------


class WindowTest(unittest.TestCase):
    def test_last_days_ends_now(self) -> None:
        window = Window.last_days(30)

        self.assertAlmostEqual(window.days, 30, places=3)
        now = datetime.now(timezone.utc)
        self.assertAlmostEqual(
            (now - window.latest).total_seconds(), 0, delta=5
        )

    def test_weekly_slices_are_contiguous_and_cover_the_window(self) -> None:
        window = Window.last_days(30)

        slices = window.split_weekly()

        self.assertEqual(len(slices), 5)
        self.assertEqual(slices[0].earliest, window.earliest)
        self.assertEqual(slices[-1].latest, window.latest)
        for before, after in zip(slices, slices[1:]):
            self.assertEqual(before.latest, after.earliest)

    def test_no_slice_is_longer_than_a_week(self) -> None:
        for slice_ in Window.last_days(30).split_weekly():
            self.assertLessEqual(slice_.days, 7.001)

    def test_a_window_shorter_than_a_week_is_one_slice(self) -> None:
        window = Window.last_days(3)

        self.assertEqual(window.split_weekly(), [window])

    def test_renders_the_clusters_time_range(self) -> None:
        window = Window(
            datetime(2026, 1, 1, tzinfo=timezone.utc),
            datetime(2026, 1, 8, tzinfo=timezone.utc),
        )

        self.assertEqual(
            window.as_time_range(),
            {
                "earliest": "2026-01-01T00:00:00Z",
                "latest": "2026-01-08T00:00:00Z",
            },
        )


class SourceTest(unittest.TestCase):
    def test_reads_the_commit_position_trailer(self) -> None:
        message = (
            "Subject\n\nCr-Commit-Position: refs/branch-heads/8103@{#1035}\n"
        )

        self.assertEqual(
            parse_commit_position(message), ("refs/branch-heads/8103", 1035)
        )

    def test_a_commit_without_a_position_is_an_error(self) -> None:
        with self.assertRaises(LuciAnalysisError):
            parse_commit_position("Subject\n")

    def test_reads_the_milestone_off_chrome_version(self) -> None:
        self.assertEqual(parse_milestone("MAJOR=157\nMINOR=0\n"), 157)

    def test_a_branch_lives_in_its_milestone_project(self) -> None:
        self.assertEqual(BRANCH.project, "chromium-m157")

    def test_main_lives_in_the_chromium_project(self) -> None:
        main = Source("main", "abc", "refs/heads/main", 1, 158)

        self.assertEqual(main.project, "chromium")

    def test_any_other_ref_has_no_project(self) -> None:
        with self.assertRaises(LuciAnalysisError):
            _ = Source("x", "abc", "refs/heads/other", 1, 1).project

    def test_describes_itself_as_ref_and_position(self) -> None:
        self.assertEqual(BRANCH.description, "refs/branch-heads/8103@{#1035}")


# A Tuesday, so the last two weekdays are that day and the Monday.
TUESDAY = date(2026, 11, 10)
CRITERIA = {"flakeRate": {"flakeThreshold": 3, "flakeThreshold1wd": 1}}


def flaky_day(day: date, flaky: int, variant_hash: str = "h") -> FlakyDay:
    return FlakyDay(day=day, variant_hash=variant_hash, flaky=flaky)


class WeekdaysBackTest(unittest.TestCase):
    def test_counts_today_when_it_is_a_weekday(self) -> None:
        self.assertEqual(weekdays_back(TUESDAY, 2), date(2026, 11, 9))

    def test_reaches_over_a_weekend(self) -> None:
        monday = date(2026, 11, 9)

        self.assertEqual(weekdays_back(monday, 2), date(2026, 11, 6))

    def test_a_weekend_day_does_not_count(self) -> None:
        sunday = date(2026, 11, 8)

        self.assertEqual(weekdays_back(sunday, 2), date(2026, 11, 5))


class PooledFlakesTest(unittest.TestCase):
    def tally(self, *days: FlakyDay) -> PooledFlakes:
        return PooledFlakes.tally(list(days), CRITERIA, TUESDAY)

    def test_sums_every_bot_and_day(self) -> None:
        flakes = self.tally(
            flaky_day(date(2026, 11, 3), 1, "h1"),
            flaky_day(date(2026, 11, 9), 1, "h2"),
            flaky_day(date(2026, 11, 10), 2, "h1"),
        )

        self.assertEqual((flakes.flaky, flakes.flaky_1wd), (4, 3))
        self.assertTrue(flakes.met)

    def test_needs_a_recent_flake(self) -> None:
        flakes = self.tally(flaky_day(date(2026, 11, 5), 5))

        self.assertEqual(flakes.flaky_1wd, 0)
        self.assertFalse(flakes.met)

    def test_needs_enough_flakes(self) -> None:
        self.assertFalse(self.tally(flaky_day(TUESDAY, 2)).met)

    def test_days_before_the_window_are_ignored(self) -> None:
        flakes = self.tally(
            flaky_day(date(2026, 10, 27), 10), flaky_day(TUESDAY, 1)
        )

        self.assertEqual(flakes.flaky, 1)

    def test_without_criteria_nothing_is_met(self) -> None:
        flakes = PooledFlakes.tally([flaky_day(TUESDAY, 9)], {}, TUESDAY)

        self.assertFalse(flakes.met)


class StabilityTest(unittest.TestCase):
    @staticmethod
    def status(analysis: JsonDict) -> Status:
        return Stability("t", {}, analysis).status

    def test_a_met_flake_rate_is_flaky(self) -> None:
        self.assertIs(self.status({"flakeRate": {"isMet": True}}), Status.FLAKY)

    def test_a_met_failure_rate_wins_over_flakiness(self) -> None:
        self.assertIs(
            self.status(
                {"flakeRate": {"isMet": True}, "failureRate": {"isMet": True}}
            ),
            Status.FAILING,
        )

    def test_results_meeting_neither_are_stable(self) -> None:
        self.assertIs(
            self.status({"flakeRate": {"totalVerdicts": 10}}), Status.STABLE
        )

    def test_no_results_is_no_data(self) -> None:
        self.assertIs(self.status({}), Status.NO_DATA)

    def test_only_flaky_and_failing_are_confirmed(self) -> None:
        self.assertTrue(
            Stability("t", {}, {"flakeRate": {"isMet": True}}).confirmed
        )
        self.assertFalse(
            Stability("t", {}, {"flakeRate": {"totalVerdicts": 9}}).confirmed
        )

    def test_links_the_example_builds(self) -> None:
        stability = Stability(
            "t",
            {},
            {
                "flakeRate": {
                    "flakeExamples": [
                        {"rootInvocations": ["build-1"]},
                        {"invocations": ["build-2", "task-x"]},
                    ]
                }
            },
        )

        self.assertEqual(
            stability.example_builds,
            ["https://ci.chromium.org/b/1", "https://ci.chromium.org/b/2"],
        )


# -- Reading what the service sends --------------------------------------


class FromApiTest(unittest.TestCase):
    def test_test_variant(self) -> None:
        parsed = TestVariant.from_api(
            {
                "variantHash": "h1",
                "variant": {
                    "def": {"os": "Ubuntu-22.04", "builder": "linux-rel"}
                },
            }
        )

        self.assertEqual(parsed.variant_hash, "h1")
        self.assertEqual(parsed.os, "Ubuntu-22.04")
        self.assertEqual(parsed.builders, ("linux-rel",))
        self.assertEqual(
            parsed.definition, {"os": "Ubuntu-22.04", "builder": "linux-rel"}
        )

    def test_a_reviver_variant_keeps_both_builder_names(self) -> None:
        parsed = TestVariant.from_api(
            {
                "variantHash": "h1",
                "variant": {
                    "def": {
                        "os": "Ubuntu-22.04",
                        "builder": "runner",
                        "reviver_builder": "linux_chromium_asan_rel_ng",
                    }
                },
            }
        )

        self.assertEqual(
            parsed.builder_description, "runner linux_chromium_asan_rel_ng"
        )

    def test_a_flaky_day(self) -> None:
        parsed = FlakyDay.from_api(
            {
                "partitionTime": "2026-11-09T00:00:00Z",
                "variantHash": "h",
                "verdictCounts": {"passed": "40", "flaky": "2"},
            }
        )

        self.assertEqual(parsed, flaky_day(date(2026, 11, 9), 2))

    def test_a_variant_with_nothing_in_it(self) -> None:
        parsed = TestVariant.from_api({})

        self.assertEqual(parsed.os, "")
        self.assertEqual(parsed.builders, ())
        self.assertEqual(parsed.builder_description, "")

    def test_cluster_summary(self) -> None:
        parsed = ClusterSummary.from_api(
            {
                "clusterId": {"algorithm": "testname-v4", "id": "c1"},
                "title": "a title",
            }
        )

        self.assertEqual(parsed.algorithm, "testname-v4")
        self.assertEqual(parsed.cluster_id, "c1")
        self.assertEqual(parsed.key, ("testname-v4", "c1"))

    def test_cluster_failure_defaults_to_one(self) -> None:
        self.assertEqual(ClusterFailure.from_api({"testId": "t"}).count, 1)

    def test_cluster_failure_reads_its_count(self) -> None:
        parsed = ClusterFailure.from_api({"testId": "t", "count": "7"})

        self.assertEqual(parsed, ClusterFailure(test_id="t", count=7))


class ClusterSummaryShapeTest(unittest.TestCase):
    TARGET = "://chrome/test\\:unit_tests!gtest::"

    def summary(self, algorithm: str, title: str) -> ClusterSummary:
        return ClusterSummary(algorithm=algorithm, cluster_id="c1", title=title)

    def test_a_reason_cluster_is_recognised(self) -> None:
        cluster = self.summary("reason-v3", "Assertion failed: %s")

        self.assertTrue(cluster.groups_by_failure_reason)
        self.assertFalse(cluster.names_one_test)

    def test_a_verbatim_test_id_names_one_test(self) -> None:
        cluster = self.summary("testname-v4", self.TARGET + "Suite#Case")

        self.assertTrue(cluster.names_one_test)
        self.assertFalse(cluster.groups_by_failure_reason)

    def test_a_wildcard_title_covers_several_tests(self) -> None:
        cluster = self.summary("testname-v4", self.TARGET + "Suite#Case/%")

        self.assertFalse(cluster.names_one_test)

    def test_an_escaped_backslash_covers_several_tests(self) -> None:
        self.assertFalse(
            self.summary("testname-v4", "prefix\\\\suffix").names_one_test
        )

    def test_an_escaped_underscore_covers_several_tests(self) -> None:
        self.assertFalse(
            self.summary("testname-v4", "prefix\\_suffix").names_one_test
        )

    def test_a_rule_cluster_never_names_one_test(self) -> None:
        cluster = self.summary("rules", "crbug.com/1234567")

        self.assertFalse(cluster.names_one_test)
        self.assertFalse(cluster.groups_by_failure_reason)


# -- The client half -----------------------------------------------------


def encode_response(payload: JsonDict) -> bytes:
    """Encode a payload the way the pRPC endpoint does.

    Responses are prefixed with the XSSI guard `)]}'` and a newline.
    """
    return b")]}'\n" + json.dumps(payload).encode("utf-8")


class FakeResponse:
    """Stand-in for http.client.HTTPResponse."""

    def __init__(self, status: int, body: bytes, reason: str = "OK") -> None:
        self.status = status
        self.reason = reason
        self._body = body

    def read(self) -> bytes:
        return self._body


class FakeTransport:
    """Scripted stand-in for http.client.HTTPSConnection.

    Instances are handed out by calling the transport itself, so it
    doubles as the patched constructor and records how often the pool
    reconnects.
    """

    def __init__(self) -> None:
        self.replies: deque[FakeResponse | Exception] = deque()
        self.requests: list[dict[str, Any]] = []
        self.connects = 0
        self.closes = 0
        self.host: str | None = None
        self.timeout: float | None = None
        self.context: ssl.SSLContext | None = None

    def add_reply(
        self,
        payload: JsonDict | None = None,
        status: int = 200,
        raw: bytes | None = None,
        reason: str = "OK",
    ) -> None:
        """Queue a response. `raw` overrides the encoded `payload`."""
        body = raw if raw is not None else encode_response(payload or {})
        self.replies.append(FakeResponse(status, body, reason))

    def add_failure(self, error: Exception) -> None:
        """Queue an exception to raise instead of responding."""
        self.replies.append(error)

    def __call__(
        self,
        host: str,
        timeout: float | None = None,
        context: ssl.SSLContext | None = None,
    ) -> FakeTransport:
        self.connects += 1
        self.host = host
        self.timeout = timeout
        self.context = context
        return self

    def request(
        self,
        method: str,
        path: str,
        body: bytes | None = None,
        headers: dict[str, str] | None = None,
    ) -> None:
        self.requests.append(
            {
                "method": method,
                "path": path,
                "body": json.loads(body) if body else None,
                "headers": headers,
            }
        )

    def getresponse(self) -> FakeResponse:
        reply = self.replies.popleft()
        if isinstance(reply, Exception):
            raise reply
        return reply

    def close(self) -> None:
        self.closes += 1


class ClientTestCase(unittest.TestCase):
    """A client wired to a scripted transport, with no real waiting.

    Every piece of state belongs to the client instance, so nothing
    leaks between tests and there is nothing to reset.
    """

    def setUp(self) -> None:
        self.transport = FakeTransport()
        self.enter_patch(
            mock.patch.object(http.client, "HTTPSConnection", self.transport)
        )
        self.client = LuciAnalysis()
        # `wait` returning False means "the backoff elapsed, carry on".
        self.shutdown = mock.Mock()
        self.shutdown.wait.return_value = False
        self.enter_patch(
            mock.patch.object(self.client, "_shutdown", self.shutdown)
        )

    def enter_patch(self, patcher: Any) -> Any:
        value = patcher.start()
        self.addCleanup(patcher.stop)
        return value

    def last_body(self) -> JsonDict:
        return self.transport.requests[-1]["body"]

    def bodies(self) -> list[JsonDict]:
        return [request["body"] for request in self.transport.requests]


class ConnectionPoolTest(unittest.TestCase):
    def setUp(self) -> None:
        self.transport = FakeTransport()
        patcher = mock.patch.object(
            http.client, "HTTPSConnection", self.transport
        )
        patcher.start()
        self.addCleanup(patcher.stop)
        self.pool = ConnectionPool("example.test", 30)

    def test_opens_a_connection_on_demand(self) -> None:
        with self.pool.borrow():
            self.assertEqual(self.transport.connects, 1)

        self.assertEqual(self.transport.host, "example.test")
        self.assertEqual(self.transport.timeout, 30)

    def test_a_finished_connection_goes_back(self) -> None:
        with self.pool.borrow():
            pass

        self.assertEqual(len(self.pool), 1)

    def test_the_next_borrower_reuses_it(self) -> None:
        with self.pool.borrow():
            pass
        with self.pool.borrow():
            pass

        self.assertEqual(self.transport.connects, 1)

    def test_a_broken_connection_is_retired(self) -> None:
        with self.assertRaises(ValueError):
            with self.pool.borrow():
                raise ValueError("the request went wrong")

        self.assertEqual(len(self.pool), 0)
        self.assertEqual(self.transport.closes, 1)

    def test_concurrent_borrowers_get_their_own(self) -> None:
        with self.pool.borrow():
            with self.pool.borrow():
                self.assertEqual(self.transport.connects, 2)

        self.assertEqual(len(self.pool), 2)

    def test_closing_empties_the_pool(self) -> None:
        with self.pool.borrow():
            pass

        self.pool.close_all()

        self.assertEqual(len(self.pool), 0)
        self.assertEqual(self.transport.closes, 1)


class TlsContextTest(unittest.TestCase):
    def test_certificates_are_verified(self) -> None:
        self.assertEqual(TLS_CONTEXT.verify_mode, ssl.CERT_REQUIRED)

    def test_the_hostname_is_checked(self) -> None:
        self.assertTrue(TLS_CONTEXT.check_hostname)

    def test_the_pool_hands_that_context_to_every_connection(self) -> None:
        transport = FakeTransport()
        with mock.patch.object(http.client, "HTTPSConnection", transport):
            with ConnectionPool("example.test", 30).borrow():
                pass

        self.assertIs(transport.context, TLS_CONTEXT)


class RequestTest(ClientTestCase):
    def request(self) -> Request:
        return self.client.request("svc", "M", field="value")

    def test_posts_json_to_the_service_method_path(self) -> None:
        self.transport.add_reply({"ok": True})

        result = self.request().send()

        self.assertEqual(result, {"ok": True})
        sent = self.transport.requests[0]
        self.assertEqual(sent["method"], "POST")
        self.assertEqual(sent["path"], "/prpc/svc/M")
        self.assertEqual(sent["headers"]["Content-Type"], "application/json")
        self.assertEqual(sent["headers"]["Accept"], "application/json")

    def test_fills_in_the_project(self) -> None:
        self.transport.add_reply()

        self.request().send()

        self.assertEqual(
            self.last_body(), {"project": "chromium", "field": "value"}
        )

    def test_strips_the_xssi_prefix(self) -> None:
        self.transport.add_reply(raw=b")]}'\n{\"testIds\": [\"a\"]}")

        self.assertEqual(self.request().send(), {"testIds": ["a"]})

    def test_keeps_a_single_line_body_without_a_prefix(self) -> None:
        # Nothing is stripped when the body holds no newline at all.
        self.transport.add_reply(raw=b'{"testIds": []}')

        self.assertEqual(self.request().send(), {"testIds": []})

    def test_retries_after_a_network_error_on_a_fresh_connection(self) -> None:
        self.transport.add_failure(ConnectionResetError("reset"))
        self.transport.add_reply({"n": 1})

        self.assertEqual(self.request().send(), {"n": 1})
        self.assertEqual(self.transport.closes, 1)
        self.assertEqual(self.transport.connects, 2)

    def test_retries_after_a_keep_alive_disconnect(self) -> None:
        self.transport.add_failure(http.client.RemoteDisconnected("bye"))
        self.transport.add_reply({"n": 1})

        self.assertEqual(self.request().send(), {"n": 1})

    def test_gives_up_after_three_network_errors(self) -> None:
        for _ in range(3):
            self.transport.add_failure(TimeoutError("timeout"))

        with self.assertRaises(LuciAnalysisError) as caught:
            self.request().send()

        self.assertIn(
            "Could not reach LUCI Analysis API", str(caught.exception)
        )
        self.assertEqual(len(self.transport.requests), 3)
        self.assertEqual(
            self.shutdown.wait.call_args_list, [mock.call(2), mock.call(4)]
        )

    def test_retries_server_errors(self) -> None:
        self.transport.add_reply(status=503, reason="Service Unavailable")
        self.transport.add_reply({"n": 1})

        self.assertEqual(self.request().send(), {"n": 1})

    def test_retries_rate_limiting(self) -> None:
        self.transport.add_reply(status=429, reason="Too Many Requests")
        self.transport.add_reply({"n": 1})

        self.assertEqual(self.request().send(), {"n": 1})

    def test_raises_after_persistent_server_errors(self) -> None:
        for _ in range(3):
            self.transport.add_reply(status=500, reason="Internal Error")

        with self.assertRaises(LuciAnalysisError) as caught:
            self.request().send()

        self.assertIn("HTTP 500", str(caught.exception))

    def test_raises_on_forbidden_without_retrying(self) -> None:
        self.transport.add_reply(status=403, reason="Forbidden")

        with self.assertRaises(LuciAnalysisError) as caught:
            self.request().send()

        self.assertIn("403 Forbidden", str(caught.exception))
        self.assertEqual(len(self.transport.requests), 1)

    def test_raises_on_not_found_without_retrying(self) -> None:
        self.transport.add_reply(status=404, reason="Not Found")

        with self.assertRaises(LuciAnalysisError) as caught:
            self.client.request("svc", "QueryTests").send()

        self.assertIn(
            "404 Not Found for method QueryTests", str(caught.exception)
        )
        self.assertEqual(len(self.transport.requests), 1)

    def test_raises_on_client_error_without_retrying(self) -> None:
        self.transport.add_reply(status=400, reason="Bad Request")

        with self.assertRaises(LuciAnalysisError) as caught:
            self.request().send()

        self.assertIn("HTTP 400", str(caught.exception))
        self.assertEqual(len(self.transport.requests), 1)

    def test_raises_on_unparseable_body(self) -> None:
        self.transport.add_reply(raw=b")]}'\nnot json at all")

        with self.assertRaises(LuciAnalysisError) as caught:
            self.request().send()

        self.assertIn(
            "Could not parse API response as JSON", str(caught.exception)
        )

    def test_a_shutdown_during_the_backoff_stops_the_retries(self) -> None:
        # Ctrl+C must not be held up by a worker waiting out its backoff.
        self.shutdown.wait.return_value = True
        self.transport.add_failure(ConnectionResetError("reset"))

        with self.assertRaises(LuciAnalysisError):
            self.request().send()

        self.assertEqual(len(self.transport.requests), 1)

    def test_a_shutdown_stops_server_error_retries_too(self) -> None:
        self.shutdown.wait.return_value = True
        self.transport.add_reply(status=503, reason="Service Unavailable")

        with self.assertRaises(LuciAnalysisError):
            self.request().send()

        self.assertEqual(len(self.transport.requests), 1)


class PagedRequestTest(ClientTestCase):
    def test_asks_for_the_largest_page(self) -> None:
        self.transport.add_reply({})

        self.client.paged_request("svc", "M").gather("items")

        self.assertEqual(self.last_body()["pageSize"], MAX_PAGE_SIZE)

    def test_follows_the_page_tokens(self) -> None:
        self.transport.add_reply({"items": ["a"], "nextPageToken": "t1"})
        self.transport.add_reply({"items": ["b"], "nextPageToken": "t2"})
        self.transport.add_reply({"items": ["c"]})

        gathered = self.client.paged_request("svc", "M").gather("items")

        self.assertEqual(gathered, ["a", "b", "c"])
        self.assertEqual(
            [body.get("pageToken") for body in self.bodies()],
            [None, "t1", "t2"],
        )

    def test_the_rest_of_the_body_is_carried_across_pages(self) -> None:
        self.transport.add_reply({"items": [], "nextPageToken": "t1"})
        self.transport.add_reply({"items": []})

        self.client.paged_request("svc", "M", testId="t").gather("items")

        for body in self.bodies():
            self.assertEqual(body["testId"], "t")

    def test_a_single_empty_page(self) -> None:
        self.transport.add_reply({})

        self.assertEqual(self.client.paged_request("svc", "M").gather("x"), [])

    def test_pages_can_be_walked_one_at_a_time(self) -> None:
        self.transport.add_reply({"items": ["a"], "nextPageToken": "t"})
        self.transport.add_reply({"items": ["b"]})

        pages = list(self.client.paged_request("svc", "M").pages())

        self.assertEqual([page["items"] for page in pages], [["a"], ["b"]])


class RequestStatsTest(ClientTestCase):
    def test_counts_a_completed_request(self) -> None:
        self.transport.add_reply({"n": 1})

        self.client.request("svc", "M").send()

        self.assertEqual(self.client.stats.started, 1)
        self.assertEqual(self.client.stats.completed, 1)
        self.assertEqual(self.client.stats.in_flight, 0)
        self.assertEqual(self.client.stats.failures, 0)

    def test_a_retry_is_not_a_second_request(self) -> None:
        self.transport.add_reply(status=503, reason="Service Unavailable")
        self.transport.add_reply({"n": 1})

        self.client.request("svc", "M").send()

        self.assertEqual(self.client.stats.retries, 1)
        self.assertEqual(self.client.stats.started, 1)
        self.assertEqual(self.client.stats.failures, 0)

    def test_counts_a_failure(self) -> None:
        self.transport.add_reply(status=403, reason="Forbidden")

        with self.assertRaises(LuciAnalysisError):
            self.client.request("svc", "M").send()

        self.assertEqual(self.client.stats.failures, 1)
        self.assertEqual(self.client.stats.in_flight, 0)

    def test_each_page_is_counted(self) -> None:
        self.transport.add_reply({"items": [], "nextPageToken": "t"})
        self.transport.add_reply({"items": []})

        self.client.paged_request("svc", "M").gather("items")

        self.assertEqual(self.client.stats.completed, 2)

    def test_in_flight_counts_the_unfinished(self) -> None:
        stats = RequestStats(started=7, completed=4)

        self.assertEqual(stats.in_flight, 3)

    def test_reset_zeroes_every_counter(self) -> None:
        stats = RequestStats(started=1, completed=2, retries=3, failures=4)

        stats.reset()

        self.assertEqual(stats, RequestStats())

    def test_two_clients_count_separately(self) -> None:
        self.transport.add_reply({})
        other = LuciAnalysis()

        self.client.request("svc", "M").send()

        self.assertEqual(self.client.stats.started, 1)
        self.assertEqual(other.stats.started, 0)


class QueriesTest(ClientTestCase):
    WINDOW = Window(
        datetime(2026, 1, 1, tzinfo=timezone.utc),
        datetime(2026, 1, 8, tzinfo=timezone.utc),
    )

    def test_tests_matching(self) -> None:
        self.transport.add_reply({"testIds": ["a"], "nextPageToken": "t"})
        self.transport.add_reply({"testIds": ["b"]})

        self.assertEqual(self.client.tests_matching("Frob"), ["a", "b"])
        self.assertEqual(self.bodies()[0]["testIdSubstring"], "Frob")
        self.assertEqual(
            self.transport.requests[0]["path"],
            "/prpc/luci.analysis.v1.TestHistory/QueryTests",
        )

    def test_variants_returns_test_variants(self) -> None:
        self.transport.add_reply(
            {
                "variants": [
                    {
                        "variantHash": "h1",
                        "variant": {
                            "def": {"os": "Mac-15", "builder": "mac-rel"}
                        },
                    }
                ]
            }
        )

        variants = self.client.variants("test-id")

        self.assertEqual(variants, [TestVariant("h1", "Mac-15", ("mac-rel",))])

    def test_ci_flaky_days_only_asks_about_ci_bots(self) -> None:
        self.transport.add_reply(
            {
                "groups": [
                    {
                        "partitionTime": "2026-01-02T00:00:00Z",
                        "variantHash": "h",
                        "verdictCounts": {"flaky": "2"},
                    },
                    {
                        "partitionTime": "2026-01-03T00:00:00Z",
                        "variantHash": "h",
                        "verdictCounts": {"passed": "9"},
                    },
                ]
            }
        )

        days = self.client.ci_flaky_days("test-id", self.WINDOW)

        self.assertEqual(days, [flaky_day(date(2026, 1, 2), 2)])
        body = self.last_body()
        self.assertEqual(body["testId"], "test-id")
        self.assertEqual(
            body["predicate"],
            {
                "partitionTimeRange": self.WINDOW.as_time_range(),
                "variantPredicate": {"contains": {"def": {"bucket": "ci"}}},
            },
        )
        self.assertEqual(
            self.transport.requests[-1]["path"],
            "/prpc/luci.analysis.v1.TestHistory/QueryStats",
        )

    def test_stability_asks_about_the_source_position(self) -> None:
        self.transport.add_reply(
            {
                "testVariants": [{"flakeRate": {"isMet": True}}],
                "criteria": {"flakeRate": {"flakeThreshold": 3}},
            }
        )
        variant = TestVariant(
            "h1", "Mac-15", ("mac-rel",), {"builder": "mac-rel"}
        )

        results, criteria = self.client.stability([("t1", variant)], BRANCH)

        self.assertEqual(
            results,
            [
                Stability(
                    "t1", {"builder": "mac-rel"}, {"flakeRate": {"isMet": True}}
                )
            ],
        )
        self.assertEqual(criteria, {"flakeRate": {"flakeThreshold": 3}})
        (sent,) = self.last_body()["testVariants"]
        self.assertEqual(sent["testId"], "t1")
        self.assertEqual(sent["variantHash"], "h1")
        self.assertEqual(sent["sources"], BRANCH.as_sources())
        self.assertEqual(
            self.transport.requests[-1]["path"],
            "/prpc/luci.analysis.v1.TestVariants/QueryStability",
        )

    def test_stability_splits_large_requests(self) -> None:
        variants = [
            ("t", TestVariant(str(i), "", ()))
            for i in range(MAX_STABILITY_BATCH + 1)
        ]
        self.transport.add_reply({"testVariants": [{}] * MAX_STABILITY_BATCH})
        self.transport.add_reply({"testVariants": [{}]})

        results, _ = self.client.stability(variants, BRANCH)

        self.assertEqual(len(results), MAX_STABILITY_BATCH + 1)
        self.assertEqual(
            [len(body["testVariants"]) for body in self.bodies()],
            [MAX_STABILITY_BATCH, 1],
        )

    def test_no_variants_means_no_request(self) -> None:
        self.assertEqual(self.client.stability([], BRANCH), ([], {}))
        self.assertEqual(self.transport.requests, [])

    def test_cluster_summaries(self) -> None:
        self.transport.add_reply(
            {
                "clusterSummaries": [
                    {
                        "clusterId": {"algorithm": "testname-v4", "id": "c1"},
                        "title": "t",
                    }
                ]
            }
        )

        summaries = self.client.cluster_summaries(
            'test_id:":unit_tests"', self.WINDOW
        )

        self.assertEqual(summaries, [ClusterSummary("testname-v4", "c1", "t")])
        body = self.last_body()
        self.assertEqual(body["failureFilter"], 'test_id:":unit_tests"')
        self.assertEqual(body["orderBy"], "metrics.`failures`.value desc")
        self.assertEqual(
            body["metrics"], ["projects/chromium/metrics/failures"]
        )
        self.assertEqual(body["timeRange"], self.WINDOW.as_time_range())
        self.assertEqual(
            self.transport.requests[-1]["path"],
            "/prpc/luci.analysis.v1.Clusters/QueryClusterSummaries",
        )

    def test_cluster_summaries_is_not_paged(self) -> None:
        # Upstream caps the answer at 200 and offers no page token.
        self.transport.add_reply({"clusterSummaries": []})

        self.client.cluster_summaries("f", self.WINDOW)

        self.assertNotIn("pageSize", self.last_body())
        self.assertEqual(len(self.transport.requests), 1)

    def test_cluster_failures(self) -> None:
        self.transport.add_reply({"failures": [{"testId": "a", "count": "3"}]})

        failures = self.client.cluster_failures(
            ClusterSummary("testname-v4", "abc12", "t")
        )

        self.assertEqual(failures, [ClusterFailure("a", 3)])
        self.assertEqual(
            self.last_body()["parent"],
            "projects/chromium/clusters/testname-v4/abc12/failures",
        )

    def test_an_empty_answer_is_an_empty_list(self) -> None:
        for _ in range(3):
            self.transport.add_reply({})

        self.assertEqual(self.client.tests_matching("x"), [])
        self.assertEqual(self.client.variants("t"), [])
        self.assertEqual(self.client.cluster_summaries("f", self.WINDOW), [])


class ClientLifecycleTest(ClientTestCase):
    def test_close_releases_the_pool(self) -> None:
        self.transport.add_reply({})
        self.client.request("svc", "M").send()

        self.client.close()

        self.assertEqual(len(self.client.pool), 0)
        self.assertEqual(self.transport.closes, 1)

    def test_a_requested_shutdown_reaches_the_backoff(self) -> None:
        client = LuciAnalysis()

        client.request_shutdown()

        self.assertTrue(client.backoff(0))


if __name__ == "__main__":
    unittest.main()
