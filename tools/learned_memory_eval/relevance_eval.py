#!/usr/bin/env python3
"""Tests the chat time relevance question of the decision model alone.

For each message in relevance_cases.json, the script asks the decision model
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

HERE = Path(__file__).resolve().parent

TRUE_A = "The memory would change or improve the answer to the message"
FALSE_A = "The memory has nothing to do with the message"
# The wording in the C++ client is A.
WORDINGS = {
    "A": lambda t: {
        "type": "noul",
        "instructions": f"Memory: {t}\nIs this memory relevant to the user's message?",
        "criteria": {"true": TRUE_A, "false": FALSE_A}},
    "B": lambda t: {
        "type": "noul",
        "instructions": f"Memory: {t}\nIs this memory relevant to the user's message?"},
    "C": lambda t: {
        "type": "noul",
        "instructions": f"Fact about the user: {t}\nWould knowing this fact help give a better answer to the user's message?",
        "criteria": {"true": "The fact makes the answer more useful or more personal",
                     "false": "The fact is not useful for this message"}},
}


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
    parser.add_argument("--cases", default=str(HERE / "relevance_cases.json"))
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

    data = json.loads(Path(args.cases).read_text())
    memories, cases = data["memories"], data["cases"]
    find = lambda needles: [i for i, t in enumerate(memories) if any(n in t for n in needles)]
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
                for i in find(case["must"]):
                    found += probabilities[i] >= threshold
                    missed += probabilities[i] < threshold
                bad += sum(probabilities[i] >= threshold for i in find(case["forbidden"]))
            kept = sum(p >= threshold for ps in scores for p in ps) / len(cases)
            print(f"  threshold {threshold}: must found {found}/{found + missed}, "
                  f"forbidden found {bad}, mean kept {kept:.1f}")
        if name in args.errors:
            for case, probabilities in zip(cases, scores):
                for i in find(case["must"]):
                    print(f"    {probabilities[i]:.2f}  {case['message'][:40]!r} -> {memories[i]}")


if __name__ == "__main__":
    main()
