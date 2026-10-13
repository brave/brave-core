#!/usr/bin/env python3
"""Compares eval runs (plan P3.5).

  ./compare.py <run A> <run B>          # deltas, paired flips, hard gates, manifest diff
  ./compare.py --table <run> ...        # one row for each run
  ./compare.py --table --tag baseline-v1   # all runs with this tag in the results root

A run is a folder of step_eval.py (L1) or run_eval.py (L2). For two L1 runs
of the same step, the script pairs the cases by id and lists the cases that B
fixed and broke, with a two-sided sign test. It warns when the manifests
differ in more than one variable (plan, change rule 1).
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

from lib import clients
from lib import scoring as sc
from lib.steps import STEPS


def load(run: Path) -> dict:
    run = Path(run)
    out = {
        "dir": run,
        "manifest": json.loads((run / "manifest.json").read_text()),
    }
    if (run / "metrics.json").exists():
        out["metrics"] = json.loads((run / "metrics.json").read_text())
        out["records"] = [
            json.loads(l)
            for l in (run / "results.jsonl").read_text().splitlines()
            if l.strip()
        ]
        out["level"] = "L1"
    elif (run / "summary.json").exists():
        out["summary"] = json.loads((run / "summary.json").read_text())
        out["level"] = "L2"
    else:
        raise SystemExit(f"{run}: no metrics.json or summary.json")
    return out


def sign_test(fixed: int, broken: int) -> float:
    """Two-sided p-value of the binomial sign test."""
    n = fixed + broken
    if n == 0:
        return 1.0
    k = min(fixed, broken)
    p = sum(math.comb(n, i) for i in range(k + 1)) / 2**n
    return min(1.0, 2 * p)


def manifest_diff(a: dict, b: dict) -> list:
    """The variables that differ: models, params, variants, datasets, code."""
    diffs = []
    ma, mb = a["manifest"], b["manifest"]
    da = {
        (m["role"], m["name"], m.get("digest"))
        for m in ma.get("ollama", {}).get("models", [])
    }
    db = {
        (m["role"], m["name"], m.get("digest"))
        for m in mb.get("ollama", {}).get("models", [])
    }
    if da != db:
        diffs.append(f"models: {sorted(da)} -> {sorted(db)}")
    for key in ("params", "variants"):
        if ma.get(key) != mb.get(key):
            ka, kb = ma.get(key) or {}, mb.get(key) or {}
            changed = sorted(
                k for k in set(ka) | set(kb) if ka.get(k) != kb.get(k)
            )
            diffs.append(
                f"{key}: "
                + ", ".join(
                    f"{k} {ka.get(k)!r} -> {kb.get(k)!r}" for k in changed
                )
            )
    sa = {(d["path"], d["sha256"]) for d in ma.get("datasets", [])}
    sb = {(d["path"], d["sha256"]) for d in mb.get("datasets", [])}
    if sa != sb:
        diffs.append("datasets: different files or contents")
    if ma.get("git", {}).get("sha") != mb.get("git", {}).get("sha"):
        diffs.append(
            f"git: {ma['git'].get('sha', '')[:10]} -> {mb['git'].get('sha', '')[:10]}"
        )
    if ma.get("ollama", {}).get("version") != mb.get("ollama", {}).get(
        "version"
    ):
        diffs.append("ollama version")
    if bool(ma.get("serial")) != bool(mb.get("serial")):
        diffs.append(
            f"serial: {bool(ma.get('serial'))} -> {bool(mb.get('serial'))}"
        )
    return diffs


def _rates(m: dict, prefix: str = "") -> dict:
    """Every rate in a metrics dict, by dotted name."""
    out = {}
    for k, v in m.items():
        if isinstance(v, dict) and {"k", "n", "p"} <= set(v):
            out[prefix + k] = v
    return out


def _gates(m: dict) -> str:
    g = m.get("hard_gates", {})
    bad = [k for k, v in g.items() if v is False]
    known = [k for k, v in g.items() if v is not None]
    if not known:
        return "relative"
    return "pass" if not bad else "FAIL: " + ", ".join(bad)


def table(runs: list) -> list:
    md = [
        "| Run | Step | Split | Model | Variant | Primary | Hard gates | Latency | Requests |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for r in runs:
        if r["level"] != "L1":
            md.append(f"| {r['dir'].name} | L2 | | | | | | | |")
            continue
        m = r["metrics"]
        step = STEPS[m["step"]]
        for v, vm in m["variants"].items():
            serial = r["manifest"].get("serial")
            lat = sc.fmt_latency(vm["latency"]) + (
                "" if serial else " (parallel)"
            )
            md.append(
                f"| {r['dir'].name} | {m['step']} | {m['split']} | {m['model']} | {v} | "
                f"{step.primary} {sc.fmt_rate(vm[step.primary])} | {_gates(vm)} | {lat} | "
                f"{m['requests']['sent']} sent, {m['requests']['cached']} cached |"
            )
    return md


def _same_judge(a, b) -> bool:
    return (
        a is None
        or b is None
        or all(
            a.get(k) == b.get(k)
            for k in ("model", "thinking", "rubric_version")
        )
    )


def compare_l1(a: dict, b: dict, va: str | None, vb: str | None) -> list:
    ma, mb = a["metrics"], b["metrics"]
    if ma["step"] != mb["step"]:
        raise SystemExit(f"different steps: {ma['step']} and {mb['step']}")
    if not _same_judge(ma.get("judge"), mb.get("judge")):
        raise SystemExit(
            "the two runs have judge metrics from different judges "
            f"({ma['judge']} and {mb['judge']}); judge it again with one judge (plan 7.5, note 10)"
        )
    va = va or next(iter(ma["variants"]))
    vb = vb or next(iter(mb["variants"]))
    step = STEPS[ma["step"]]
    xa, xb = ma["variants"][va], mb["variants"][vb]
    md = [
        f"# Compare {ma['step']} ({ma['split']} vs {mb['split']})",
        "",
        f"A: `{a['dir'].name}` variant {va}, model {ma['model']}, {ma['repeats']} repeat(s).",
        f"B: `{b['dir'].name}` variant {vb}, model {mb['model']}, {mb['repeats']} repeat(s).",
        "",
    ]
    diffs = manifest_diff(a, b)
    if va != vb:
        diffs.append(f"variant: {va} -> {vb}")
    if len(diffs) > 1:
        md += [
            f"> WARNING: {len(diffs)} variables differ. Change one variable at a time.",
            "",
        ]
    md += [
        "Differences: " + ("; ".join(diffs) if diffs else "none"),
        "",
        "| Metric | A | B | Delta |",
        "|---|---|---|---|",
    ]
    ra, rb = _rates(xa), _rates(xb)
    for k in ra:
        if k in rb and ra[k]["p"] is not None and rb[k]["p"] is not None:
            mark = " (primary)" if k == step.primary else ""
            md.append(
                f"| {k}{mark} | {sc.fmt_rate(ra[k])} | {sc.fmt_rate(rb[k])} | "
                f"{rb[k]['p'] - ra[k]['p']:+.2f} |"
            )
    for k in sorted(set(xa) & set(xb)):
        if (
            isinstance(xa[k], (int, float))
            and not isinstance(xa[k], bool)
            and k not in ra
        ):
            md.append(
                f"| {k} | {xa[k]:.4g} | {xb[k]:.4g} | {xb[k] - xa[k]:+.4g} |"
            )
    md += ["", f"Hard gates: A {_gates(xa)}; B {_gates(xb)}.", ""]
    recs_a = [r for r in a["records"] if r["variant"] == va]
    recs_b = [r for r in b["records"] if r["variant"] == vb]
    fl = sc.flips(recs_a, recs_b)
    p = sign_test(len(fl["fixed"]), len(fl["broken"]))
    md += [
        f"Paired cases: {fl['paired']}. Fixed by B: {len(fl['fixed'])}. Broken by B: "
        f"{len(fl['broken'])}. Sign test p = {p:.3f}.",
        "",
    ]
    if fl["fixed"]:
        md.append("Fixed: " + ", ".join(fl["fixed"]))
    if fl["broken"]:
        md.append("Broken: " + ", ".join(fl["broken"]))
    if ma["split"] == "holdout" or mb["split"] == "holdout":
        md += ["", "Holdout: use only the totals (plan, data rule 2)."]
    return md


def compare_l2(a: dict, b: dict) -> list:
    def by_set(summary):
        """The mean of each number over the repeats of a set."""
        groups: dict = {}
        for s in summary:
            groups.setdefault(s["set"], []).append(s)
        out = {}
        for name, runs in groups.items():
            keys = [
                k
                for k, v in runs[0].items()
                if isinstance(v, (int, float)) and not isinstance(v, bool)
            ]
            out[name] = {
                k: sum(r[k] for r in runs if r.get(k) is not None)
                / max(1, sum(1 for r in runs if r.get(k) is not None))
                for k in keys
            }
            out[name]["runs"] = len(runs)
        return out

    sa, sb = by_set(a["summary"]), by_set(b["summary"])
    md = [f"# Compare L2: `{a['dir'].name}` vs `{b['dir'].name}`", ""]
    diffs = manifest_diff(a, b)
    if len(diffs) > 1:
        md += [
            f"> WARNING: {len(diffs)} variables differ. Change one variable at a time.",
            "",
        ]
    md += [
        "Differences: " + ("; ".join(diffs) if diffs else "none"),
        "",
        "| Set | Metric | A | B | Delta |",
        "|---|---|---|---|---|",
    ]
    for name in sorted(set(sa) & set(sb)):
        for k, v in sa[name].items():
            w = sb[name].get(k)
            if (
                isinstance(v, (int, float))
                and not isinstance(v, bool)
                and isinstance(w, (int, float))
            ):
                if v != w:
                    md.append(
                        f"| {name} | {k} | {v:.4g} | {w:.4g} | {w - v:+.4g} |"
                    )
    return md


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("runs", nargs="*")
    parser.add_argument("--table", action="store_true")
    parser.add_argument(
        "--tag",
        help="with --table: all runs whose folder name ends with this tag",
    )
    parser.add_argument("--results", default=None)
    parser.add_argument("--variant-a")
    parser.add_argument("--variant-b")
    args = parser.parse_args()
    paths = [Path(r) for r in args.runs]
    if args.tag:
        paths += sorted(
            p
            for p in clients.results_root(args.results).iterdir()
            if p.is_dir() and p.name.endswith(args.tag)
        )
    runs = [load(p) for p in paths]
    if args.table:
        print("\n".join(table(runs)))
        return
    if len(runs) != 2:
        sys.exit("give two runs, or --table")
    a, b = runs
    if a["level"] != b["level"]:
        sys.exit("one run is L1 and the other is L2")
    print(
        "\n".join(
            compare_l1(a, b, args.variant_a, args.variant_b)
            if a["level"] == "L1"
            else compare_l2(a, b)
        )
    )


if __name__ == "__main__":
    main()
