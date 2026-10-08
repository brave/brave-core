#!/usr/bin/env python3
"""Checks that lib/requests.py builds the same requests as the browser.

Reads a recording of eval_proxy.py (run_eval.py --proxy writes
<run>/proxy.jsonl). For each recorded request, it takes the inputs from the
request (the turn, the sentence, the two memories, ...), builds the request
again with the "current" variant of lib/requests.py, and compares.

- Decision model requests (gate, sentence, relation, relevance): the exact
  bytes must be the same.
- LLM requests (rewrite, relation-llm, merge): the message list must be the
  same. The script also reports if the whole body is the same.

  ./parity_check.py <run>/proxy.jsonl
  ./parity_check.py <run>/proxy.jsonl --require gate sentence rewrite
  ./parity_check.py --traces <run>/*/report.json

With --traces, it checks the Python ports of ParseRewriteAnswer() and
IsFaithfulRewrite() on the trace of the browser: the facts of each
"rewrite_parsed" step, and the "no_new_details" flag of each
"rewrite_parsed" and "merge_parsed" step.

Exit status 1 when a request or a result differs, or a --require kind is
missing.
"""
from __future__ import annotations

import argparse
import datetime
import json
import re
import sys
from pathlib import Path

from lib import requests as rq

DM_KINDS = ["gate", "sentence", "relation", "relevance"]
LLM_KINDS = ["rewrite", "relation-llm", "merge"]
RELEVANCE_PREFIX = "Memory: "
RELEVANCE_SUFFIX = "\nIs this memory relevant to the user's message?"


def _messages(body: dict) -> tuple:
    texts = []
    for m in body.get("messages", []):
        content = m.get("content")
        if isinstance(content, list):
            content = "".join(b.get("text", "") for b in content)
        texts.append(content)
    return tuple(texts)


def _split_pair(user: str) -> tuple:
    head, sep, new = user.partition("\nNew memory: ")
    if not sep or not head.startswith("Old memory: "):
        raise ValueError("not an 'Old memory / New memory' message")
    return head[len("Old memory: "):], new


def _split_rewrite(user: str) -> tuple:
    first, _, rest = user.partition("\n")
    if not first.startswith("Turn date: "):
        raise ValueError("no turn date")
    date = first[len("Turn date: "):]
    text = "\n" + rest
    sentences, i = [], 0
    while text:
        marker = f"\n{i}: "
        if not text.startswith(marker):
            raise ValueError(f"no sentence {i}")
        text = text[len(marker):]
        nxt = text.find(f"\n{i + 1}: ")
        sentences.append(text if nxt < 0 else text[:nxt])
        text = "" if nxt < 0 else text[nxt:]
        i += 1
    return date, sentences


def rebuild(kind: str, body: dict) -> str:
    """The request of lib/requests.py "current" for the inputs of |body|."""
    model, state = body.get("model", ""), body.get("state")
    if kind == "gate":
        return rq.gate_body(model, state)
    if kind == "sentence":
        return rq.sentence_body(model, state)
    if kind == "relation":
        return rq.relation_body(model, state["new_memory"], state["old_memory"])
    if kind == "relevance":
        memories = []
        for name, q in body["questions"].items():
            text = q["instructions"]
            if not (text.startswith(RELEVANCE_PREFIX) and text.endswith(RELEVANCE_SUFFIX)):
                raise ValueError(f"question {name} has another wording")
            memories.append(text[len(RELEVANCE_PREFIX):-len(RELEVANCE_SUFFIX)])
        return rq.relevance_body(model, state["user_message"],
                                 state.get("previous_message", ""), memories)
    user = _messages(body)[1]
    if kind == "rewrite":
        date, sentences = _split_rewrite(user)
        return rq.chat_body(model, *rq.rewrite_messages(sentences, date))
    old, new = _split_pair(user)
    if kind == "relation-llm":
        return rq.chat_body(model, *rq.relation_llm_messages(new, old))
    if kind == "merge":
        return rq.chat_body(model, *rq.merge_messages(old, new))
    raise ValueError(f"no builder for {kind}")


def first_difference(a: str, b: str) -> str:
    i = next((k for k, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
    lo = max(0, i - 60)
    return (f"at character {i}:\n    recorded: ...{a[lo:i + 60]!r}\n"
            f"    python:   ...{b[lo:i + 60]!r}")


def check(records: list) -> dict:
    result = {k: {"recorded": 0, "same": 0, "body_same": 0, "diffs": []}
              for k in DM_KINDS + LLM_KINDS}
    for n, rec in enumerate(records):
        kind = rec.get("kind")
        if kind not in result:
            continue
        r = result[kind]
        r["recorded"] += 1
        recorded = rec["body"]
        body = json.loads(recorded)
        try:
            built = rebuild(kind, body)
        except (ValueError, KeyError, IndexError, TypeError) as e:
            r["diffs"].append(f"line {n + 1}: cannot read the inputs: {e}")
            continue
        if kind in DM_KINDS:
            same = built == recorded
            r["body_same"] += same
        else:
            r["body_same"] += built == recorded
            same = _messages(json.loads(built)) == _messages(body)
        r["same"] += same
        if not same:
            r["diffs"].append(f"line {n + 1} ({rec.get('set')}): "
                              + first_difference(recorded, built))
    return result


def _local_ymd(iso: str) -> tuple:
    """base::Time::LocalExplode() of a TimeFormatAsIso8601() text."""
    t = datetime.datetime.fromisoformat(iso.replace("Z", "+00:00")).astimezone()
    return (t.year, t.month, t.day)


def check_traces(paths: list) -> dict:
    """Compares the ports with the C++ results in report.json traces."""
    out = {"parse": [0, 0], "rewrite_flag": [0, 0], "merge_flag": [0, 0], "diffs": []}
    for path in paths:
        trace = json.loads(Path(path).read_text()).get("trace", [])
        turn_date = candidates = rewrite_answer = old_text = fact_text = None
        memories: dict = {}  # text -> (created, updated), local dates
        for step in trace:
            name = step["step"]
            if name == "turn":
                turn_date, candidates, rewrite_answer = _local_ymd(step["date"]), [], None
            elif name == "sentence_decisions":
                candidates = [s["text"] for s in step["sentences"] if s["result"] == "keep"]
            elif name == "llm_call" and step["purpose"] == "rewrite":
                rewrite_answer = step["answer"]
                m = re.match(r"Turn date: (\d{4})-(\d{2})-(\d{2})", step["request"])
                if m and tuple(int(x) for x in m.groups()) != turn_date:
                    out["diffs"].append(f"{path}: turn date {turn_date} is not the request date")
            elif name == "rewrite_parsed":
                cpp = [(f["text"], f["sources"]) for f in step["facts"]]
                if step["used_llm"]:
                    out["parse"][1] += 1
                    py = rq.parse_rewrite_answer(rewrite_answer or "", len(candidates))
                    if (py is not None) == step["parsed"] and (py or []) == cpp:
                        out["parse"][0] += 1
                    else:
                        out["diffs"].append(f"{path}: parse {py} != C++ {cpp}")
                for f in step["facts"]:
                    out["rewrite_flag"][1] += 1
                    py = rq.is_faithful_rewrite(f["text"], [candidates[i] for i in f["sources"]],
                                                [turn_date])
                    if py == f["no_new_details"]:
                        out["rewrite_flag"][0] += 1
                    else:
                        out["diffs"].append(f"{path}: rewrite flag of {f['text']!r}: python {py}, "
                                            f"C++ {f['no_new_details']}")
            elif name == "fact":
                fact_text = step["text"]
            elif name == "relation_applied":
                old_text = step["old"]
            elif name == "merge_parsed":
                out["merge_flag"][1] += 1
                created, updated = memories.get(old_text, (turn_date, turn_date))
                text = step["text"]
                py = bool(text) and rq.is_faithful_rewrite(
                    text, [old_text, fact_text], [turn_date, created, updated])
                if py == step["no_new_details"]:
                    out["merge_flag"][0] += 1
                else:
                    out["diffs"].append(f"{path}: merge flag of {text!r}: python {py}, "
                                        f"C++ {step['no_new_details']}")
            elif name == "store":
                text, previous = step["text"], step.get("previous")
                if step["action"] == "add":
                    memories[text] = (turn_date, turn_date)
                elif text in memories:  # same: only the date changes
                    created, updated = memories[text]
                    memories[text] = (created, max(updated, turn_date))
                elif previous in memories:  # replace or merge
                    memories[text] = (memories.pop(previous)[0], turn_date)
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("recording", nargs="?", help="proxy.jsonl of eval_proxy.py")
    parser.add_argument("--traces", nargs="*", default=None, metavar="REPORT",
                        help="report.json files: check the parse and faithful-check ports")
    parser.add_argument("--require", nargs="*", default=[],
                        help="kinds that the recording must have")
    parser.add_argument("--max-diffs", type=int, default=3)
    args = parser.parse_args()
    if args.traces is not None:
        out = check_traces(args.traces)
        print(f"Port check on {len(args.traces)} traces\n")
        print("| Port | Same as C++ |\n|---|---|")
        for k, label in (("parse", "ParseRewriteAnswer (rewrite answers)"),
                         ("rewrite_flag", "IsFaithfulRewrite (rewrite facts)"),
                         ("merge_flag", "IsFaithfulRewrite (merges)")):
            print(f"| {label} | {out[k][0]}/{out[k][1]} |")
        for d in out["diffs"][:args.max_diffs * 3]:
            print(f"\nDIFFERENT: {d}")
        print("\nRESULT: " + ("FAIL" if out["diffs"] else "PASS"))
        sys.exit(1 if out["diffs"] else 0)
    if not args.recording:
        parser.error("give a recording, or --traces")
    records = [json.loads(line) for line in Path(args.recording).read_text().splitlines()
               if line.strip()]
    result = check(records)
    print(f"Parity check of {args.recording} ({len(records)} records)\n")
    print("| Kind | Compared | Recorded | Same | Whole body same |")
    print("|---|---|---|---|---|")
    failed = False
    for kind, r in result.items():
        how = "exact bytes" if kind in DM_KINDS else "message list"
        print(f"| {kind} | {how} | {r['recorded']} | {r['same']} | {r['body_same']} |")
        failed |= r["same"] != r["recorded"]
    missing = [k for k, r in result.items() if r["recorded"] == 0]
    if missing:
        print(f"\nNot reached by this recording: {', '.join(missing)}. "
              "Grow the parity set to cover them (relevance runs only at chat time).")
    for kind, r in result.items():
        for d in r["diffs"][:args.max_diffs]:
            print(f"\nDIFFERENT {kind}, {d}")
    required_missing = [k for k in args.require if k in missing]
    if required_missing:
        print(f"\nFAIL: required kinds missing: {', '.join(required_missing)}")
    print("\nRESULT: " + ("FAIL" if failed or required_missing else "PASS"))
    sys.exit(1 if failed or required_missing else 0)


if __name__ == "__main__":
    main()
