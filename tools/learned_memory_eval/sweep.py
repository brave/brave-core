#!/usr/bin/env python3
"""Sweeps the keep thresholds offline, from the traces of an L2 run (plan P3.4).

The decision model is repeatable (noise_check.py), so the probabilities in a
trace are the answers that any threshold would get. The script computes the
keep decisions of DreamingRun for a grid of thresholds, and scores the kept
sentences against gold.turns of each set. No model runs.

  ./sweep.py <run>                                  # the run's own thresholds
  ./sweep.py <run> --gate 0.1 0.2 0.3 --fact 0.5 0.6 0.7
  ./sweep.py <run> --sensitive 0.2 0.25 0.3 --short-lived 0.4 0.5 0.6

Limit: the trace has sentence answers only for the turns that passed the gate
of the run. A gate threshold below the run's gate threshold cannot be exact,
so the table shows those turns as "unknown". To sweep the gate fully, run
run_eval.py one time with --gate-threshold 0.

Scores, for each grid point:
- wanted: sentences of a turn with required gold facts that match a fact of
  the turn (the words of match.all). Kept / wanted is the recall.
- wrong keeps: sentences kept from a turn whose gold.turns list is empty.
- forbidden keeps: kept sentences that match a forbidden item (must be 0).
- extra keeps: other kept sentences of fact turns (for example the question).
"""
from __future__ import annotations

import argparse
import itertools
import json
import sys
from pathlib import Path

from lib import gold as gold_lib
from lib import scoring as sc
from lib.steps import keep_rule

PARAMS = [("gate", "gate_threshold"), ("fact", "fact_threshold"),
          ("sensitive", "max_sensitive"), ("instruction", "max_instruction"),
          ("short_lived", "max_short_lived")]


def load_runs(run: Path, gold_from: Path | None) -> list:
    out = []
    paths = sorted(run.glob("*/r*/report.json")) or sorted(run.glob("*/report.json"))
    for path in paths:
        chat_set = json.loads((path.parent / "chats.json").read_text())
        if gold_from and (gold_from / f"{chat_set['name']}.json").exists():
            chat_set = json.loads((gold_from / f"{chat_set['name']}.json").read_text())
        if "gold" not in chat_set:
            continue
        report = json.loads(path.read_text())
        if "trace" in report:
            out.append((chat_set, report, path.parent))
    return out


def sentences_of(chat_set: dict, report: dict) -> list:
    """One row for each sentence of each read turn: the probabilities and the
    gold role of the sentence."""
    gold = chat_set["gold"]
    facts = {f["id"]: f for f in gold.get("facts", [])}
    stale = {s["id"]: s for s in gold.get("stale", [])}
    forbidden = gold.get("forbidden", [])
    rows = []
    by_entry = gold_lib._by_entry(report["trace"])
    for entry, steps in by_entry.items():
        tid = gold_lib.turn_id(entry)
        gate = next((s for s in steps if s["step"] == "gate"), None)
        if gate is None or tid is None:
            continue
        split = next((s for s in steps if s["step"] == "split"), None)
        sd = next((s for s in steps if s["step"] == "sentence_decisions"), None)
        if split is None:
            # A skipped turn: the trace has no sentences. Use the whole turn.
            text = next((s["text"] for s in steps if s["step"] == "turn"), "")
            split = {"sentences": [{"index": 0, "text": text, "denylist": False}]}
        decisions = {s["index"]: s for s in (sd or {}).get("sentences", [])}
        turn_items = [facts.get(i) or stale.get(i) for i in gold.get("turns", {}).get(tid, [])]
        wanted_items = [i for i in turn_items if i and (i["id"] in stale or i.get("required"))]
        optional = [i for i in turn_items if i and i["id"] in facts and not i.get("required")]
        for s in split["sentences"]:
            d = decisions.get(s["index"])
            role = "extra"
            if any(gold_lib.matches(f["match"], s["text"]) or gold_lib._sentence_match(f["match"], s["text"])
                   for f in forbidden):
                role = "forbidden"
            elif not turn_items:
                role = "wrong" if gold.get("turns", {}).get(tid) == [] else "extra"
            elif any(gold_lib._sentence_match(i["match"], s["text"]) for i in wanted_items):
                role = "wanted"
            elif any(gold_lib._sentence_match(i["match"], s["text"]) for i in optional):
                role = "optional"
            rows.append({"set": chat_set["name"], "turn": tid, "text": s["text"],
                         "denylist": s["denylist"], "gate": gate["probabilities"].get("yes", 0.0),
                         "probs": None if d is None else {
                             "fact": d["fact"].get("yes", 0.0), "safety": d["safety"]},
                         "traced": None if d is None else d["result"] == "keep", "role": role})
    return rows


def decide(row: dict, p: dict):
    """True, False, or None when the trace has no answers for the sentence."""
    if row["denylist"] or row["gate"] < p["gate_threshold"]:
        return False
    if row["probs"] is None:
        return None
    keep, _ = keep_rule(row["probs"], p["fact_threshold"], p["max_sensitive"],
                        p["max_instruction"], p["max_short_lived"])
    return keep


def score(rows: list, p: dict) -> dict:
    out = {"wanted": 0, "kept_wanted": 0, "wrong": 0, "forbidden": 0, "extra": 0, "unknown": 0}
    for r in rows:
        k = decide(r, p)
        if r["role"] == "wanted":
            out["wanted"] += 1
        if k is None:
            out["unknown"] += 1
            continue
        if not k:
            continue
        if r["role"] == "wanted":
            out["kept_wanted"] += 1
        elif r["role"] in ("wrong", "forbidden", "extra"):
            out[r["role"]] += 1
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("run", help="an L2 run folder of run_eval.py")
    parser.add_argument("--gold-from", default=None, help="chat sets with newer gold, by name")
    for flag, _ in PARAMS:
        parser.add_argument(f"--{flag.replace('_', '-')}", dest=flag, type=float, nargs="*")
    parser.add_argument("--top", type=int, default=0, help="show only the best N points")
    args = parser.parse_args()
    runs = load_runs(Path(args.run), Path(args.gold_from) if args.gold_from else None)
    if not runs:
        sys.exit("no report.json with a trace and gold in this run")
    config = runs[0][1].get("config", {})
    current = {name: config.get(name) for _, name in PARAMS}
    if None in current.values():
        sys.exit(f"report.json config has no {[k for k, v in current.items() if v is None]}: "
                 "the build is older than the harness")
    rows = [r for chat_set, report, _ in runs for r in sentences_of(chat_set, report)]

    # The grid point of the run must give the kept sentences of the trace.
    diffs = [r for r in rows if r["traced"] is not None and decide(r, current) != r["traced"]]
    check = ("PASS: the run's thresholds give the kept sentences of the trace "
             f"({sum(1 for r in rows if r['traced'] is not None)} sentences)") if not diffs else \
        f"FAIL: {len(diffs)} sentences differ from the trace, e.g. {diffs[0]['text']!r}"

    grid = {name: getattr(args, flag) or [current[name]] for flag, name in PARAMS}
    points = [dict(zip(grid, values)) for values in itertools.product(*grid.values())]
    results = [(p, score(rows, p)) for p in points]
    results.sort(key=lambda x: (x[1]["forbidden"], -x[1]["kept_wanted"], x[1]["wrong"]))
    if args.top:
        results = results[:args.top]
    print(f"# Threshold sweep of {args.run}\n")
    print(f"{len(runs)} set runs, {len(rows)} sentences. {check}\n")
    print("| Gate | Fact | Sensitive < | Instruction < | Short-lived < | Wanted kept | "
          "Wrong keeps | Forbidden keeps | Extra keeps | Unknown |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    for p, s in results:
        mark = " **(run)**" if p == current else ""
        print(f"| {p['gate_threshold']}{mark} | {p['fact_threshold']} | {p['max_sensitive']} | "
              f"{p['max_instruction']} | {p['max_short_lived']} | "
              f"{sc.fmt_rate(sc.rate(s['kept_wanted'], s['wanted']), False)} | {s['wrong']} | "
              f"{s['forbidden']}{' **LEAK RISK**' if s['forbidden'] else ''} | {s['extra']} | {s['unknown']} |")
    sys.exit(1 if diffs else 0)


if __name__ == "__main__":
    main()
