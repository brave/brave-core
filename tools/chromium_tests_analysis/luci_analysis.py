# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Client and domain model for the Chromium LUCI Analysis API.
"""

from __future__ import annotations

import http.client
import json
import ssl
import threading
from collections import deque
from collections.abc import Iterable, Iterator
from contextlib import contextmanager
from dataclasses import dataclass, replace
from datetime import datetime, timedelta, timezone
from enum import Enum
from typing import Any

# A JSON object decoded from an API response, or built to send as one.
# Confined to this module: nothing above the client should see one.
JsonDict = dict[str, Any]

LUCI_ANALYSIS_HOST = "analysis.api.luci.app"
TEST_HISTORY_SERVICE = "luci.analysis.v1.TestHistory"
CLUSTERS_SERVICE = "luci.analysis.v1.Clusters"
CHROMIUM_PROJECT = "chromium"

REQUEST_TIMEOUT_SECS = 30
MAX_ATTEMPTS = 3

# The TLS settings every connection is made with: certificate chain and
# hostname both verified. One shared context loads the CA bundle once instead of
# once per connection in the pool.
TLS_CONTEXT = ssl.create_default_context()

# The largest page the API serves. Every paged request in
# analysis/proto/v1 documents "values above 1000 will be coerced to 1000".
MAX_PAGE_SIZE = 1000

# The most failure groups `cluster_failures` will return, according to
# analysis/internal/analysis/cluster_failures.go. A cluster that comes
# back with exactly this many was very likely cut short upstream.
MAX_CLUSTER_FAILURES = 2000

# Flake rate thresholds used to classify upstream flakiness. See
# docs/best-practices/testing-upstream-failures.md.
KNOWN_FLAKE_RATE = 0.05
OCCASIONAL_FLAKE_RATE = 0.01
MIN_MEANINGFUL_VERDICTS = 10


class LuciAnalysisError(Exception):
    """Raised when a LUCI Analysis API request fails."""


# -- Time ----------------------------------------------------------------


@dataclass(frozen=True)
class Window:
    """A span of time, in the two shapes the API asks for it."""

    # Start of the span, inclusive.
    earliest: datetime
    # End of the span, exclusive.
    latest: datetime

    @classmethod
    def last_days(cls, days: int) -> Window:
        """The window ending now and reaching `days` back."""
        now = datetime.now(timezone.utc)
        return cls(now - timedelta(days=days), now)

    @property
    def days(self) -> float:
        """How long the window is."""
        return (self.latest - self.earliest).total_seconds() / 86400

    def split_weekly(self) -> list[Window]:
        """Cut the window into consecutive slices of a week or less."""
        slices = []
        start = self.earliest
        while start < self.latest:
            end = min(start + timedelta(days=7), self.latest)
            slices.append(Window(start, end))
            start = end
        return slices

    def _wire(self) -> dict[str, str]:
        return {
            "earliest": self.earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "latest": self.latest.strftime("%Y-%m-%dT%H:%M:%SZ"),
        }

    def as_time_range(self) -> dict[str, str]:
        """The `timeRange` field the Clusters service takes."""
        return self._wire()

    def as_predicate(self) -> JsonDict:
        """The `predicate` field the TestHistory service takes."""
        return {"partitionTimeRange": self._wire()}


# -- What the numbers mean -----------------------------------------------


class Verdict(Enum):
    """How a test's upstream history reads, and what to do about it."""

    KNOWN_FLAKE = "known_upstream_flake"
    OCCASIONAL = "occasional_upstream_failures"
    INSUFFICIENT_DATA = "insufficient_data"
    STABLE = "stable_upstream"

    @property
    def severity(self) -> int:
        """Ordering for "the worst verdict across several tests"."""
        return _VERDICT_SEVERITY[self]

    @property
    def headline(self) -> str:
        """The verdict as a report heading."""
        return _VERDICT_HEADLINE[self]

    @property
    def recommendation(self) -> str:
        """What a reader should do about this verdict."""
        return _VERDICT_RECOMMENDATION[self]

    @property
    def style(self) -> str:
        """How to colour the verdict in a terminal."""
        return _VERDICT_STYLE[self]


# Declared outside the enum body, where a plain assignment would become
# another member rather than a class attribute.
_VERDICT_SEVERITY = {
    Verdict.KNOWN_FLAKE: 0,
    Verdict.OCCASIONAL: 1,
    Verdict.INSUFFICIENT_DATA: 2,
    Verdict.STABLE: 3,
}

_VERDICT_HEADLINE = {
    Verdict.KNOWN_FLAKE: "KNOWN UPSTREAM FLAKE",
    Verdict.OCCASIONAL: "OCCASIONAL UPSTREAM FAILURES",
    Verdict.INSUFFICIENT_DATA: "INSUFFICIENT DATA",
    Verdict.STABLE: "STABLE UPSTREAM",
}

_VERDICT_STYLE = {
    Verdict.KNOWN_FLAKE: "bold red",
    Verdict.OCCASIONAL: "yellow",
    Verdict.INSUFFICIENT_DATA: "dim",
    Verdict.STABLE: "green",
}

_VERDICT_RECOMMENDATION = {
    Verdict.KNOWN_FLAKE: ("Safe to filter -- this test has a confirmed"
                          " flakiness pattern in Chromium upstream."),
    Verdict.OCCASIONAL: ("Consider filtering -- test shows some upstream"
                         " instability. Document findings in filter"
                         " comment."),
    Verdict.INSUFFICIENT_DATA: ("Cannot determine -- insufficient upstream"
                                " data for this test in the lookback"
                                " period."),
    Verdict.STABLE: ("Investigate Brave-specific causes -- test appears"
                     " stable in Chromium upstream."),
}


@dataclass(frozen=True)
class VerdictCounts:
    """How a test's runs turned out, tallied.

    Instances add, so aggregating across days, variants or configs is
    `sum(counts, VerdictCounts())`.
    """

    # Verdicts with passing results and no failing ones.
    passed: int = 0

    # Verdicts with failing results and no passing ones.
    failed: int = 0

    # Verdicts with both passing and failing results: the test failed
    # and then passed on retry within the same invocation.
    flaky: int = 0

    # Verdicts with only skipped results, so the test never really ran.
    skipped: int = 0

    # Verdicts where the harness itself failed rather than the test:
    # execution errored, with nothing passing, failing or skipped.
    execution_errored: int = 0

    # Verdicts LUCI never attempted, e.g. because an earlier step of the
    # build failed.
    precluded: int = 0

    # Statuses none of the above covers. Counted in the total, never in
    # the flake rate.
    other: int = 0

    @classmethod
    def from_api(cls, counts: JsonDict) -> VerdictCounts:
        """Read a `verdictCounts` object. Values arrive as strings."""
        return cls(
            passed=int(counts.get("passed", 0)),
            failed=int(counts.get("failed", 0)),
            flaky=int(counts.get("flaky", 0)),
            skipped=int(counts.get("skipped", 0)),
            execution_errored=int(counts.get("executionErrored", 0)),
            precluded=int(counts.get("precluded", 0)),
        )

    def __add__(self, other: VerdictCounts) -> VerdictCounts:
        return VerdictCounts(
            passed=self.passed + other.passed,
            failed=self.failed + other.failed,
            flaky=self.flaky + other.flaky,
            skipped=self.skipped + other.skipped,
            execution_errored=(self.execution_errored +
                               other.execution_errored),
            precluded=self.precluded + other.precluded,
            other=self.other + other.other,
        )

    @property
    def total(self) -> int:
        """Every verdict, whatever it was."""
        return (self.passed + self.failed + self.flaky + self.skipped +
                self.execution_errored + self.precluded + self.other)

    @property
    def meaningful(self) -> int:
        """Verdicts that say something about flakiness.

        Skipped and precluded runs tell us nothing either way, so they
        stay out of the denominator.
        """
        return self.passed + self.failed + self.flaky

    @property
    def flake_rate(self) -> float:
        """Share of meaningful verdicts that did not simply pass."""
        if not self.meaningful:
            return 0.0
        return (self.failed + self.flaky) / self.meaningful


@dataclass(frozen=True)
class DailyCounts:
    """One day of a test's history, across every variant."""

    # The day, as "YYYY-MM-DD".
    date: str
    # That day's verdicts, summed over every variant.
    counts: VerdictCounts


@dataclass(frozen=True)
class Flakiness:
    """What a test's history adds up to, and how it reads.

    Build one with `of_groups` (the usual path, from QueryStats) or
    `of_verdicts` (the fallback, from Query).
    """

    # Every verdict in the window, summed.
    counts: VerdictCounts = VerdictCounts()

    # The same verdicts split by day, oldest first, with empty days
    # left out. Only the single-test report renders this.
    daily: tuple[DailyCounts, ...] = ()

    @classmethod
    def of_groups(cls, groups: Iterable[StatsGroup]) -> Flakiness:
        """Aggregate per-day, per-variant stats into one reading."""
        by_date: dict[str, VerdictCounts] = {}
        for group in groups:
            by_date[group.date] = (by_date.get(group.date, VerdictCounts()) +
                                   group.counts)
        daily = tuple(
            DailyCounts(date, counts)
            for date, counts in sorted(by_date.items()) if counts.total)
        total = VerdictCounts()
        for day in daily:
            total += day.counts
        return cls(counts=total, daily=daily)

    @classmethod
    def of_verdicts(cls, verdicts: Iterable[TestVerdict]) -> Flakiness:
        """Tally individual verdicts, for when QueryStats has nothing."""
        total = VerdictCounts()
        for verdict in verdicts:
            total += verdict.as_counts()
        return cls(counts=total)

    @property
    def flake_rate(self) -> float:
        """Share of meaningful verdicts that did not simply pass."""
        return self.counts.flake_rate

    @property
    def verdict(self) -> Verdict:
        """How this history reads."""
        if self.counts.meaningful < MIN_MEANINGFUL_VERDICTS:
            return Verdict.INSUFFICIENT_DATA
        if self.flake_rate >= KNOWN_FLAKE_RATE:
            return Verdict.KNOWN_FLAKE
        if self.flake_rate >= OCCASIONAL_FLAKE_RATE:
            return Verdict.OCCASIONAL
        return Verdict.STABLE

    def as_json(self) -> JsonDict:
        """The shape `check-upstream-flake.py --json` publishes.

        Other tooling reads these keys, so they are a contract; see
        agents/skills/make-ci-green/retrigger-ci.py.
        """
        return {
            "total_verdicts": self.counts.total,
            "meaningful_verdicts": self.counts.meaningful,
            "passed": self.counts.passed,
            "failed": self.counts.failed,
            "flaky": self.counts.flaky,
            "skipped": self.counts.skipped,
            "execution_errored": self.counts.execution_errored,
            "precluded": self.counts.precluded,
            "flake_rate": self.flake_rate,
            "verdict": self.verdict.value,
            "recommendation": self.verdict.recommendation,
            "daily_breakdown": [{
                "date": day.date,
                "passed": day.counts.passed,
                "failed": day.counts.failed,
                "flaky": day.counts.flaky,
                "skipped": day.counts.skipped,
                "execution_errored": day.counts.execution_errored,
                "precluded": day.counts.precluded,
                "total": day.counts.total,
            } for day in self.daily],
        }


# -- What the service returns --------------------------------------------


@dataclass(frozen=True)
class StatsGroup:
    """One day of one variant's verdicts for a single test."""

    # The day, as "YYYY-MM-DD", or "unknown" if the API sent none.
    date: str

    # Identifies the bot configuration. Resolve it with `variants()`.
    variant_hash: str

    # How that day's runs on that one variant turned out.
    counts: VerdictCounts

    @classmethod
    def from_api(cls, group: JsonDict) -> StatsGroup:
        """Read a QueryStats group."""
        partition_time = group.get("partitionTime", "")
        return cls(
            date=partition_time[:10] if partition_time else "unknown",
            variant_hash=group.get("variantHash", ""),
            counts=VerdictCounts.from_api(group.get("verdictCounts", {})),
        )


@dataclass(frozen=True)
class TestVerdict:
    """How one invocation of a test turned out."""

    # Upper-cased status name, e.g. "PASSED" or "EXECUTION_ERRORED".
    status: str

    @classmethod
    def from_api(cls, verdict: JsonDict) -> TestVerdict:
        """Read a Query verdict."""
        return cls(status=verdict.get("status", "").upper())

    def as_counts(self) -> VerdictCounts:
        """This verdict as a tally of one."""
        field = {
            "PASSED": "passed",
            "FAILED": "failed",
            "FLAKY": "flaky",
            "SKIPPED": "skipped",
        }.get(self.status, "other")
        return replace(VerdictCounts(), **{field: 1})


@dataclass(frozen=True)
class TestVariant:
    """A bot configuration a test has run on."""

    # What `StatsGroup.variant_hash` refers to.
    variant_hash: str

    # OS of the machine that ran the test, not the platform under test:
    # iOS simulator bots report a Mac os, ChromeOS builds report Ubuntu.
    os: str

    # The builder, plus the builder whose failures it is retrying:
    # Chromium's flake-retry bots run as "runner" in bucket "reviver" and
    # name the original in "reviver_builder".
    builders: tuple[str, ...]

    @classmethod
    def from_api(cls, entry: JsonDict) -> TestVariant:
        """Read a QueryVariants entry."""
        definition = entry.get("variant", {}).get("def", {})
        names = (definition.get("builder",
                                ""), definition.get("reviver_builder", ""))
        return cls(
            variant_hash=entry.get("variantHash", ""),
            os=definition.get("os", ""),
            builders=tuple(name for name in names if name),
        )

    @property
    def builder_description(self) -> str:
        """Every builder name involved, lowercased, for substring tests."""
        return " ".join(self.builders).lower()


@dataclass(frozen=True)
class ClusterSummary:
    """A group of failures LUCI Analysis considers related."""

    # How the cluster was formed: "testname-...", "reason-..." or
    # "rules" for one tied to a filed bug.
    algorithm: str

    # Identifies the cluster within its algorithm.
    cluster_id: str

    # For a single-test cluster the verbatim test ID; otherwise a SQL
    # LIKE pattern, or a bug reference for a rule.
    title: str

    @classmethod
    def from_api(cls, summary: JsonDict) -> ClusterSummary:
        """Read a QueryClusterSummaries entry."""
        cluster = summary.get("clusterId", {})
        return cls(
            algorithm=cluster.get("algorithm", ""),
            cluster_id=cluster.get("id", ""),
            title=summary.get("title", ""),
        )

    @property
    def key(self) -> tuple[str, str]:
        """What identifies this cluster, for de-duplication."""
        return (self.algorithm, self.cluster_id)

    @property
    def groups_by_failure_reason(self) -> bool:
        """True for clusters keyed on the failure message.

        Their failures also count towards a test name cluster, so
        following them up separately would only repeat work.
        """
        return self.algorithm.startswith("reason")

    @property
    def names_one_test(self) -> bool:
        """True when the title is a single test's verbatim ID.

        Titles of clusters covering several tests (parameterised
        variants, bug rules) are SQL LIKE patterns, with literals escaped
        by backslash. A verbatim test ID never contains "\\\\", "\\_" or
        "%" -- only the "\\:" that flat test ID encoding produces.
        """
        if not self.algorithm.startswith("testname"):
            return False
        return not ("%" in self.title or "\\\\" in self.title
                    or "\\_" in self.title)


@dataclass(frozen=True)
class ClusterFailure:
    """A group of identical failures inside a cluster."""

    # The test that failed, as a structured LUCI test ID.
    test_id: str
    # How many failures this group stands for; the API returns one entry
    # per group of identical failures rather than one per failure.
    count: int

    @classmethod
    def from_api(cls, failure: JsonDict) -> ClusterFailure:
        """Read a QueryClusterFailures entry."""
        return cls(test_id=failure.get("testId", ""),
                   count=int(failure.get("count", 1)))


# -- Talking to the service ----------------------------------------------


@dataclass
class RequestStats:
    """Live counters for the requests a client is making.

    A progress display reads these to report throughput. Every update
    happens on one thread between greenlet switches, so plain increments
    are enough.
    """

    # Logical requests begun, counted once however often they retry.
    started: int = 0

    # Logical requests finished, whether they succeeded or raised.
    completed: int = 0

    # Attempts made beyond the first, from a network error, a 5xx or a
    # 429. The cause is not distinguished.
    retries: int = 0

    # Requests that gave up and raised LuciAnalysisError.
    failures: int = 0

    @property
    def in_flight(self) -> int:
        """Requests started but not yet finished."""
        return self.started - self.completed

    def reset(self) -> None:
        """Zero every counter."""
        self.started = self.completed = self.retries = self.failures = 0


class ConnectionPool:
    """Warm HTTPS connections, lent out one per request in flight.

    The pool settles at the peak concurrency and every request after the
    first reuses a warm TLS session. Caching one connection per worker
    instead would be no help under gevent, where each request runs on its
    own short-lived greenlet.
    """

    def __init__(self, host: str, timeout: float) -> None:
        # Where new connections are opened to.
        self._host = host
        # Seconds a socket operation may block before it gives up.
        self._timeout = timeout
        # Connections not currently serving a request. Borrowing takes
        # from one end and returning puts back at the same end, so the
        # warmest connection is always the next one out.
        self._idle: deque[http.client.HTTPSConnection] = deque()

    def __len__(self) -> int:
        return len(self._idle)

    @contextmanager
    def borrow(self) -> Iterator[http.client.HTTPSConnection]:
        """Lend out a connection, retiring it if the request fails.

        A connection only goes back into the pool once its response has
        been read in full, which is the point at which reuse is safe.
        """
        try:
            connection = self._idle.popleft()
        except IndexError:
            connection = http.client.HTTPSConnection(self._host,
                                                     timeout=self._timeout,
                                                     context=TLS_CONTEXT)
        reusable = False
        try:
            yield connection
            reusable = True
        finally:
            if reusable:
                self._idle.append(connection)
            else:
                connection.close()

    def close_all(self) -> None:
        """Close every pooled connection."""
        while True:
            try:
                self._idle.popleft().close()
            except IndexError:
                return


class Request:
    """A single pRPC call.

    Spawned by `LuciAnalysis.request`, which supplies the client whose
    pool, counters and shutdown flag it works against. It owns the retry
    policy and the decoding, so callers never see an HTTP status.
    """

    def __init__(self, client: LuciAnalysis, service: str, method: str,
                 body: JsonDict) -> None:

        # Whose pool, counters and shutdown flag this call works
        # against.
        self._client = client

        # Full pRPC service name, e.g. "luci.analysis.v1.TestHistory".
        self._service = service

        # RPC method on that service, e.g. "QueryStats".
        self._method = method

        # The request body, already including `project`. `PagedRequest`
        # copies it per page rather than mutating it, so it stays the
        # first page's body.
        self._body = body

    @property
    def path(self) -> str:
        """Where this call is POSTed."""
        return f"/prpc/{self._service}/{self._method}"

    def send(self) -> JsonDict:
        """Make the call and return the decoded reply.

        Raises:
            LuciAnalysisError on auth, network, HTTP or parse errors.
        """
        return self._send_body(self._body)

    def _send_body(self, body: JsonDict) -> JsonDict:
        """One logical request, counted once however often it retries."""
        stats = self._client.stats
        stats.started += 1
        try:
            raw = self._attempt_until_answered(body)
        except LuciAnalysisError:
            stats.failures += 1
            raise
        finally:
            stats.completed += 1
        return self._decode(raw)

    def _attempt_until_answered(self, body: JsonDict) -> bytes:
        """POST the body, retrying transient failures."""
        data = json.dumps(body).encode("utf-8")
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
        }
        client = self._client
        for attempt in range(1, MAX_ATTEMPTS + 1):
            try:
                with client.pool.borrow() as connection:
                    connection.request("POST",
                                       self.path,
                                       body=data,
                                       headers=headers)
                    response = connection.getresponse()
                    status = response.status
                    reason = response.reason
                    raw = response.read()
            # OSError covers timeouts and connection resets;
            # HTTPException covers e.g. a keep-alive connection the
            # server has meanwhile closed (RemoteDisconnected) or
            # truncated reads.
            except (OSError, http.client.HTTPException) as e:
                if attempt < MAX_ATTEMPTS and not client.backoff(attempt):
                    client.stats.retries += 1
                    continue
                raise LuciAnalysisError(
                    f"Could not reach LUCI Analysis API: {e}") from e

            if status == 200:
                return raw
            if status == 403:
                raise LuciAnalysisError(
                    "403 Forbidden from LUCI Analysis API. The API may"
                    " require authentication for this query.")
            if status == 404:
                raise LuciAnalysisError(
                    f"404 Not Found for method {self._method}.")
            # Retry transient server errors and rate limiting.
            if ((status >= 500 or status == 429) and attempt < MAX_ATTEMPTS
                    and not client.backoff(attempt)):
                client.stats.retries += 1
                continue
            raise LuciAnalysisError(
                f"HTTP {status} from LUCI Analysis API: {reason}")
        raise AssertionError("the retry loop always returns or raises")

    @staticmethod
    def _decode(raw: bytes) -> JsonDict:
        """Strip the pRPC XSSI prefix and parse what is left.

        The prefix is )]}' followed by a newline, so everything up to and
        including the first newline goes.
        """
        newline = raw.find(b"\n")
        if newline >= 0:
            raw = raw[newline + 1:]
        try:
            return json.loads(raw)
        except json.JSONDecodeError as e:
            raise LuciAnalysisError(
                "Could not parse API response as JSON. Raw response (first"
                f" 500 bytes): {raw[:500]}") from e


class PagedRequest(Request):
    """A pRPC call whose results arrive over several pages."""

    def pages(self) -> Iterator[JsonDict]:
        """Yield each page, following the API's page tokens."""
        body = dict(self._body)
        while True:
            page = self._send_body(body)
            yield page
            token = page.get("nextPageToken")
            if not token:
                return
            body = {**body, "pageToken": token}

    def gather(self, field: str) -> list[JsonDict]:
        """Concatenate `field` across every page."""
        return [item for page in self.pages() for item in page.get(field, [])]


class LuciAnalysis:
    """Every question this tool asks of LUCI Analysis, for one project.

    Holds the connection pool, the request counters and the shutdown
    flag, so a process has one of these rather than a module full of
    globals. Query methods return domain objects; `request` and
    `paged_request` are there for calls this class does not wrap yet.
    """

    def __init__(self,
                 project: str = CHROMIUM_PROJECT,
                 host: str = LUCI_ANALYSIS_HOST,
                 timeout: float = REQUEST_TIMEOUT_SECS) -> None:

        # LUCI project every query is scoped to.
        self.project = project

        # Live request counters, for a caller's progress display to read.
        self.stats = RequestStats()

        # Connections shared by every request this client makes.
        self.pool = ConnectionPool(host, timeout)

        # Set once the process is shutting down, e.g. after Ctrl+C.
        # Requests already in flight still finish, but none of them
        # starts a further attempt, which would otherwise hold the
        # process open for the length of the retry backoff.
        self._shutdown = threading.Event()

    # -- lifecycle

    def request_shutdown(self) -> None:
        """Stop retrying failed requests, cutting short any backoff."""
        self._shutdown.set()

    def backoff(self, attempt: int) -> bool:
        """Wait before the next attempt.

        Returns:
            True if a shutdown was requested while waiting, in which case
            the caller must give up rather than retry.
        """
        return self._shutdown.wait(2**attempt)

    def close(self) -> None:
        """Release every pooled connection."""
        self.pool.close_all()

    # -- spawning requests

    def request(self, service: str, method: str, **body: Any) -> Request:
        """A one-shot call, with `project` filled in."""
        return Request(self, service, method, {
            "project": self.project,
            **body
        })

    def paged_request(self, service: str, method: str,
                      **body: Any) -> PagedRequest:
        """A paged call, with `project` and the largest page size."""
        return PagedRequest(self, service, method, {
            "project": self.project,
            "pageSize": MAX_PAGE_SIZE,
            **body
        })

    # -- the queries this tool makes

    def tests_matching(self, substring: str) -> list[str]:
        """Every test ID containing `substring`."""
        request = self.paged_request(TEST_HISTORY_SERVICE,
                                     "QueryTests",
                                     testIdSubstring=substring)
        return [
            test_id for page in request.pages()
            for test_id in page.get("testIds", [])
        ]

    def history(self, test_id: str, window: Window) -> list[StatsGroup]:
        """A test's verdict counts, by day and variant."""
        request = self.paged_request(TEST_HISTORY_SERVICE,
                                     "QueryStats",
                                     testId=test_id,
                                     predicate=window.as_predicate())
        return [StatsGroup.from_api(g) for g in request.gather("groups")]

    def verdicts(self, test_id: str, window: Window) -> list[TestVerdict]:
        """A test's individual verdicts.

        The fallback for when `history` comes back empty.
        """
        request = self.paged_request(TEST_HISTORY_SERVICE,
                                     "Query",
                                     testId=test_id,
                                     predicate=window.as_predicate())
        return [TestVerdict.from_api(v) for v in request.gather("verdicts")]

    def variants(self, test_id: str) -> list[TestVariant]:
        """The bot configurations a test has run on."""
        request = self.paged_request(TEST_HISTORY_SERVICE,
                                     "QueryVariants",
                                     testId=test_id)
        return [TestVariant.from_api(v) for v in request.gather("variants")]

    def cluster_summaries(self, failure_filter: str,
                          window: Window) -> list[ClusterSummary]:
        """The clusters with the most failures matching a filter.

        Upstream caps this at 200 clusters and does not paginate: a bare
        `LIMIT 200` in analysis/internal/analysis/cluster_summaries.go,
        documented nowhere in the proto.

        Args:
            failure_filter: AIP-160 filter applied to each failure, e.g.
                'test_id:":browser_tests!gtest"' (substring match).
            window: The span of failures to consider.
        """
        request = self.request(
            CLUSTERS_SERVICE,
            "QueryClusterSummaries",
            failureFilter=failure_filter,
            orderBy="metrics.`failures`.value desc",
            metrics=[f"projects/{self.project}/metrics/failures"],
            timeRange=window.as_time_range())
        summaries = request.send().get("clusterSummaries", [])
        return [ClusterSummary.from_api(s) for s in summaries]

    def cluster_failures(self,
                         cluster: ClusterSummary) -> list[ClusterFailure]:
        """Example failures from inside a cluster.

        Upstream serves at most 2000 groups, and only ever looks at the
        last 7 days whatever window the rest of the run uses; see
        ReadClusterFailures in
        analysis/internal/analysis/cluster_failures.go.
        """
        parent = (f"projects/{self.project}/clusters/{cluster.algorithm}/"
                  f"{cluster.cluster_id}/failures")
        request = Request(self, CLUSTERS_SERVICE, "QueryClusterFailures",
                          {"parent": parent})
        failures = request.send().get("failures", [])
        return [ClusterFailure.from_api(f) for f in failures]
