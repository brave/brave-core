#!/usr/bin/env python3
"""Checks the eval datasets (datasets/DATASETS.md).

  ./validate_datasets.py                 # all files, errors only
  ./validate_datasets.py --coverage      # also the coverage table for each split
  ./validate_datasets.py --strict        # a missing coverage tag is an error
  ./validate_datasets.py --root DIR      # another datasets folder

Errors: bad JSON, a missing field, a bad value, a duplicate id, a split that
does not agree with the file name, a text in both splits of a step, a bad
memory id, and bad e2e gold. Exit status 1 when there is an error.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STEPS = ["gate", "sentence", "rewrite", "relation", "merge", "relevance"]
SAFETY = {"ok", "sensitive", "instruction", "short_lived", "not_about_user"}
CATEGORY = {"personal_fact", "preference", "topic", None}
RELATIONS = {"different", "same", "replace", "merge", "same_topic"}
TYPES = {"long_term", "short_term", "permanent"}
COVERAGE = {
    "gate": ["fact_and_question", "task_only", "long_turn", "pasted_text", "other_person",
             ("lang:", 2)],
    "sentence": ["category:personal_fact", "category:preference", "category:topic",
                 "safety:ok", "safety:sensitive", "safety:instruction", "safety:short_lived",
                 "safety:not_about_user", "health", "diet_or_allergy", "job_sounds_medical",
                 "weeks_long", "instruction_or_style", ("lang:", 4)],
    "rewrite": ["relative_date", "several_sentences_one_fact", "pronoun_subject", "names",
                "numbers", "looks_like_prompt_example", ("lang:!en", 1)],
    "relation": ["same", "replace", "merge", "shared_word", "temporary_vs_lasting",
                 "same_topic", "preference_change", "family", "conflict_both_true"],
    "merge": ["count_change", "place_detail", "two_lists"],
    "relevance": ["direct", "indirect", "short_follow_up", "style_wish", "unrelated",
                  ("lang:!en", 1)],
}


class Checker:
    def __init__(self, root: Path):
        self.root = root
        self.errors: list = []
        self.ids: dict = {}

    def error(self, where: str, message: str) -> None:
        self.errors.append(f"{where}: {message}")

    def claim_id(self, where: str, case_id) -> None:
        if not isinstance(case_id, str) or not case_id:
            self.error(where, "missing id")
        elif case_id in self.ids:
            self.error(where, f"duplicate id {case_id} (also in {self.ids[case_id]})")
        else:
            self.ids[case_id] = where


def _is_words(value) -> bool:
    """A must list: words, or lists of words (one of them must match)."""
    return isinstance(value, list) and all(
        isinstance(w, str) or (isinstance(w, list) and w and all(isinstance(x, str) for x in w))
        for w in value)


def _norm(text: str) -> str:
    return re.sub(r"\s+", " ", text.strip().lower())


def check_case(c: Checker, step: str, where: str, case: dict, memory_ids: set) -> str | None:
    """Checks one case. Returns the key for the split overlap check."""
    def need(field, kind, ok=None):
        if field not in case:
            c.error(where, f"missing field {field}")
            return False
        if not isinstance(case[field], kind) or (ok and not ok(case[field])):
            c.error(where, f"bad {field}: {case[field]!r}")
            return False
        return True

    gold = case.get("gold")
    if step in ("gate", "sentence"):
        need("text", str, lambda t: t.strip())
        if step == "gate":
            if gold not in ("keep", "skip", "either"):
                c.error(where, f"bad gold {gold!r}")
        elif not isinstance(gold, dict):
            c.error(where, "missing field gold")
        else:
            for f, ok in (("fact", lambda v: isinstance(v, bool)),
                          ("safety", lambda v: v in SAFETY),
                          ("category", lambda v: v in CATEGORY),
                          ("temporary", lambda v: isinstance(v, bool)),
                          ("keep", lambda v: isinstance(v, bool))):
                if f not in gold:
                    c.error(where, f"missing field gold.{f}")
                elif not ok(gold[f]):
                    c.error(where, f"bad gold.{f}: {gold[f]!r}")
            if gold.get("keep") and (not gold.get("fact") or gold.get("safety") != "ok"):
                c.error(where, "gold.keep is true, but fact is false or safety is not ok")
        return _norm(case.get("text", ""))
    if step == "rewrite":
        need("turn_date", str, lambda d: re.fullmatch(r"\d{4}-\d{2}-\d{2}", d))
        need("sentences", list, lambda s: s and all(isinstance(x, str) and x.strip() for x in s))
        facts = gold.get("facts") if isinstance(gold, dict) else None
        if not isinstance(facts, list) or not facts:
            c.error(where, "missing field gold.facts")
        else:
            for f in facts:
                if not (isinstance(f, dict) and _is_words(f.get("must")) and
                        _is_words(f.get("must_not", [])) and isinstance(f.get("canonical"), str)):
                    c.error(where, f"bad gold fact {f!r}")
        return _norm(" | ".join(case.get("sentences") or []))
    if step == "relation":
        need("new", str, str.strip)
        need("old", str, str.strip)
        if gold not in RELATIONS:
            c.error(where, f"bad gold {gold!r}")
        for f in ("acceptable", "costly"):
            if need(f, list) and not set(case[f]) <= RELATIONS:
                c.error(where, f"bad {f}: {case[f]!r}")
        if gold not in case.get("acceptable", []):
            c.error(where, "gold is not in acceptable")
        if set(case.get("acceptable", [])) & set(case.get("costly", [])):
            c.error(where, "an answer is both acceptable and costly")
        return _norm(f"{case.get('new')} | {case.get('old')}")
    if step == "merge":
        need("old", str, str.strip)
        need("new", str, str.strip)
        need("canonical", str, str.strip)
        if not (isinstance(gold, dict) and _is_words(gold.get("must")) and gold["must"] and
                _is_words(gold.get("must_not", []))):
            c.error(where, "bad gold: needs must (and must_not) word lists")
        return _norm(f"{case.get('old')} | {case.get('new')}")
    if step == "relevance":
        need("message", str, str.strip)
        if need("memories", list):
            bad = [m for m in case["memories"] if m not in memory_ids]
            if bad:
                c.error(where, f"unknown memory ids {bad}")
        for f in ("must", "forbidden"):
            if need(f, list):
                extra = set(case[f]) - set(case.get("memories", []))
                if extra:
                    c.error(where, f"{f} has ids that are not in memories: {sorted(extra)}")
        if set(case.get("must", [])) & set(case.get("forbidden", [])):
            c.error(where, "an id is in must and in forbidden")
        return _norm(f"{case.get('previous_message', '')} | {case.get('message')}")
    return None


def read_jsonl(c: Checker, path: Path) -> list:
    rows = []
    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as e:
            c.error(f"{path.name}:{n}", f"bad JSON: {e}")
            continue
        if not isinstance(row, dict):
            c.error(f"{path.name}:{n}", "a line must be an object")
            continue
        rows.append((n, row))
    return rows


def check_steps(c: Checker, coverage: dict) -> dict:
    counts = {}
    for step in STEPS:
        folder = c.root / "steps" / step
        memory_ids = set()
        if step == "relevance" and (folder / "memories.jsonl").exists():
            for n, m in read_jsonl(c, folder / "memories.jsonl"):
                where = f"steps/relevance/memories.jsonl:{n}"
                c.claim_id(where, m.get("id"))
                if not isinstance(m.get("text"), str) or not m["text"].strip():
                    c.error(where, "missing field text")
                memory_ids.add(m.get("id"))
        keys = {}
        for split in ("dev", "holdout"):
            path = folder / f"{split}.jsonl"
            if not path.exists():
                continue
            tags = coverage.setdefault(step, {}).setdefault(split, {})
            rows = read_jsonl(c, path)
            counts[(step, split)] = len(rows)
            for n, case in rows:
                where = f"steps/{step}/{split}.jsonl:{n}"
                c.claim_id(where, case.get("id"))
                if case.get("split") != split:
                    c.error(where, f"split is {case.get('split')!r}, the file is {split}")
                if not isinstance(case.get("tags"), list):
                    c.error(where, "missing field tags")
                else:
                    for t in case["tags"]:
                        tags[t] = tags.get(t, 0) + 1
                if not isinstance(case.get("origin"), str):
                    c.error(where, "missing field origin")
                key = check_case(c, step, where, case, memory_ids)
                if key is not None:
                    keys.setdefault(key, set()).add(split)
        for key, splits in keys.items():
            if len(splits) > 1:
                c.error(f"steps/{step}", f"text in both splits: {key[:80]!r}")
    return counts


def _turn_ids(chat_set: dict) -> list:
    return [f"c{ci}t{ti}" for ci, chat in enumerate(chat_set.get("chats", []))
            for ti, _ in enumerate(chat.get("turns", []))]


def _check_match(c: Checker, where: str, item: dict) -> None:
    m = item.get("match")
    if not (isinstance(m, dict) and (m.get("all") or m.get("any")) and
            all(isinstance(m.get(k, []), list) for k in ("all", "any"))):
        c.error(where, f"bad match {m!r}")


def check_chat_set(c: Checker, path: Path, need_gold: bool) -> int:
    where = str(path.relative_to(c.root))
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        c.error(where, f"bad JSON: {e}")
        return 0
    if not isinstance(data.get("name"), str) or not isinstance(data.get("chats"), list):
        c.error(where, "needs name and chats")
        return 0
    for ci, chat in enumerate(data["chats"]):
        if not chat.get("turns") or not all(isinstance(t.get("user"), str) for t in chat["turns"]):
            c.error(where, f"chat {ci} needs turns with user text")
    gold = data.get("gold")
    if gold is None:
        if need_gold:
            c.error(where, "missing field gold")
        return len(data["chats"])
    turns = set(_turn_ids(data))
    ids = {}
    for kind in ("facts", "stale", "forbidden"):
        for item in gold.get(kind, []):
            w = f"{where} gold.{kind} {item.get('id')}"
            if not isinstance(item.get("id"), str):
                c.error(w, "missing id")
            elif item["id"] in ids:
                c.error(w, "duplicate gold id")
            ids[item.get("id")] = kind
            _check_match(c, w, item)
            for s in item.get("sources", []):
                if s not in turns:
                    c.error(w, f"unknown source turn {s}")
            if kind == "facts":
                if not isinstance(item.get("required"), bool):
                    c.error(w, "missing field required")
                if item.get("type") not in TYPES | {None}:
                    c.error(w, f"bad type {item.get('type')!r}")
                if item.get("category") not in CATEGORY:
                    c.error(w, f"bad category {item.get('category')!r}")
    for item in gold.get("stale", []):
        if item.get("replaced_by") and ids.get(item["replaced_by"]) != "facts":
            c.error(f"{where} gold.stale {item.get('id')}", "replaced_by is not a fact id")
    for op in gold.get("ops", []):
        w = f"{where} gold.ops {op}"
        if not isinstance(op.get("optional", False), bool):
            c.error(w, "optional must be true or false")
        if op.get("op") == "replace":
            if op.get("old") not in ids or ids.get(op.get("new")) != "facts":
                c.error(w, "replace needs old and new gold ids")
        elif op.get("op") == "merge":
            if not op.get("facts") or any(ids.get(f) != "facts" for f in op["facts"]):
                c.error(w, "merge needs fact ids")
        else:
            c.error(w, "op must be replace or merge")
    gold_turns = gold.get("turns")
    if not isinstance(gold_turns, dict):
        c.error(where, "missing field gold.turns")
    else:
        missing = sorted(turns - set(gold_turns), key=lambda t: [int(x) for x in t[1:].split("t")])
        if missing:
            c.error(where, f"gold.turns has no entry for {', '.join(missing)}")
        for t, facts in gold_turns.items():
            if t not in turns:
                c.error(where, f"gold.turns has an unknown turn {t}")
            for f in facts:
                # A stale item came from a turn too, before a later turn replaced it.
                if ids.get(f) not in ("facts", "stale"):
                    c.error(where, f"gold.turns {t}: {f} is not a fact or stale id")
    for kind in ("facts", "stale"):
        for item in gold.get(kind, []):
            for s in item.get("sources", []):
                if isinstance(gold_turns, dict) and item["id"] not in gold_turns.get(s, []):
                    c.error(where, f"{item['id']} has source {s}, but gold.turns {s} "
                                   "does not list it")
    return len(data["chats"])


def coverage_table(coverage: dict) -> tuple:
    lines, missing = [], []
    for step, needs in COVERAGE.items():
        for split in ("dev", "holdout"):
            tags = coverage.get(step, {}).get(split)
            if tags is None:
                lines.append(f"| {step} | {split} | (no file) | |")
                continue
            absent = []
            for need in needs:
                if isinstance(need, tuple):
                    prefix, n = need
                    if prefix == "lang:!en":
                        have = [t for t in tags if t.startswith("lang:") and t != "lang:en"]
                    else:
                        have = [t for t in tags if t.startswith(prefix)]
                    if len(have) < n:
                        absent.append(f"{n}x {prefix}* (has {len(have)})")
                elif need not in tags:
                    absent.append(need)
            lines.append(f"| {step} | {split} | {len(needs) - len(absent)}/{len(needs)} | "
                         f"{', '.join(absent)} |")
            missing += [f"{step}/{split}: {a}" for a in absent]
    return lines, missing


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=str(HERE / "datasets"))
    parser.add_argument("--coverage", action="store_true", help="print the coverage table")
    parser.add_argument("--strict", action="store_true", help="a missing tag is an error")
    args = parser.parse_args()
    root = Path(args.root)
    c = Checker(root)
    card = root / "DATASETS.md"
    if not card.exists() or not re.search(r"^Version:\s*\S+", card.read_text(), re.M):
        c.error("DATASETS.md", "missing, or it has no 'Version:' line")
    coverage: dict = {}
    counts = check_steps(c, coverage)
    sets = {}
    for split in ("dev", "holdout"):
        for path in sorted((root / "e2e" / split).glob("*.json")):
            sets[f"e2e/{split}/{path.stem}"] = check_chat_set(c, path, need_gold=True)
    for path in sorted((root / "parity").glob("*.json")):
        sets[f"parity/{path.stem}"] = check_chat_set(c, path, need_gold=False)

    print("| Dataset | Cases |\n|---|---|")
    for (step, split), n in counts.items():
        print(f"| steps/{step}/{split} | {n} |")
    for name, n in sets.items():
        print(f"| {name} | {n} chats |")
    lines, missing = coverage_table(coverage)
    if args.coverage or args.strict:
        print("\n| Step | Split | Coverage | Missing tags |\n|---|---|---|---|")
        print("\n".join(lines))
    if args.strict:
        c.errors += [f"coverage: {m}" for m in missing]
    elif missing:
        print(f"\n{len(missing)} coverage tags are missing (see --coverage). "
              "This is not an error until the sets grow (plan P1.3).")
    if c.errors:
        print(f"\n{len(c.errors)} errors:")
        for e in c.errors:
            print(f"  - {e}")
        sys.exit(1)
    print("\nOK: no errors.")


if __name__ == "__main__":
    main()
