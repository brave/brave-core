# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Client and domain model for the Chromium LUCI Analysis API."""

from __future__ import annotations

import base64
import http.client
import json
import re
import ssl
import subprocess
import threading
import urllib.request
from collections import deque
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass, field
from datetime import date, datetime, timedelta, timezone
from enum import Enum
from pathlib import Path
from typing import Any

# A JSON object decoded from an API response, or built to send as one.
# Confined to this module: nothing above the client should see one.
JsonDict = dict[str, Any]

LUCI_ANALYSIS_HOST = "analysis.api.luci.app"
TEST_HISTORY_SERVICE = "luci.analysis.v1.TestHistory"
TEST_VARIANTS_SERVICE = "luci.analysis.v1.TestVariants"
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

# QueryStability rejects requests with more test variants than this.
MAX_STABILITY_BATCH = 100

# How far back QueryStability reads, counted from now whatever position is
# asked about; see analysis/internal/testresults/stability.
STABILITY_WINDOW_DAYS = 14

# How far back from today pooled flakes are counted, in whole days.
# QueryStability counts a bot's flakes ±7 days and ±1 weekday around the
# position; the branch keeps building past it, so these spans of the same
# width end today instead.
POOLED_WINDOW_DAYS = 14
POOLED_WINDOW_WEEKDAYS = 2

CHROMIUM_SRC = Path(__file__).resolve().parents[3]
GITILES = "https://chromium.googlesource.com/chromium/src"
GITILES_HOST = "chromium.googlesource.com"


class LuciAnalysisError(Exception):
    """Raised when a LUCI Analysis API request fails."""


# -- Time ----------------------------------------------------------------


@dataclass(frozen=True)
class Window:
    """A span of time, as the Clusters service asks for it."""

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

    def as_time_range(self) -> dict[str, str]:
        """The `timeRange` field the Clusters service takes."""
        return {
            "earliest": self.earliest.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "latest": self.latest.strftime("%Y-%m-%dT%H:%M:%SZ"),
        }


# -- Where to look -------------------------------------------------------


@dataclass(frozen=True)
class Source:
    """A position on a Chromium branch, and the LUCI project holding it."""

    # What was asked for, e.g. "checkout" or "157.0.8103.39".
    label: str
    commit: str
    # e.g. "refs/branch-heads/8103" or "refs/heads/main".
    ref: str
    position: int
    milestone: int

    @property
    def project(self) -> str:
        """Branch results live in a LUCI project per milestone."""
        if self.ref == "refs/heads/main":
            return "chromium"
        if self.ref.startswith("refs/branch-heads/"):
            return f"chromium-m{self.milestone}"
        raise LuciAnalysisError(f"No LUCI project is known for {self.ref}.")

    @property
    def description(self) -> str:
        """e.g. "refs/branch-heads/8103@{#1035}"."""
        return f"{self.ref}@{{#{self.position}}}"

    def as_sources(self) -> JsonDict:
        """The `sources` field QueryStability takes."""
        return {
            "gitilesCommit": {
                "host": GITILES_HOST,
                "project": "chromium/src",
                "ref": self.ref,
                "commitHash": self.commit,
                "position": str(self.position),
            }
        }


def parse_commit_position(message: str) -> tuple[str, int]:
    """The ref and position of a commit's Cr-Commit-Position trailer."""
    match = re.search(
        r"^Cr-Commit-Position: (\S+)@\{#(\d+)\}$", message, re.MULTILINE
    )
    if not match:
        raise LuciAnalysisError("Commit has no Cr-Commit-Position trailer.")
    return match.group(1), int(match.group(2))


def parse_milestone(version_file: str) -> int:
    """MAJOR from the contents of chrome/VERSION."""
    match = re.search(r"^MAJOR=(\d+)$", version_file, re.MULTILINE)
    if not match:
        raise LuciAnalysisError("chrome/VERSION has no MAJOR line.")
    return int(match.group(1))


def source_from_checkout() -> Source:
    """The position the local Chromium checkout is at."""
    commit, _, message = subprocess.run(
        ["git", "-C", str(CHROMIUM_SRC), "log", "-1", "--format=%H%n%B"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.partition("\n")
    ref, position = parse_commit_position(message)
    version = (CHROMIUM_SRC / "chrome" / "VERSION").read_text()
    return Source("checkout", commit, ref, position, parse_milestone(version))


def _gitiles(path: str) -> bytes:
    try:
        with urllib.request.urlopen(f"{GITILES}/+{path}", timeout=30) as r:
            return r.read()
    except OSError as e:
        raise LuciAnalysisError(
            f"Could not read {path} from gitiles: {e}"
        ) from e


def source_from_gitiles(committish: str) -> Source:
    """The position of a tag, branch or commit in chromium/src."""
    raw = _gitiles(f"log/{committish}?format=JSON&n=1")
    commit = json.loads(raw[raw.find(b"\n") + 1 :])["log"][0]
    ref, position = parse_commit_position(commit["message"])
    version = base64.b64decode(
        _gitiles(f"/{commit['commit']}/chrome/VERSION?format=TEXT")
    ).decode()
    return Source(
        committish, commit["commit"], ref, position, parse_milestone(version)
    )


def source_for(ref: str | None) -> Source:
    """`ref` resolved through gitiles, or the checkout when it is None."""
    return source_from_gitiles(ref) if ref else source_from_checkout()


# -- What upstream says --------------------------------------------------


class Status(Enum):
    """How upstream reads one test variant near one position."""

    FLAKY = "flaky"
    FAILING = "failing"
    STABLE = "stable"
    NO_DATA = "no_data"


@dataclass(frozen=True)
class Stability:
    """Upstream's stability analysis of one test variant."""

    test_id: str
    # The variant definition: builder, os, test_suite and so on.
    variant: dict[str, str]
    # TestVariantStabilityAnalysis as returned. Zero and false fields are
    # omitted on the wire.
    analysis: JsonDict

    @property
    def _flake(self) -> JsonDict:
        return self.analysis.get("flakeRate", {})

    @property
    def _failure(self) -> JsonDict:
        return self.analysis.get("failureRate", {})

    def flake(self, name: str) -> int:
        """An integer field of the flake-rate analysis."""
        return int(self._flake.get(name, 0))

    def failure(self, name: str) -> int:
        """An integer field of the failure-rate analysis."""
        return int(self._failure.get(name, 0))

    @property
    def status(self) -> Status:
        if self._failure.get("isMet"):
            return Status.FAILING
        if self._flake.get("isMet"):
            return Status.FLAKY
        if self.flake("totalVerdicts") or self._failure.get("recentVerdicts"):
            return Status.STABLE
        return Status.NO_DATA

    @property
    def confirmed(self) -> bool:
        """Whether the test meets upstream's flake or failure criteria."""
        return self.status in (Status.FLAKY, Status.FAILING)

    @property
    def example_builds(self) -> list[str]:
        """Links to builds where the test was flaky."""
        builds = []
        for example in self._flake.get("flakeExamples", []):
            for inv in example.get("rootInvocations") or example.get(
                "invocations", []
            ):
                if inv.startswith("build-"):
                    builds.append(f"https://ci.chromium.org/b/{inv[6:]}")
        return builds

    def as_json(self) -> JsonDict:
        return {
            "test_id": self.test_id,
            "variant": self.variant,
            "variant_hash": self.analysis.get("variantHash", ""),
            "status": self.status.value,
            "flake_rate_met": bool(self._flake.get("isMet")),
            "failure_rate_met": bool(self._failure.get("isMet")),
            "run_flaky_verdicts": self.flake("runFlakyVerdicts"),
            "total_verdicts": self.flake("totalVerdicts"),
            "run_flaky_verdicts_1wd": self.flake("runFlakyVerdicts1wd"),
            "run_flaky_verdicts_12h": self.flake("runFlakyVerdicts12h"),
            "unexpected_test_runs": self.failure("unexpectedTestRuns"),
            "consecutive_unexpected_test_runs": self.failure(
                "consecutiveUnexpectedTestRuns"
            ),
            "example_builds": self.example_builds,
        }


def weekdays_back(today: date, count: int) -> date:
    """The first day of the span ending today that holds `count` weekdays.

    Weekend days inside the span count towards it, as they do upstream.
    """
    start = today
    while True:
        if start.weekday() < 5:
            count -= 1
            if count == 0:
                return start
        start -= timedelta(days=1)


@dataclass(frozen=True)
class PooledFlakes:
    """Flaky verdicts of several bots counted together.

    QueryStability only counts a verdict as flaky when a single run both
    failed and passed. Chromium retries failed browser tests in a separate
    run, so a test that flakes that way is never flaky to it. Counting the
    FLAKY verdicts of several bots against the same thresholds catches it;
    the thresholds are counts with no rate, so pooling dilutes nothing.
    """

    # FLAKY verdicts in the last POOLED_WINDOW_DAYS days.
    flaky: int
    # Of those, the ones in the last POOLED_WINDOW_WEEKDAYS weekdays.
    flaky_1wd: int
    # The project's flake_threshold and flake_threshold_1wd.
    threshold: int
    threshold_1wd: int

    @classmethod
    def tally(
        cls, days: list[FlakyDay], criteria: JsonDict, today: date
    ) -> PooledFlakes:
        """Count `days` against the thresholds in `criteria`."""
        first_day = today - timedelta(days=POOLED_WINDOW_DAYS - 1)
        first_weekday = weekdays_back(today, POOLED_WINDOW_WEEKDAYS)
        recent = [day for day in days if day.day >= first_day]
        flake_rate = criteria.get("flakeRate", {})
        return cls(
            flaky=sum(day.flaky for day in recent),
            flaky_1wd=sum(
                day.flaky for day in recent if day.day >= first_weekday
            ),
            threshold=int(flake_rate.get("flakeThreshold", 0)),
            threshold_1wd=int(flake_rate.get("flakeThreshold1wd", 0)),
        )

    @property
    def met(self) -> bool:
        """Whether the pooled verdicts meet the project's flake thresholds."""
        return (
            self.threshold > 0
            and self.flaky >= self.threshold
            and self.flaky_1wd >= self.threshold_1wd
        )

    def as_json(self) -> JsonDict:
        return {
            "met": self.met,
            "flaky_verdicts": self.flaky,
            "flaky_verdicts_1wd": self.flaky_1wd,
        }


# -- What the service returns --------------------------------------------


@dataclass(frozen=True)
class FlakyDay:
    """How many FLAKY verdicts one bot had on one day."""

    day: date
    variant_hash: str
    flaky: int

    @classmethod
    def from_api(cls, group: JsonDict) -> FlakyDay:
        """Read a TestHistory.QueryStats group."""
        return cls(
            day=date.fromisoformat(group["partitionTime"][:10]),
            variant_hash=group.get("variantHash", ""),
            flaky=int(group.get("verdictCounts", {}).get("flaky", 0)),
        )


@dataclass(frozen=True)
class TestVariant:
    """A bot configuration a test has run on."""

    # Identifies the variant in other requests.
    variant_hash: str

    # OS of the machine that ran the test, not the platform under test:
    # iOS simulator bots report a Mac os, ChromeOS builds report Ubuntu.
    os: str

    # The builder, plus the builder whose failures it is retrying:
    # Chromium's flake-retry bots run as "runner" in bucket "reviver" and
    # name the original in "reviver_builder".
    builders: tuple[str, ...]

    # The whole variant definition, keys as above plus e.g. test_suite.
    definition: dict[str, str] = field(
        default_factory=dict, compare=False, hash=False
    )

    @classmethod
    def from_api(cls, entry: JsonDict) -> TestVariant:
        """Read a QueryVariants entry."""
        definition = entry.get("variant", {}).get("def", {})
        names = (
            definition.get("builder", ""),
            definition.get("reviver_builder", ""),
        )
        return cls(
            variant_hash=entry.get("variantHash", ""),
            os=definition.get("os", ""),
            builders=tuple(name for name in names if name),
            definition=definition,
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
        return not (
            "%" in self.title or "\\\\" in self.title or "\\_" in self.title
        )


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
        return cls(
            test_id=failure.get("testId", ""),
            count=int(failure.get("count", 1)),
        )


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
            connection = http.client.HTTPSConnection(
                self._host, timeout=self._timeout, context=TLS_CONTEXT
            )
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

    def __init__(
        self, client: LuciAnalysis, service: str, method: str, body: JsonDict
    ) -> None:

        # Whose pool, counters and shutdown flag this call works
        # against.
        self._client = client

        # Full pRPC service name, e.g. "luci.analysis.v1.TestHistory".
        self._service = service

        # RPC method on that service, e.g. "QueryStability".
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
                    connection.request(
                        "POST", self.path, body=data, headers=headers
                    )
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
                    f"Could not reach LUCI Analysis API: {e}"
                ) from e

            if status == 200:
                return raw
            if status == 403:
                raise LuciAnalysisError(
                    "403 Forbidden from LUCI Analysis API. The API may"
                    " require authentication for this query."
                )
            if status == 404:
                raise LuciAnalysisError(
                    f"404 Not Found for method {self._method}."
                )
            # Retry transient server errors and rate limiting.
            if (
                (status >= 500 or status == 429)
                and attempt < MAX_ATTEMPTS
                and not client.backoff(attempt)
            ):
                client.stats.retries += 1
                continue
            raise LuciAnalysisError(
                f"HTTP {status} from LUCI Analysis API: {reason}"
            )
        raise AssertionError("the retry loop always returns or raises")

    @staticmethod
    def _decode(raw: bytes) -> JsonDict:
        """Strip the pRPC XSSI prefix and parse what is left.

        The prefix is )]}' followed by a newline, so everything up to and
        including the first newline goes.
        """
        newline = raw.find(b"\n")
        if newline >= 0:
            raw = raw[newline + 1 :]
        try:
            return json.loads(raw)
        except json.JSONDecodeError as e:
            raise LuciAnalysisError(
                "Could not parse API response as JSON. Raw response (first"
                f" 500 bytes): {raw[:500]}"
            ) from e


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

    def gather(self, key: str) -> list[JsonDict]:
        """Concatenate `key` across every page."""
        return [item for page in self.pages() for item in page.get(key, [])]


class LuciAnalysis:
    """Every question this tool asks of LUCI Analysis, for one project.

    Holds the connection pool, the request counters and the shutdown
    flag, so a process has one of these rather than a module full of
    globals. Query methods return domain objects; `request` and
    `paged_request` are there for calls this class does not wrap yet.
    """

    def __init__(
        self,
        project: str = CHROMIUM_PROJECT,
        host: str = LUCI_ANALYSIS_HOST,
        timeout: float = REQUEST_TIMEOUT_SECS,
    ) -> None:

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
        return Request(self, service, method, {"project": self.project, **body})

    def paged_request(
        self, service: str, method: str, **body: Any
    ) -> PagedRequest:
        """A paged call, with `project` and the largest page size."""
        return PagedRequest(
            self,
            service,
            method,
            {"project": self.project, "pageSize": MAX_PAGE_SIZE, **body},
        )

    # -- the queries this tool makes

    def tests_matching(self, substring: str) -> list[str]:
        """Every test ID containing `substring`."""
        request = self.paged_request(
            TEST_HISTORY_SERVICE, "QueryTests", testIdSubstring=substring
        )
        return [
            test_id
            for page in request.pages()
            for test_id in page.get("testIds", [])
        ]

    def variants(self, test_id: str) -> list[TestVariant]:
        """The bot configurations a test has run on."""
        request = self.paged_request(
            TEST_HISTORY_SERVICE, "QueryVariants", testId=test_id
        )
        return [TestVariant.from_api(v) for v in request.gather("variants")]

    def ci_flaky_days(self, test_id: str, window: Window) -> list[FlakyDay]:
        """A test's FLAKY verdicts on CI (postsubmit) bots, by day and bot."""
        request = self.paged_request(
            TEST_HISTORY_SERVICE,
            "QueryStats",
            testId=test_id,
            predicate={
                "partitionTimeRange": window.as_time_range(),
                "variantPredicate": {"contains": {"def": {"bucket": "ci"}}},
            },
        )
        return [
            day
            for day in map(FlakyDay.from_api, request.gather("groups"))
            if day.flaky
        ]

    def stability(
        self, variants: list[tuple[str, TestVariant]], source: Source
    ) -> tuple[list[Stability], JsonDict]:
        """Upstream's stability analysis of each (test ID, variant).

        Returns:
            One Stability per input, in input order, and the project's
            criteria they were judged by.
        """
        results: list[Stability] = []
        criteria: JsonDict = {}
        for start in range(0, len(variants), MAX_STABILITY_BATCH):
            batch = variants[start : start + MAX_STABILITY_BATCH]
            response = self.request(
                TEST_VARIANTS_SERVICE,
                "QueryStability",
                testVariants=[
                    {
                        "testId": test_id,
                        "variantHash": variant.variant_hash,
                        "sources": source.as_sources(),
                    }
                    for test_id, variant in batch
                ],
            ).send()
            criteria = response.get("criteria", criteria)
            # Answers come back in request order.
            for (test_id, variant), analysis in zip(
                batch, response.get("testVariants", [])
            ):
                results.append(Stability(test_id, variant.definition, analysis))
        return results, criteria

    def cluster_summaries(
        self, failure_filter: str, window: Window
    ) -> list[ClusterSummary]:
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
            timeRange=window.as_time_range(),
        )
        summaries = request.send().get("clusterSummaries", [])
        return [ClusterSummary.from_api(s) for s in summaries]

    def cluster_failures(self, cluster: ClusterSummary) -> list[ClusterFailure]:
        """Example failures from inside a cluster.

        Upstream serves at most 2000 groups, and only ever looks at the
        last 7 days whatever window the rest of the run uses; see
        ReadClusterFailures in
        analysis/internal/analysis/cluster_failures.go.
        """
        parent = (
            f"projects/{self.project}/clusters/{cluster.algorithm}/"
            f"{cluster.cluster_id}/failures"
        )
        request = Request(
            self, CLUSTERS_SERVICE, "QueryClusterFailures", {"parent": parent}
        )
        failures = request.send().get("failures", [])
        return [ClusterFailure.from_api(f) for f in failures]
