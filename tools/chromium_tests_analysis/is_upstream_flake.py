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
import json
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Any

from rich import box
from rich.console import Console
from rich.table import Table
from rich.text import Text

from luci_analysis import (
    POOLED_WINDOW_DAYS,
    POOLED_WINDOW_WEEKDAYS,
    LuciAnalysis,
    LuciAnalysisError,
    PooledFlakes,
    Source,
    Stability,
    Status,
    Window,
    source_for,
)

# Past this many matching test IDs the query is almost certainly too broad.
MAX_TEST_IDS = 20

EXIT_CONFIRMED = 0
EXIT_ERROR = 1
EXIT_NOT_CONFIRMED = 3

console = Console()


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


_STATUS_STYLE = {
    Status.FLAKY: "bold red",
    Status.FAILING: "bold magenta",
    Status.STABLE: "green",
    Status.NO_DATA: "dim",
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

    variants = [
        (test_id, variant)
        for test_id in test_ids
        for variant in client.variants(test_id)
        if filters.accept(variant.definition)
    ]
    return client.stability(variants, source)


@dataclass(frozen=True)
class Pool:
    """One test's CI bots on one bot OS, with their flakes counted together."""

    test_id: str
    # The first part of the bot OS, e.g. "Windows" or "Ubuntu".
    os_family: str
    builders: tuple[str, ...]
    flakes: PooledFlakes

    def as_json(self) -> dict[str, Any]:
        return {
            "test_id": self.test_id,
            "os_family": self.os_family,
            "builders": list(self.builders),
            **self.flakes.as_json(),
        }


def pool(
    client: LuciAnalysis, results: list[Stability], criteria: dict[str, Any]
) -> list[Pool]:
    """Pool each test's CI bots by bot OS.

    QueryStability only counts a flake that failed and passed within one run;
    Chromium retries failed browser tests in a separate run, so a pool can be
    flaky when none of its bots is.
    """
    hashes: dict[tuple[str, str], dict[str, str]] = defaultdict(dict)
    for r in results:
        if r.variant.get("bucket") == "ci":
            family = r.variant.get("os", "").split("-")[0]
            hashes[r.test_id, family][r.analysis.get("variantHash", "")] = (
                r.variant.get("builder", "")
            )
    window = Window.last_days(POOLED_WINDOW_DAYS)
    today = datetime.now(timezone.utc).date()
    days_by_test = {
        test_id: client.ci_flaky_days(test_id, window)
        for test_id in dict.fromkeys(test_id for test_id, _ in hashes)
    }
    return [
        Pool(
            test_id,
            family,
            tuple(sorted(f"ci/{name}" for name in set(bots.values()))),
            PooledFlakes.tally(
                [d for d in days_by_test[test_id] if d.variant_hash in bots],
                criteria,
                today,
            ),
        )
        for (test_id, family), bots in sorted(hashes.items())
    ]


# -- Reporting ----------------------------------------------------------------


def render(
    results: list[Stability],
    pools: list[Pool],
    criteria: dict[str, Any],
    source: Source,
    show_all: bool,
) -> None:
    console.print(
        f"[bold]{source.label}[/]: {source.description}"
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
        for p in pools:
            if p.test_id == test_id and p.flakes.flaky:
                style = "bold red" if p.flakes.met else "none"
                console.print(
                    f"  {p.os_family} CI bots together:"
                    f" {p.flakes.flaky} flaky in {POOLED_WINDOW_DAYS} days,"
                    f" {p.flakes.flaky_1wd} in the last"
                    f" {POOLED_WINDOW_WEEKDAYS} weekdays"
                    + (" -> FLAKY" if p.flakes.met else ""),
                    style=style,
                )
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
    pooled = [p for p in pools if p.flakes.met]
    console.print()
    if confirmed or pooled:
        parts = []
        if confirmed:
            parts.append(f"{len(confirmed)} variant(s)")
        if pooled:
            parts.append(
                "pooled CI bots on "
                + ", ".join(sorted({p.os_family for p in pooled}))
            )
        console.print(
            f"CONFIRMED upstream on {' and '.join(parts)}.",
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

    source = source_for(args.ref)
    client = LuciAnalysis(project=source.project)
    results, criteria = query(
        client,
        gtest_name(args.test),
        Filters(args.builder, args.os, args.suite),
        source,
    )
    pools = pool(client, results, criteria)
    confirmed = any(r.confirmed for r in results) or any(
        p.flakes.met for p in pools
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
                    "confirmed": confirmed,
                    "variants": [r.as_json() for r in results],
                    "pooled": [p.as_json() for p in pools],
                },
                indent=2,
            )
        )
    else:
        render(results, pools, criteria, source, args.all)
    return EXIT_CONFIRMED if confirmed else EXIT_NOT_CONFIRMED


if __name__ == "__main__":
    try:
        sys.exit(main())
    except LuciAnalysisError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(EXIT_ERROR)
