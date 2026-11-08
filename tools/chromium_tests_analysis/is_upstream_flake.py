#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Ask LUCI Analysis whether an upstream test is a confirmed flake, per variant,
on the Chromium branch Brave is built from.

This is the same question Chromium's CQ asks before exonerating a failure
(TestVariants.QueryStability, see build/recipes/recipe_modules/test_utils): for
one test, on one variant (builder, os, test suite), at one position on one
branch, does the history in the surrounding days meet the project's flake-rate
or failure-rate criteria? The criteria and the counting are upstream's own;
this script only picks the inputs and reports the answer.

By default the branch and position are those of the Chromium checkout
(`src/`), e.g. refs/branch-heads/8103@{#1035} for 157.0.8103.39. Branch
results live in their own LUCI project (chromium-m157), main in `chromium`.

Usage:
    vpython3 tools/chromium_tests_analysis/is_upstream_flake.py \\
        "Suite.Test" [--ref 157.0.8103.39|main] [--builder REGEX] \\
        [--os REGEX] [--suite REGEX] [--all] [--json]

Exit status is 0 when at least one selected variant is confirmed flaky or
failing upstream, 3 when none is, and 1 on errors.
"""

from __future__ import annotations

import argparse
import base64
import json
import re
import subprocess
import sys
import urllib.request
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Any

from rich import box
from rich.console import Console
from rich.table import Table
from rich.text import Text

from luci_analysis import LuciAnalysis, LuciAnalysisError

CHROMIUM_SRC = Path(__file__).resolve().parents[3]
GITILES = "https://chromium.googlesource.com/chromium/src"
GITILES_HOST = "chromium.googlesource.com"
TEST_VARIANTS_SERVICE = "luci.analysis.v1.TestVariants"
TEST_HISTORY_SERVICE = "luci.analysis.v1.TestHistory"

# QueryStability rejects requests with more test variants than this.
MAX_STABILITY_BATCH = 100

# Past this many matching test IDs the query is almost certainly too broad.
MAX_TEST_IDS = 20

EXIT_CONFIRMED = 0
EXIT_ERROR = 1
EXIT_NOT_CONFIRMED = 3

console = Console()


# -- Where to look ------------------------------------------------------------


@dataclass(frozen=True)
class Source:
    """A position on a Chromium branch, and the LUCI project holding it."""

    # What the user asked for, e.g. "checkout" or "157.0.8103.39".
    label: str
    commit: str
    # e.g. "refs/branch-heads/8103" or "refs/heads/main".
    ref: str
    position: int
    milestone: int

    @property
    def project(self) -> str:
        if self.ref == "refs/heads/main":
            return "chromium"
        if self.ref.startswith("refs/branch-heads/"):
            return f"chromium-m{self.milestone}"
        raise LuciAnalysisError(f"No LUCI project is known for {self.ref}.")

    def as_sources(self) -> dict[str, Any]:
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


def gitiles(path: str) -> bytes:
    try:
        with urllib.request.urlopen(f"{GITILES}/+{path}", timeout=30) as r:
            return r.read()
    except OSError as e:
        raise LuciAnalysisError(
            f"Could not read {path} from gitiles: {e}"
        ) from e


def source_from_gitiles(committish: str) -> Source:
    """The position of a tag, branch or commit in chromium/src."""
    raw = gitiles(f"log/{committish}?format=JSON&n=1")
    commit = json.loads(raw[raw.find(b"\n") + 1 :])["log"][0]
    ref, position = parse_commit_position(commit["message"])
    version = base64.b64decode(
        gitiles(f"/{commit['commit']}/chrome/VERSION?format=TEXT")
    ).decode()
    return Source(
        committish, commit["commit"], ref, position, parse_milestone(version)
    )


# -- What to look at ----------------------------------------------------------


def gtest_name(test_name: str) -> str:
    """The LUCI form of a gtest name, as it appears after "::".

    "Suite.Test" becomes "Suite#Test", and the parameterised
    "Inst/Suite.Test/Param" becomes "Suite#Test/Inst.Param".
    """
    match = re.fullmatch(
        r"(?:([^/.]+)/)?([^/.]+)\.([^/]+)(?:/(.+))?", test_name
    )
    if not match:
        return test_name
    inst, suite, test, param = match.groups()
    name = f"{suite}#{test}"
    if inst and param:
        return f"{name}/{inst}.{param}"
    if param:
        return f"{name}/{param}"
    return name


def matches_test(test_id: str, name: str) -> bool:
    """Whether a test ID is `name`, or one of its parameterisations."""
    _, _, tail = test_id.partition("::")
    return tail == name or tail.startswith(name + "/")


@dataclass(frozen=True)
class Filters:
    """Regexes a variant's keys must match, case-insensitively."""

    builder: str | None = None
    os: str | None = None
    suite: str | None = None

    def accept(self, variant: dict[str, str]) -> bool:
        for key, pattern in (
            ("builder", self.builder),
            ("os", self.os),
            ("test_suite", self.suite),
        ):
            if pattern and not re.search(
                pattern, variant.get(key, ""), re.IGNORECASE
            ):
                return False
        return True


# -- What upstream says -------------------------------------------------------


class Status(Enum):
    FLAKY = "flaky"
    FAILING = "failing"
    STABLE = "stable"
    NO_DATA = "no_data"


_STATUS_STYLE = {
    Status.FLAKY: "bold red",
    Status.FAILING: "bold magenta",
    Status.STABLE: "green",
    Status.NO_DATA: "dim",
}


@dataclass(frozen=True)
class Stability:
    """Upstream's stability analysis of one test variant."""

    test_id: str
    variant: dict[str, str]
    # TestVariantStabilityAnalysis as returned. Zero and false fields are
    # omitted on the wire.
    analysis: dict[str, Any]

    @property
    def _flake(self) -> dict[str, Any]:
        return self.analysis.get("flakeRate", {})

    @property
    def _failure(self) -> dict[str, Any]:
        return self.analysis.get("failureRate", {})

    def flake(self, field: str) -> int:
        return int(self._flake.get(field, 0))

    def failure(self, field: str) -> int:
        return int(self._failure.get(field, 0))

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

    def as_json(self) -> dict[str, Any]:
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


def describe_criteria(criteria: dict[str, Any]) -> str:
    flake = criteria.get("flakeRate", {})
    failure = criteria.get("failureRate", {})
    parts = [
        f"flaky if >= {flake.get('flakeThreshold', 0)} flaky verdicts"
        " within ±7 days"
    ]
    if flake.get("flakeThreshold1wd"):
        parts.append(f">= {flake['flakeThreshold1wd']} within ±1 weekday")
    if flake.get("flakeRateThreshold"):
        parts.append(f"rate >= {float(flake['flakeRateThreshold']):.1%}")
    return (
        ", ".join(parts)
        + f"; failing if >= {failure.get('failureThreshold', 0)} of 10 runs"
        f" unexpected, or >= {failure.get('consecutiveFailureThreshold', 0)}"
        " consecutive"
    )


def query(
    client: LuciAnalysis, name: str, filters: Filters, source: Source
) -> tuple[list[Stability], dict[str, Any]]:
    """Every matching test variant's stability, and the criteria applied."""
    test_ids = [
        test_id
        for test_id in client.tests_matching(name)
        if matches_test(test_id, name)
    ]
    if len(test_ids) > MAX_TEST_IDS:
        raise LuciAnalysisError(
            f"{len(test_ids)} test IDs match {name}; pass a more specific name."
        )

    positions = []
    for test_id in test_ids:
        for entry in client.paged_request(
            TEST_HISTORY_SERVICE, "QueryVariants", testId=test_id
        ).gather("variants"):
            variant = entry.get("variant", {}).get("def", {})
            if filters.accept(variant):
                positions.append((test_id, variant, entry["variantHash"]))

    results: list[Stability] = []
    criteria: dict[str, Any] = {}
    for start in range(0, len(positions), MAX_STABILITY_BATCH):
        batch = positions[start : start + MAX_STABILITY_BATCH]
        response = client.request(
            TEST_VARIANTS_SERVICE,
            "QueryStability",
            testVariants=[
                {
                    "testId": test_id,
                    "variantHash": variant_hash,
                    "sources": source.as_sources(),
                }
                for test_id, _, variant_hash in batch
            ],
        ).send()
        criteria = response.get("criteria", criteria)
        # Answers come back in request order.
        for (test_id, variant, _), analysis in zip(
            batch, response.get("testVariants", [])
        ):
            results.append(Stability(test_id, variant, analysis))
    return results, criteria


# -- Reporting ----------------------------------------------------------------


def render(
    results: list[Stability],
    criteria: dict[str, Any],
    source: Source,
    show_all: bool,
) -> None:
    console.print(
        f"[bold]{source.label}[/]: {source.ref}@{{#{source.position}}}"
        f" (LUCI project {source.project})"
    )
    if criteria:
        console.print(f"Criteria: {describe_criteria(criteria)}", style="dim")

    shown = [r for r in results if show_all or r.status != Status.NO_DATA]
    order = list(Status)
    shown.sort(
        key=lambda r: (
            r.test_id,
            order.index(r.status),
            r.variant.get("builder", ""),
        )
    )
    for test_id in dict.fromkeys(r.test_id for r in shown):
        table = Table(box=box.SIMPLE_HEAD, title=test_id, title_justify="left")
        table.add_column("Builder", no_wrap=True)
        table.add_column("OS", no_wrap=True)
        table.add_column("Suite", no_wrap=True)
        table.add_column("Status", no_wrap=True)
        table.add_column("Flaky ±7d", justify="right", no_wrap=True)
        table.add_column("±1wd", justify="right", no_wrap=True)
        table.add_column("Unexpected", justify="right", no_wrap=True)
        for r in shown:
            if r.test_id != test_id:
                continue
            builder = r.variant.get("builder", "")
            if r.variant.get("bucket"):
                builder = f"{r.variant['bucket']}/{builder}"
            table.add_row(
                builder,
                r.variant.get("os", ""),
                r.variant.get("test_suite", ""),
                Text(r.status.value.upper(), style=_STATUS_STYLE[r.status]),
                f"{r.flake('runFlakyVerdicts')}/{r.flake('totalVerdicts')}",
                str(r.flake("runFlakyVerdicts1wd")),
                str(r.failure("unexpectedTestRuns")),
            )
        console.print()
        console.print(table)
        for r in shown:
            if r.test_id == test_id and r.example_builds:
                console.print(
                    f"  {r.variant.get('builder', '')}: "
                    + " ".join(r.example_builds[:3]),
                    overflow="fold",
                )

    hidden = len(results) - len(shown)
    if hidden:
        console.print(
            f"\n{hidden} variant(s) had no results near this position"
            " (--all to list them).",
            style="dim",
        )
    if not results:
        console.print("No matching test variants were found.")
        return
    confirmed = [r for r in results if r.confirmed]
    console.print()
    if confirmed:
        console.print(
            f"CONFIRMED upstream on {len(confirmed)} variant(s).",
            style="bold red",
        )
    else:
        console.print("Not confirmed upstream on any variant.", style="green")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Ask LUCI Analysis whether an upstream test is a confirmed flake,"
            " per variant, at a position on a Chromium branch."
        )
    )
    parser.add_argument(
        "test",
        help=(
            "gtest name ('Suite.Test', 'Inst/Suite.Test/0') or the part of a"
            " LUCI test ID after '::'."
        ),
    )
    parser.add_argument(
        "--ref",
        help=(
            "chromium/src tag, branch or commit to check at, e.g."
            " 157.0.8103.39, refs/branch-heads/8103 or main. Defaults to the"
            " local Chromium checkout."
        ),
    )
    parser.add_argument("--builder", help="Regex the builder must match.")
    parser.add_argument("--os", help="Regex the bot OS must match.")
    parser.add_argument("--suite", help="Regex the test suite must match.")
    parser.add_argument(
        "--all",
        action="store_true",
        help="Also list variants with no results near the position.",
    )
    parser.add_argument(
        "--json", action="store_true", help="Print JSON instead of tables."
    )
    args = parser.parse_args()

    source = (
        source_from_gitiles(args.ref) if args.ref else source_from_checkout()
    )
    client = LuciAnalysis(project=source.project)
    results, criteria = query(
        client,
        gtest_name(args.test),
        Filters(args.builder, args.os, args.suite),
        source,
    )

    if args.json:
        print(
            json.dumps(
                {
                    "test": args.test,
                    "project": source.project,
                    "ref": source.ref,
                    "position": source.position,
                    "commit": source.commit,
                    "criteria": criteria,
                    "confirmed": any(r.confirmed for r in results),
                    "variants": [r.as_json() for r in results],
                },
                indent=2,
            )
        )
    else:
        render(results, criteria, source, args.all)
    return (
        EXIT_CONFIRMED
        if any(r.confirmed for r in results)
        else EXIT_NOT_CONFIRMED
    )


if __name__ == "__main__":
    try:
        sys.exit(main())
    except LuciAnalysisError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(EXIT_ERROR)
