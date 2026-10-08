#!/usr/bin/env python3
"""Tests the chat time relevance question of the decision model alone.

For each message in datasets/steps/relevance/dev.jsonl, the script asks the decision model
(Ollama /v1/systemone) one noul question for each memory, in one request, the
same as OllamaDecisionClient::AskRelevance(). It reports, for several
thresholds, the memories that must be relevant and were found (recall), the
memories that must not be relevant and were found (forbidden), and the latency.
Use it before you change the relevance question or the threshold
(`relevance_threshold` of the feature).

  ./relevance_eval.py                 # all wordings
  ./relevance_eval.py A B --errors A  # two wordings, list the misses of A
"""
from __future__ import annotations

import argparse
import json
import sys
import time
import urllib.request
from pathlib import Path

import ollama_setup
from lib import requests as rq

HERE = Path(__file__).resolve().parent

# The wordings are in lib/requests.py. The wording in the C++ client is A.
WORDINGS = {k: (lambda make: (lambda t: make(t).as_dict()))(make)
            for k, make in rq.RELEVANCE.items()}


def load_cases(path: str | None) -> tuple:
    """Returns (memories, cases). Each case has "must" and "forbidden" as
    memory indexes. |path| is an old case file, or None for
    datasets/steps/relevance/dev.jsonl and memories.jsonl."""
    if path:
        data = json.loads(Path(path).read_text())
        memories = data["memories"]
        find = lambda needles: [i for i, t in enumerate(memories) if any(n in t for n in needles)]
        return memories, [{"message": c["message"], "must": find(c["must"]),
                           "forbidden": find(c["forbidden"])} for c in data["cases"]]
    folder = HERE / "datasets" / "steps" / "relevance"
    read = lambda p: [json.loads(l) for l in p.read_text().splitlines() if l.strip()]
    by_id = {m["id"]: m["text"] for m in read(folder / "memories.jsonl")}
    cases = []
    memories = None
    for c in read(folder / "dev.jsonl"):
        texts = [by_id[i] for i in c["memories"]]
        if memories is None:
            memories = texts
        if texts != memories:
            sys.exit("relevance_eval.py needs the same memories in each case. "
                     "Use step_eval.py relevance.")
        index = {i: n for n, i in enumerate(c["memories"])}
        cases.append({"message": c["message"], "must": [index[i] for i in c["must"]],
                      "forbidden": [index[i] for i in c["forbidden"]]})
    return memories, cases


def ask(url: str, model: str, message: str, memories: list, wording) -> tuple:
    questions = {f"m{i}": wording(t) for i, t in enumerate(memories)}
    body = json.dumps({"model": model, "state": {"user_message": message},
                       "questions": questions}).encode()
    request = urllib.request.Request(url, body, {"Content-Type": "application/json"})
    start = time.time()
    with urllib.request.urlopen(request, timeout=300) as response:
        answers = json.load(response)["answers"]
    return [answers[f"m{i}"]["noul"] for i in range(len(memories))], time.time() - start


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("wordings", nargs="*", default=list(WORDINGS))
    parser.add_argument("--cases", default=None,
                        help="an old case file instead of datasets/steps/relevance")
    parser.add_argument("--ollama", default=ollama_setup.DEFAULT_OLLAMA)
    parser.add_argument("--model", default=ollama_setup.DEFAULT_DECISION)
    parser.add_argument("--keep-alive", default=ollama_setup.DEFAULT_KEEP_ALIVE)
    parser.add_argument("--no-warmup", action="store_true",
                        help="do not load the decision model before the test")
    parser.add_argument("--thresholds", type=float, nargs="*", default=[0.3, 0.4, 0.5, 0.6])
    parser.add_argument("--errors", nargs="*", default=[], help="wordings to list the misses of")
    args = parser.parse_args()
    try:
        if not args.no_warmup:
            ollama_setup.load_models(args.ollama, args.model, None, args.keep_alive)
    except ollama_setup.OllamaSetupError as error:
        sys.exit(str(error))

    memories, cases = load_cases(args.cases)
    for name in args.wordings:
        scores, latencies = [], []
        for case in cases:
            probabilities, seconds = ask(f"{args.ollama}/v1/systemone", args.model, case["message"], memories, WORDINGS[name])
            scores.append(probabilities)
            latencies.append(seconds)
        print(f"\n== {name}: {len(memories)} memories, mean latency {sum(latencies) / len(latencies):.2f} s")
        for threshold in args.thresholds:
            found = missed = bad = 0
            for case, probabilities in zip(cases, scores):
                for i in case["must"]:
                    found += probabilities[i] >= threshold
                    missed += probabilities[i] < threshold
                bad += sum(probabilities[i] >= threshold for i in case["forbidden"])
            kept = sum(p >= threshold for ps in scores for p in ps) / len(cases)
            print(f"  threshold {threshold}: must found {found}/{found + missed}, "
                  f"forbidden found {bad}, mean kept {kept:.1f}")
        if name in args.errors:
            for case, probabilities in zip(cases, scores):
                for i in case["must"]:
                    print(f"    {probabilities[i]:.2f}  {case['message'][:40]!r} -> {memories[i]}")


if __name__ == "__main__":
    main()
