#!/usr/bin/env python3
"""Measures the noise of the models: the same request, sent many times.

Sends N decision model requests (sentence questions, "current" wording) and N
rewrite requests (local LLM, "current" prompt), each R times, with no cache.
The report says if the decision model gives the same probabilities each time,
and how often the LLM gives the same answer. Plan section 8.3 sets the repeats
of the evals from this result.

  ./noise_check.py                           # 20 requests x 5, both models
  ./noise_check.py --count 10 --repeats 3
  ./noise_check.py --parallel 4              # as the browser sends sentences

Output: <results root>/<time>-noise/ (manifest.json, results.jsonl,
metrics.json, report.md).
"""
from __future__ import annotations

import argparse
import json
import statistics
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import ollama_setup
from lib import clients, manifest
from lib import requests as rq

HERE = Path(__file__).resolve().parent


def read_jsonl(path: Path) -> list:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def flatten(probs: dict) -> dict:
    out = {}
    for k, v in probs.items():
        if isinstance(v, dict):
            out.update({f"{k}.{kk}": vv for kk, vv in v.items()})
        else:
            out[k] = v
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--ollama", default=clients.DEFAULT_OLLAMA)
    parser.add_argument("--decision", default=ollama_setup.DEFAULT_DECISION)
    parser.add_argument("--llm", default=ollama_setup.DEFAULT_LLM)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--parallel", type=int, default=1)
    parser.add_argument("--skip-llm", action="store_true")
    parser.add_argument("--results", default=None)
    parser.add_argument("--tag", default="noise")
    args = parser.parse_args()
    try:
        ollama_setup.load_models(args.ollama, args.decision, None if args.skip_llm else args.llm)
    except ollama_setup.OllamaSetupError as e:
        sys.exit(str(e))

    root = clients.results_root(args.results)
    dm, llm = clients.make_clients(args.ollama, root, use_cache=False)
    sentences = read_jsonl(HERE / "datasets/steps/sentence/dev.jsonl")[:args.count]
    rewrites = read_jsonl(HERE / "datasets/steps/rewrite/dev.jsonl")
    keep = [c for c in read_jsonl(HERE / "datasets/steps/sentence/dev.jsonl") if c["gold"]["keep"]]
    rewrite_inputs = [(c["id"], c["turn_date"], c["sentences"]) for c in rewrites]
    rewrite_inputs += [(c["id"], "2026-09-20", [c["text"]]) for c in keep]
    rewrite_inputs = rewrite_inputs[:args.count]

    run_id, run_dir = manifest.new_run_dir(root, args.tag)
    datasets = [HERE / "datasets/steps/sentence/dev.jsonl", HERE / "datasets/steps/rewrite/dev.jsonl"]
    manifest.write_manifest(
        run_dir, run_id=run_id, tag=args.tag, ollama=args.ollama,
        models={"decision": args.decision, "llm": None if args.skip_llm else args.llm},
        params={"count": args.count, "parallel": args.parallel},
        variants={"sentence": rq.resolve("sentence", "current"), "rewrite": "current"},
        datasets=[str(d) for d in datasets], repeats=args.repeats, cache=False,
        extra={"level": "L1", "step": "noise"})
    results = open(run_dir / "results.jsonl", "w", encoding="utf-8")

    def dm_job(item):
        case, r = item
        res = dm.ask(args.decision, rq.sentence_body(args.decision, case["text"]), r)
        parsed = rq.parse_sentence(res["answers"]) if res["answers"] else None
        return case["id"], r, parsed, res

    print(f"Decision model: {len(sentences)} requests x {args.repeats}", flush=True)
    jobs = [(c, r) for r in range(args.repeats) for c in sentences]
    with ThreadPoolExecutor(args.parallel) as pool:
        dm_out = list(pool.map(dm_job, jobs))
    by_case: dict = {}
    for cid, r, parsed, res in dm_out:
        results.write(json.dumps({"kind": "sentence", "id": cid, "repeat": r, "probs": parsed,
                                  "latency_ms": res["latency_ms"], "error": res["error"]}) + "\n")
        if parsed:
            by_case.setdefault(cid, []).append(flatten(parsed))
    spreads, identical = [], 0
    for cid, runs in by_case.items():
        keys = runs[0].keys()
        spread = max(max(x[k] for x in runs) - min(x[k] for x in runs) for k in keys)
        spreads.append(spread)
        identical += spread == 0
    dm_latency = [res["latency_ms"] for *_, res in dm_out if res["latency_ms"] is not None]
    dm_errors = sum(1 for *_, res in dm_out if res["error"])
    metrics = {"decision": {
        "model": args.decision, "requests": len(sentences), "repeats": args.repeats,
        "errors": dm_errors, "identical_requests": identical,
        "max_spread": max(spreads) if spreads else None,
        "mean_spread": statistics.mean(spreads) if spreads else None,
        "median_latency_ms": statistics.median(dm_latency) if dm_latency else None}}

    if not args.skip_llm:
        def llm_job(item):
            (cid, date, sents), r = item
            res = llm.complete(args.llm, rq.chat_body(args.llm, *rq.rewrite_messages(sents, date)), r)
            facts = rq.parse_rewrite_answer(res["content"] or "", len(sents))
            return cid, r, res, facts

        print(f"LLM rewrite: {len(rewrite_inputs)} requests x {args.repeats}", flush=True)
        jobs = [(x, r) for r in range(args.repeats) for x in rewrite_inputs]
        with ThreadPoolExecutor(args.parallel) as pool:
            llm_out = list(pool.map(llm_job, jobs))
        answers: dict = {}
        parsed: dict = {}
        for cid, r, res, facts in llm_out:
            results.write(json.dumps({"kind": "rewrite", "id": cid, "repeat": r,
                                      "content": res["content"], "facts": facts,
                                      "latency_ms": res["latency_ms"], "error": res["error"]},
                                     ensure_ascii=False) + "\n")
            answers.setdefault(cid, []).append(res["content"])
            parsed.setdefault(cid, []).append(json.dumps(facts))
        distinct = [len(set(v)) for v in answers.values()]
        distinct_facts = [len(set(v)) for v in parsed.values()]
        lat = [res["latency_ms"] for _, _, res, _ in llm_out if res["latency_ms"] is not None]
        metrics["rewrite"] = {
            "model": args.llm, "requests": len(rewrite_inputs), "repeats": args.repeats,
            "errors": sum(1 for _, _, res, _ in llm_out if res["error"]),
            "identical_answers": sum(d == 1 for d in distinct),
            "identical_facts": sum(d == 1 for d in distinct_facts),
            "mean_distinct_answers": statistics.mean(distinct),
            "max_distinct_answers": max(distinct),
            "median_latency_ms": statistics.median(lat) if lat else None,
            "changed": {cid: sorted(set(v)) for cid, v in parsed.items() if len(set(v)) > 1}}
    results.close()
    (run_dir / "metrics.json").write_text(json.dumps(metrics, indent=2, ensure_ascii=False))

    d = metrics["decision"]
    md = [f"# Noise check {run_id}", "",
          f"Parallel requests: {args.parallel}. No cache.", "",
          "| Model | Requests x repeats | Errors | Same answer every time | Max spread | Mean spread | Median latency |",
          "|---|---|---|---|---|---|---|",
          f"| {d['model']} (sentence) | {d['requests']} x {d['repeats']} | {d['errors']} | "
          f"{d['identical_requests']}/{len(by_case)} | {d['max_spread']:.2g} | {d['mean_spread']:.2g} | "
          f"{d['median_latency_ms']} ms |"]
    if "rewrite" in metrics:
        w = metrics["rewrite"]
        md.append(f"| {w['model']} (rewrite) | {w['requests']} x {w['repeats']} | {w['errors']} | "
                  f"text {w['identical_answers']}/{w['requests']}, facts {w['identical_facts']}/"
                  f"{w['requests']} | {w['max_distinct_answers']} answers | "
                  f"{w['mean_distinct_answers']:.2f} answers | {w['median_latency_ms']} ms |")
        md += ["", "Spread for the decision model: the largest difference of one probability "
               "over the repeats. For the LLM: the number of different answers for one request.", ""]
        if w["changed"]:
            md += ["## Rewrites that changed", ""]
            for cid, variants in w["changed"].items():
                md.append(f"- `{cid}`")
                md += [f"  - {v}" for v in variants]
    (run_dir / "report.md").write_text("\n".join(md) + "\n")
    print("\n".join(md))
    print(f"\nRun folder: {run_dir}")


if __name__ == "__main__":
    main()
