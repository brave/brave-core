#!/usr/bin/env python3
"""Judge calibration (plan P2.7).

Sorts the same outputs of the rewrite and merge steps with the fast judge
(Sonnet, low) and the final judge (Opus, high), and counts how often they
agree. It also makes a file of judge checks for the reviewer (judge_check.jsonl):
`./review.py --quick` asks them.

  ./judge_calibrate.py                                  # the newest baseline-v2 runs, 100 outputs
  ./judge_calibrate.py --rewrite-run DIR --merge-run DIR --n 100 --checks 10

Rule of the plan: the fast judge is allowed when it agrees with the final judge
on at least 90% of the outputs ("faithful" or not). Below 90%, use the final
judge only. The final judge is a hint until a person agrees with at least 9 of 10
checked verdicts.

Output: <results root>/judge_calibration/<time>/calibration.json, calibration.md
and judge_check.jsonl.
"""
from __future__ import annotations

import argparse
import datetime
import json
import random
import sys
from pathlib import Path

from lib import clients
from lib import judge as J
from lib import scoring as sc
from lib.steps import read_jsonl


def latest(root: Path, step: str, tag: str) -> Path:
    runs = sorted(p for p in root.iterdir() if p.is_dir() and p.name.endswith(f"-{step}-dev-{tag}"))
    if not runs:
        sys.exit(f"no run *-{step}-dev-{tag} in {root}")
    return runs[-1]


def load_inputs(step: str, run: Path) -> dict:
    records = [json.loads(l) for l in (run / "results.jsonl").read_text().splitlines() if l.strip()]
    cases = {c["id"]: c for c in read_jsonl(Path(__file__).resolve().parent / "datasets" / "steps" / step / "dev.jsonl")}
    return J.step_inputs(step, "current", [r for r in records if r["variant"] == "current"], cases)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--rewrite-run")
    parser.add_argument("--merge-run")
    parser.add_argument("--tag", default="baseline-v2", help="find the newest runs with this tag")
    parser.add_argument("--n", type=int, default=100, help="outputs to judge (60%% rewrite, 40%% merge)")
    parser.add_argument("--checks", type=int, default=10, help="verdicts of the final judge for the reviewer")
    parser.add_argument("--seed", default="calibration-1")
    parser.add_argument("--results", default=None)
    parser.add_argument("--parallel", type=int, default=4)
    parser.add_argument("--agent-cmd", default="pi")
    args = parser.parse_args()
    root = clients.results_root(args.results)
    runs = {"rewrite": Path(args.rewrite_run) if args.rewrite_run else latest(root, "rewrite", args.tag),
            "merge": Path(args.merge_run) if args.merge_run else latest(root, "merge", args.tag)}
    rng = random.Random(args.seed)
    counts = {"rewrite": round(args.n * 0.6), "merge": args.n - round(args.n * 0.6)}
    fast, final = J.judge_spec("fast"), J.judge_spec("final")
    rows = []
    for kind in ("rewrite", "merge"):
        inputs = load_inputs(kind, runs[kind])
        names = sorted(inputs)
        names = rng.sample(names, min(counts[kind], len(names)))
        picked = {n: inputs[n] for n in names}
        print(f"{kind}: {len(inputs)} outputs in {runs[kind].name}, {len(picked)} sampled", flush=True)
        vf = J.judge(kind, picked, fast, root, args.parallel, args.agent_cmd)
        vo = J.judge(kind, picked, final, root, args.parallel, args.agent_cmd)
        for n in picked:
            if n in vf and n in vo:
                rows.append({"kind": kind, "name": n, "input": picked[n], "fast": vf[n], "final": vo[n]})
    both = len(rows)
    agree = sum(1 for r in rows if J.faithful(r["fast"]) == J.faithful(r["final"]))
    per_flag = {f: sc.rate(sum(1 for r in rows if bool(r["fast"].get(f)) == bool(r["final"].get(f))), both)
                for f in J.FLAGS}
    faithful_final = sum(1 for r in rows if J.faithful(r["final"]))
    # Cohen's kappa on "faithful"
    pf = sum(1 for r in rows if J.faithful(r["fast"])) / both if both else 0
    po = faithful_final / both if both else 0
    pe = pf * po + (1 - pf) * (1 - po)
    kappa = ((agree / both) - pe) / (1 - pe) if both and pe < 1 else None
    result = {"time": datetime.datetime.now().isoformat(timespec="seconds"), "outputs": both, "runs": {k: v.name for k, v in runs.items()},
              "fast": J.meta("rewrite", fast), "final": J.meta("rewrite", final),
              "agree_faithful": sc.rate(agree, both), "agree_by_flag": per_flag, "kappa_faithful": kappa,
              "final_faithful": sc.rate(faithful_final, both),
              "fast_allowed": both > 0 and agree / both >= 0.9}
    out = root / "judge_calibration" / datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    out.mkdir(parents=True)
    (out / "calibration.json").write_text(json.dumps({**result, "rows": rows}, indent=2, ensure_ascii=False))

    # checks for the reviewer: half faithful, half not, as far as the final judge has them
    good = [r for r in rows if J.faithful(r["final"])]
    bad = [r for r in rows if not J.faithful(r["final"])]
    rng.shuffle(good), rng.shuffle(bad)
    half = args.checks // 2
    chosen = bad[:half] + good[:args.checks - min(half, len(bad))]
    chosen = chosen[:args.checks]
    with open(out / "judge_check.jsonl", "w", encoding="utf-8") as f:
        for n, r in enumerate(chosen, 1):
            f.write(json.dumps({"kind": "judge", "id": f"judge-check-{r['kind']}-{n:02d}", "step": r["kind"],
                                "input": r["input"], "verdict": r["final"]}, ensure_ascii=False) + "\n")
    md = ["# Judge calibration", "",
          f"{both} outputs of {', '.join(f'`{v}`' for v in result['runs'].values())}. Fast judge: "
          f"{fast['model']} ({fast['thinking']}). Final judge: {final['model']} ({final['thinking']}).", "",
          f"- Agreement on faithful or not: {sc.fmt_rate(result['agree_faithful'])}, kappa "
          f"{'n/a' if kappa is None else f'{kappa:.2f}'}.",
          *[f"- Agreement on {f}: {sc.fmt_rate(r, False)}" for f, r in per_flag.items()],
          f"- The final judge finds {sc.fmt_rate(result['final_faithful'], False)} faithful.",
          f"- **The fast judge is {'allowed' if result['fast_allowed'] else 'NOT allowed'}** (rule: at least 90% agreement).",
          f"- {len(chosen)} checks for the reviewer are in `judge_check.jsonl` (asked by `./review.py --quick`). "
          "Until she agrees with at least 9 of 10, the judge is a hint only.", "",
          "## Disagreements", ""]
    for r in rows:
        if J.faithful(r["fast"]) != J.faithful(r["final"]):
            md.append(f"- `{r['kind']}` {json.dumps(r['input'], ensure_ascii=False)}")
            md.append(f"  - fast: {json.dumps(r['fast'], ensure_ascii=False)}")
            md.append(f"  - final: {json.dumps(r['final'], ensure_ascii=False)}")
    (out / "calibration.md").write_text("\n".join(md) + "\n")
    print("\n".join(md[:12]))
    print(f"\nFolder: {out}")


if __name__ == "__main__":
    main()
