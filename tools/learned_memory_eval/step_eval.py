#!/usr/bin/env python3
"""Runs one step of learned memory alone on its labeled cases (plan level L1).

No browser. Each case is one request, built with lib/requests.py: the exact
bytes of the C++ code for the variant "current". Steps:
  gate, sentence, relation, relevance   decision model (/v1/systemone)
  rewrite, relation-llm, merge          local LLM (/v1/chat/completions)

  ./step_eval.py gate                                   # current wording, dev
  ./step_eval.py sentence --variant current G --errors G
  ./step_eval.py relation --threshold 0.7 --margin 0.2
  ./step_eval.py relevance --thresholds 0.2 0.3 0.4
  ./step_eval.py rewrite --model gemma3:4b --repeats 3
  ./step_eval.py sentence --split holdout               # final candidates only

Output: <results root>/<time>-<step>-<tag>/ with manifest.json, results.jsonl
(one line per case, variant and repeat), metrics.json and report.md. The
answers are cached, so a second run of the same request is free.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import ollama_setup
from lib import clients, manifest
from lib import judge as judge_lib
from lib import requests as rq
from lib import scoring as sc
from lib.steps import STEPS

HERE = Path(__file__).resolve().parent
# Plan 8.3, measured with noise_check.py on 2026-10-08: the decision model is
# repeatable, the LLM is not.
DEFAULT_REPEATS = {"dm": 1, "llm": 3}


class Context:
    def __init__(self, dm, llm, model):
        self.dm, self.llm, self.model = dm, llm, model


def parse_args(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("step", choices=list(STEPS))
    g = p.add_argument_group("shared")
    g.add_argument("--split", choices=["dev", "holdout"], default="dev")
    g.add_argument("--variant", nargs="+", default=["current"],
                   help="variant ids of lib/requests.py; the first one is the base of the flips")
    g.add_argument("--model", default=None,
                   help=f"model under test (default {ollama_setup.DEFAULT_DECISION} for decision "
                        f"model steps, {ollama_setup.DEFAULT_LLM} for LLM steps)")
    g.add_argument("--repeats", type=int, default=None,
                   help="repeats of each request (default 1 for decision model steps, 3 for LLM steps)")
    g.add_argument("--errors", nargs="*", default=None, metavar="VARIANT",
                   help="list the wrong cases of these variants (no value: all)")
    g.add_argument("--tag", default="", help="short name of the run")
    g.add_argument("--parallel", type=int, default=4, help="requests at the same time")
    g.add_argument("--serial", action="store_true", help="one request at a time, for latency")
    g.add_argument("--no-cache", action="store_true", help="do not read or write the cache")
    g.add_argument("--refresh", action="store_true",
                   help="send every request again, and write the new answers to the cache")
    g.add_argument("--cases", default=None, help="a JSONL case file instead of the dataset")
    g.add_argument("--judge", choices=["fast", "final"], default=None,
                   help="rewrite and merge: also judge the outputs with a separate agent "
                        "(fast: Sonnet, final: Opus; plan 7.5). Verdicts are cached")
    g.add_argument("--judge-model", default=None, help="agent model of the judge (default: the role's model)")
    g.add_argument("--judge-thinking", default=None)
    g.add_argument("--results", default=None, help="results root")
    g.add_argument("--ollama", default=clients.DEFAULT_OLLAMA)
    g.add_argument("--keep-alive", default=ollama_setup.DEFAULT_KEEP_ALIVE)
    g.add_argument("--no-warmup", action="store_true")
    s = p.add_argument_group("step params (C++ defaults)")
    s.add_argument("--gate-threshold", type=float, default=0.2)
    s.add_argument("--gate-thresholds", type=float, nargs="*", default=[0.05, 0.1, 0.2, 0.3, 0.5, 0.7])
    s.add_argument("--fact-threshold", type=float, default=0.6)
    s.add_argument("--max-sensitive", type=float, default=0.3)
    s.add_argument("--max-instruction", type=float, default=0.25)
    s.add_argument("--max-short-lived", type=float, default=0.5)
    s.add_argument("--grid-fact", type=float, nargs="*", default=[0.5, 0.6, 0.7])
    s.add_argument("--grid-sensitive", type=float, nargs="*", default=[0.2, 0.25, 0.3])
    s.add_argument("--threshold", type=float, default=0.8, help="certain_threshold")
    s.add_argument("--margin", type=float, default=0.2, help="certain_margin")
    s.add_argument("--relevance-threshold", type=float, default=0.3)
    s.add_argument("--max-turn-memories", type=int, default=5)
    s.add_argument("--thresholds", type=float, nargs="*", default=[0.2, 0.3, 0.4, 0.5, 0.6],
                   help="relevance thresholds to sweep")
    return p.parse_args(argv)


def main(argv=None) -> None:
    args = parse_args(argv)
    if args.judge and args.step not in ("rewrite", "merge"):
        sys.exit("--judge works for rewrite and merge only")
    step = STEPS[args.step](args)
    model = args.model or (ollama_setup.DEFAULT_DECISION if step.kind == "dm" else ollama_setup.DEFAULT_LLM)
    repeats = args.repeats or DEFAULT_REPEATS[step.kind]
    try:
        variants = [rq.resolve(args.step, v) for v in args.variant]
    except KeyError as e:
        sys.exit(str(e.args[0]))
    if args.split == "holdout":
        print("Holdout run: use it only for a final candidate, and read only the totals "
              "(plan, data rule 1).", file=sys.stderr)
    try:
        cases = step.load(args.split)
    except FileNotFoundError as e:
        sys.exit(str(e))
    by_id = {c["id"]: c for c in cases}
    if not args.no_warmup:
        try:
            ollama_setup.load_models(args.ollama, model if step.kind == "dm" else None,
                                     model if step.kind == "llm" else None, args.keep_alive,
                                     log=lambda m: print(m, file=sys.stderr, flush=True))
        except ollama_setup.OllamaSetupError as e:
            sys.exit(str(e))

    root = clients.results_root(args.results)
    dm, llm = clients.make_clients(args.ollama, root, use_cache=not args.no_cache,
                                   refresh=args.refresh)
    ctx = Context(dm, llm, model)
    tag = "-".join(x for x in (args.step, args.split, args.tag) if x)
    run_id, run_dir = manifest.new_run_dir(root, tag)
    manifest.write_manifest(
        run_dir, run_id=run_id, tag=args.tag, ollama=args.ollama,
        models={"decision" if step.kind == "dm" else "llm": model},
        params={"split": args.split, **step.params()},
        variants={args.step: {v: rq.VARIANTS[args.step][v].get("desc", "") for v in variants}},
        datasets=[str(p) for p in step.dataset_files(args.split)], repeats=repeats,
        cache=not args.no_cache, extra={"level": "L1", "step": args.step,
                                        "refresh": args.refresh, "serial": args.serial})

    jobs = [(v, r, c) for v in variants for r in range(repeats) for c in cases]
    print(f"{args.step}: {len(cases)} cases x {len(variants)} variants x {repeats} repeats "
          f"= {len(jobs)} requests ({model})", file=sys.stderr, flush=True)
    started = time.time()

    def work(job):
        v, r, c = job
        rec = step.run(c, v, r, ctx)
        return {"step": args.step, "variant": v, "repeat": r, "id": c["id"], **rec}

    with ThreadPoolExecutor(1 if args.serial else max(1, args.parallel)) as pool:
        records = list(pool.map(work, jobs))
    seconds = time.time() - started
    with open(run_dir / "results.jsonl", "w", encoding="utf-8") as f:
        for rec in records:
            f.write(json.dumps(rec, ensure_ascii=False) + "\n")

    metrics = {"run_id": run_id, "step": args.step, "split": args.split, "model": model,
               "repeats": repeats, "seconds": round(seconds, 1),
               "requests": {"sent": dm.calls + llm.calls, "cached": dm.cache_hits + llm.cache_hits},
               "variants": {}}
    by_variant = {v: [r for r in records if r["variant"] == v] for v in variants}
    for v in variants:
        metrics["variants"][v] = step.score(by_variant[v], by_id)
    judged = {}
    if args.judge:
        spec = judge_lib.judge_spec(args.judge, args.judge_model, args.judge_thinking)
        inputs = {v: judge_lib.step_inputs(args.step, v, by_variant[v], by_id) for v in variants}
        all_inputs = {n: i for v in variants for n, i in inputs[v].items()}
        verdicts = judge_lib.judge(args.step, all_inputs, spec, root, parallel=args.parallel,
                                   log=lambda m: print(m, file=sys.stderr, flush=True))
        for v in variants:
            vs = [verdicts[n] for n in inputs[v] if n in verdicts]
            judged[v] = judge_lib.flag_metrics(vs)
            metrics["variants"][v].update(judged[v])
        metrics["judge"] = judge_lib.meta(args.step, spec)
        with open(run_dir / "judge.jsonl", "w", encoding="utf-8") as f:
            for name, item in all_inputs.items():
                f.write(json.dumps({"name": name, "input": item, "verdict": verdicts.get(name)},
                                   ensure_ascii=False) + "\n")
    if len(variants) > 1:
        metrics["flips"] = {v: sc.flips(by_variant[variants[0]], by_variant[v]) for v in variants[1:]}
    (run_dir / "metrics.json").write_text(json.dumps(metrics, indent=2, ensure_ascii=False))

    md = [f"# Step eval: {args.step} ({args.split})", "",
          f"Run `{run_id}`. Model `{model}`. {len(cases)} cases, {repeats} repeat(s). "
          f"{metrics['requests']['sent']} requests sent, {metrics['requests']['cached']} from the "
          f"cache, {seconds:.0f} s.", ""]
    if len(variants) > 1:
        md += [f"| Variant | {step.primary} | Fixed vs {variants[0]} | Broken vs {variants[0]} |",
               "|---|---|---|---|"]
        for v in variants:
            fl = metrics.get("flips", {}).get(v)
            md.append(f"| {v} | {sc.fmt_rate(metrics['variants'][v][step.primary])} | "
                      f"{len(fl['fixed']) if fl else ''} | {len(fl['broken']) if fl else ''} |")
        md.append("")
    for v in variants:
        md += [f"## Variant {v}: {rq.VARIANTS[args.step][v].get('desc', '')}", ""]
        md += step.render(v, metrics["variants"][v]) + [""]
        if v in judged:
            j = metrics["judge"]
            md += [f"Judge: {j['role']} ({j['model']}, {j['thinking']}, rubric v{j['rubric_version']}).", ""]
            md += judge_lib.fmt_flag_metrics(judged[v]) + [""]
        fl = metrics.get("flips", {}).get(v)
        if fl and (fl["fixed"] or fl["broken"]):
            md += [f"Fixed vs {variants[0]}: {', '.join(fl['fixed']) or 'none'}.",
                   f"Broken vs {variants[0]}: {', '.join(fl['broken']) or 'none'}.", ""]
    if args.errors is not None and args.split != "holdout":
        for v in (args.errors or variants):
            vid = rq.resolve(args.step, v)
            if vid in by_variant:
                lines = step.errors(by_variant[vid], by_id)
                md += [f"## Wrong cases of {vid} ({len(lines)})", ""] + (lines or ["(none)"]) + [""]
    elif args.errors is not None:
        md += ["Errors are not listed for the holdout (plan, data rule 2).", ""]
    (run_dir / "report.md").write_text("\n".join(md) + "\n")
    print("\n".join(md))
    print(f"Run folder: {run_dir}", file=sys.stderr)


if __name__ == "__main__":
    main()
