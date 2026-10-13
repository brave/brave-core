#!/usr/bin/env python3
"""Adds the true extra memories of an e2e run to the gold as optional facts
(decision of 2026-10-08; DATASETS.md version 9).

An "extra" is a stored memory that matches no gold item. The judge
(run_eval.py --judge) sorts the extras; the class `true_unlabeled` means that
the memory is true, safe and stated by the user, and that the gold only lacks
it. This script turns those memories into optional facts:

  1. collect   the true_unlabeled extras of <run>/extras_judge.json, with their source turns
  2. propose   one agent (Opus, high) for each set groups them into facts, writes the match
               words, or rejects a memory (sensitive, an instruction, not lasting, not stated)
  3. check     a script: the match words must match every memory that the fact covers, from a
               source turn of the fact, and must not match any other fact (both ways)
  4. apply     appends the facts (required: false) to datasets/e2e/<split>/<set>.json
  5. verify    scores the run again with the new gold: extras go down, recall, leaks and
               ops do not change, duplicates are listed

  ./extras_to_gold.py <run dir> --split dev            # steps 1-3 and 5 on a copy; nothing is changed
  ./extras_to_gold.py <run dir> --split dev --apply    # also steps 4 and 5 on the real files

Run it only for dev sets. Never use the extras of a holdout run (data rule 2).
"""

from __future__ import annotations

import argparse
import copy
import json
import re
import shutil
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import agent_run
from lib import agent_tasks, clients
from lib import e2e_panel as E
from lib import gold as G

HERE = Path(__file__).resolve().parent
DATASETS = HERE / "datasets"
NOTE = (
    "Optional (dataset version 9): a true fact that the user states. It was stored and found as an extra "
    "in baseline-e2e-v2, and the judge sorted it as true_unlabeled."
)


def collect(run_dir: Path, sets_dir: Path) -> dict:
    """{set name: [item]} for the true_unlabeled extras of a run."""
    judged = json.loads((run_dir / "extras_judge.json").read_text())
    verdicts = {r["name"]: r["verdict"] for r in judged["rows"]}
    out: dict = {}
    for path in sorted(run_dir.glob("*/r*/report.json")):
        chat_set = json.loads(
            (sets_dir / f"{path.parent.parent.name}.json").read_text()
        )
        data = json.loads(path.read_text())
        if "error" in data or chat_set.get("gold") is None:
            continue
        g = G.score(
            chat_set["gold"], data.get("memories", []), data.get("trace", [])
        )
        rel = path.parent.relative_to(run_dir)
        for n, (memory, sources) in enumerate(
            zip(g["extras"], g["extra_sources"])
        ):
            name = f"{rel}|{n}"
            if verdicts.get(name, {}).get("class") != "true_unlabeled":
                continue
            out.setdefault(chat_set["name"], []).append(
                {
                    "id": name,
                    "memory": memory,
                    "source_turns": sorted({s.split(" ")[0] for s in sources}),
                    "sentences": [
                        s.split(": ", 1)[1] for s in sources if ": " in s
                    ],
                }
            )
    return out


def task_text(name: str, rubric: str) -> str:
    return f"""# Agent task: add true facts to the gold of a chat set (set: {name})

A memory feature stored the memories in `items.jsonl` from the chats in
`set.json`. They match no item of the gold, and a judge said that each one is a
true, safe fact that the user states. You add them to the gold as OPTIONAL
facts, so that they are no longer counted as extras. `set.json` has the chats
and the current gold. Read both first.

## Rubric (the format and the match rules of the gold)

{rubric}

## What to do

1. Many memories say the same fact in other words (the feature ran 3 times).
   Group them. Write one fact for each idea in `new_facts.json`: a list of
   objects with `id` (snake_case, new, not used in the gold), `canonical` (short,
   third person, no subject), `match` (`all` and `any` words), `category`
   (`personal_fact`, `preference`, `topic` or null), `type` (`long_term`,
   `short_term` or null) and `sources` (the turn ids where the user states it).
   Do not write `required`: the script sets it to false.
2. The match words must:
   - match every memory text of the group (the script checks it, case
     insensitive, substring),
   - be words that every good memory of the fact has, and not match the
     canonical text of any other fact in the gold or in `new_facts.json`
     (use names, numbers, nouns; never one common word like "dog" or "uses"),
   - not match the text of a forbidden item.
3. Reject a memory (do not add it) when it is: sensitive or an instruction (see
   Forbidden in the rubric), not stated in the turn, a state of hours or days,
   too vague to match safely, or already covered by a fact of the gold. Say which
   in `reason`. For "already covered" give the gold fact id in `fact`.
4. Write `answers.jsonl`: one JSON line for each item, with the `id` of the item:

{{"id": "...", "action": "add" | "reject", "fact": "<id of the new or the gold fact>" | null, "reason": "one short sentence"}}

Write valid JSON. `fact` is required for `add`.
"""


def check(items: list, doc: dict, facts: list, answers: dict) -> list:
    """Errors in the proposal of the agent for one set."""
    errors = []
    gold = doc["gold"]
    old_ids = (
        {f["id"] for f in gold["facts"]}
        | {s["id"] for s in gold["stale"]}
        | {f["id"] for f in gold["forbidden"]}
    )
    turns = {
        f"c{c}t{t}"
        for c, chat in enumerate(doc["chats"])
        for t, _ in enumerate(chat["turns"])
    }
    new_ids = set()
    for f in facts:
        for key in ("id", "canonical", "match", "category", "type", "sources"):
            if key not in f:
                errors.append(f"a fact lacks {key}: {f}")
        if errors:
            return errors
        if not re.fullmatch(r"[a-z][a-z0-9_]*", f["id"]):
            errors.append(f"{f['id']}: the id must be snake_case")
        if f["id"] in old_ids or f["id"] in new_ids:
            errors.append(f"{f['id']}: the id is not new")
        new_ids.add(f["id"])
        m = f["match"]
        if not (isinstance(m, dict) and (m.get("all") or m.get("any"))):
            errors.append(f"{f['id']}: bad match")
            continue
        if f["category"] not in (
            "personal_fact",
            "preference",
            "topic",
            None,
        ) or f["type"] not in ("long_term", "short_term", None):
            errors.append(f"{f['id']}: bad category or type")
        if not f["sources"] or any(s not in turns for s in f["sources"]):
            errors.append(f"{f['id']}: sources must be turn ids of the set")
        if not G.matches(m, f["canonical"]):
            errors.append(
                f"{f['id']}: the match does not match its own canonical"
            )
        for other in gold["facts"] + [x for x in facts if x is not f]:
            if G.matches(m, other["canonical"]):
                errors.append(
                    f"{f['id']}: the match also matches the canonical of {other['id']}"
                )
            if G.matches(other["match"], f["canonical"]):
                errors.append(
                    f"{f['id']}: the match of {other['id']} also matches this canonical"
                )
        for item in gold["forbidden"]:
            if G.matches(item["match"], f["canonical"]):
                errors.append(
                    f"{f['id']}: a forbidden match ({item['id']}) matches this canonical"
                )
    by_fact = {f["id"]: f for f in facts}
    covered = set()
    for item in items:
        a = answers.get(item["id"])
        if a is None:
            errors.append(f"no answer for {item['id']}")
            continue
        if a.get("action") not in ("add", "reject"):
            errors.append(f"{item['id']}: action must be add or reject")
        elif not isinstance(a.get("reason"), str):
            errors.append(f"{item['id']}: reason must be text")
        elif a["action"] == "add":
            f = by_fact.get(a.get("fact"))
            if f is None:
                errors.append(
                    f"{item['id']}: add needs a fact id from new_facts.json"
                )
                continue
            covered.add(f["id"])
            if not G.matches(f["match"], item["memory"]):
                errors.append(
                    f"{item['id']}: the match of {f['id']} does not match the memory {item['memory']!r}"
                )
            if not set(item["source_turns"]) & set(f["sources"]):
                errors.append(
                    f"{item['id']}: no source turn of {f['id']} is a source turn of the memory "
                    f"({item['source_turns']})"
                )
    for f in facts:
        if f["id"] not in covered:
            errors.append(f"{f['id']}: no memory uses this fact; remove it")
    return errors


def propose(args, items_by_set: dict, sets_dir: Path, out: Path) -> dict:
    """Runs one agent for each set. Returns {set: (facts, answers)}."""
    root = clients.results_root(args.results)
    rubric = E.rubric_text(HERE)[0]
    model, thinking = agent_run.DEFAULT_MODELS["generate"]

    def one(name, items):
        doc = json.loads((sets_dir / f"{name}.json").read_text())
        folder = agent_tasks.task_root(root) / f"extras-gold-{name}"
        agent_tasks.write_task(folder, task_text(name, rubric), items)
        (folder / "set.json").write_text(
            json.dumps(doc, ensure_ascii=False, indent=1)
        )

        def validate(item, a):
            try:
                facts = json.loads((folder / "new_facts.json").read_text())
            except (OSError, json.JSONDecodeError) as e:
                return f"new_facts.json: {e}"
            if not isinstance(facts, list):
                return "new_facts.json must be a list"
            answers = {
                x["id"]: x
                for x in agent_tasks.read_answers(folder)[1]
                if "id" in x
            }
            problems = check(items, doc, facts, answers)
            return (
                ("; ".join(problems[:12]) if problems else None)
                if item["id"] == items[0]["id"]
                else None
            )

        res = agent_run.run_tasks(
            [folder],
            model,
            thinking,
            validate,
            "extras-gold",
            parallel=1,
            agent_cmd=args.agent_cmd,
            log=lambda m: print(f"[{name}] {m}", flush=True),
        )
        answers, errors = res[folder]
        if errors:
            print(
                f"{name}: the proposal has errors after all tries: {errors[:2]}"
            )
            return name, None
        facts = json.loads((folder / "new_facts.json").read_text())
        shutil.copy(folder / "new_facts.json", out / f"{name}.new_facts.json")
        return name, (facts, answers)

    with ThreadPoolExecutor(max(1, len(items_by_set))) as pool:
        result = {
            n: r
            for n, r in pool.map(lambda kv: one(*kv), items_by_set.items())
            if r
        }
    return result


def merge(doc: dict, facts: list) -> dict:
    new = copy.deepcopy(doc)
    for f in facts:
        new["gold"]["facts"].append(
            {
                "id": f["id"],
                "canonical": f["canonical"],
                "match": f["match"],
                "required": False,
                "type": f["type"],
                "category": f["category"],
                "sources": f["sources"],
                "note": NOTE,
            }
        )
        for s in f["sources"]:
            new["gold"]["turns"].setdefault(s, []).append(f["id"])
    return new


def score_run(run_dir: Path, docs: dict) -> dict:
    """Pooled numbers of a run scored with |docs| (set name -> chat set with gold)."""
    total = {
        "required": 0,
        "found": 0,
        "memories": 0,
        "matched": 0,
        "extras": 0,
        "leaks": 0,
        "wrong_ops": 0,
        "missed_ops": 0,
        "duplicates": 0,
        "stale_kept": 0,
    }
    for path in sorted(run_dir.glob("*/r*/report.json")):
        name = path.parent.parent.name
        if name not in docs:
            continue
        data = json.loads(path.read_text())
        if "error" in data:
            continue
        g = G.score(
            docs[name]["gold"], data.get("memories", []), data.get("trace", [])
        )
        total["required"] += g["required"]
        total["found"] += len(g["found"])
        total["memories"] += g["memories"]
        total["matched"] += g["matched_memories"]
        total["extras"] += len(g["extras"])
        total["leaks"] += len(g["leaks"])
        total["wrong_ops"] += len(g["wrong_ops"])
        total["missed_ops"] += len(g["missed_ops"])
        total["duplicates"] += len(g["duplicates"])
        total["stale_kept"] += len(g["stale_kept"])
    return total


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "run", help="an e2e run folder with extras_judge.json (dev sets only)"
    )
    parser.add_argument("--split", choices=["dev"], default="dev")
    parser.add_argument(
        "--apply",
        action="store_true",
        help="write the facts to datasets/e2e/<split>/",
    )
    parser.add_argument("--results", default=None)
    parser.add_argument("--agent-cmd", default="pi")
    args = parser.parse_args()
    run_dir = Path(args.run)
    sets_dir = DATASETS / "e2e" / args.split
    out = clients.results_root(args.results) / "extras_to_gold" / run_dir.name
    out.mkdir(parents=True, exist_ok=True)

    items = collect(run_dir, sets_dir)
    print(
        f"{sum(len(v) for v in items.values())} true extras in {len(items)} sets: "
        + ", ".join(f"{k} {len(v)}" for k, v in items.items())
    )
    (out / "items.json").write_text(
        json.dumps(items, indent=1, ensure_ascii=False)
    )
    proposals = propose(args, items, sets_dir, out)
    docs_old = {
        p.stem: json.loads(p.read_text()) for p in sets_dir.glob("*.json")
    }
    docs_new = dict(docs_old)
    for name, (facts, answers) in proposals.items():
        docs_new[name] = merge(docs_old[name], facts)
        added = sum(1 for a in answers.values() if a["action"] == "add")
        print(
            f"{name}: {len(facts)} new facts cover {added} of {len(items[name])} memories; "
            f"{len(items[name]) - added} rejected"
        )
        for i, a in answers.items():
            if a["action"] == "reject":
                print(f"    rejected {i}: {a['reason']}")
    before = score_run(run_dir, docs_old)
    after = score_run(run_dir, docs_new)
    lines = ["| | Before | After |", "|---|---|---|"]
    for k in before:
        lines.append(f"| {k} | {before[k]} | {after[k]} |")
    print("\n".join(lines))
    (out / "verify.md").write_text("\n".join(lines) + "\n")
    problems = [
        k
        for k in (
            "required",
            "found",
            "leaks",
            "wrong_ops",
            "missed_ops",
            "stale_kept",
        )
        if before[k] != after[k]
    ]
    if problems:
        sys.exit(
            f"STOP: {problems} changed. Optional facts must not change these numbers."
        )
    if after["duplicates"] > before["duplicates"]:
        print(
            f"WARNING: duplicates go from {before['duplicates']} to {after['duplicates']}: "
            "a new fact matches two memories of one run. Look at verify.md and the facts."
        )
    if args.apply:
        for name, (facts, _) in proposals.items():
            if not facts:
                continue  # do not rewrite a file that does not change
            path = sets_dir / f"{name}.json"
            path.write_text(
                json.dumps(docs_new[name], indent=1, ensure_ascii=False) + "\n"
            )
            print(f"wrote {path.relative_to(HERE)}")
    else:
        print(
            "\nNothing was changed. Run again with --apply to write the facts."
        )


if __name__ == "__main__":
    main()
