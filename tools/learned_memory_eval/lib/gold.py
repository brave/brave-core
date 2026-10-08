"""Scores an end-to-end run against the gold of its chat set (plan 8.2, P3.1)
and finds the step that lost each missed fact (P3.2).

The gold format is in datasets/DATASETS.md, section 4. A memory matches a gold
item when its text has every word of match.all and one word of match.any
(case-insensitive substrings, the same as "expect").
"""
from __future__ import annotations

import re

STAGES = ["not_read", "gate", "denylist", "sentence", "rewrite", "relation", "stored"]


def from_sources(item: dict, links: list | None) -> bool:
    """True when a memory with |links| comes from a source turn of |item|, or
    when one side has no sources. A word match alone is not enough: an LLM
    rewrite can invent a text with the right words in another turn (for
    example "Owls lives in Berlin" from a turn about a dog)."""
    sources = set(item.get("sources") or [])
    if not sources or links is None:
        return True
    return bool(sources & {turn_id(l.get("entry")) for l in links})


def item_matches(item: dict, text: str | None, links: list | None) -> bool:
    return matches(item["match"], text) and from_sources(item, links)


def matches(match: dict, text: str | None) -> bool:
    if not text:
        return False
    t = text.lower()
    all_words = [w.lower() for w in match.get("all", [])]
    any_words = [w.lower() for w in match.get("any", [])]
    return all(w in t for w in all_words) and (not any_words or any(w in t for w in any_words))


def _sentence_match(match: dict, text: str) -> bool:
    """A looser match for a user sentence: the words of "all" only, or one of
    "any" when there is no "all"."""
    t = text.lower()
    if match.get("all"):
        return all(w.lower() in t for w in match["all"])
    return any(w.lower() in t for w in match.get("any", []))


def turn_id(entry: str | None) -> str | None:
    """eval-3-0-user -> c3t0."""
    m = re.fullmatch(r"eval-(\d+)-(\d+)-user", entry or "")
    return f"c{m.group(1)}t{m.group(2)}" if m else None


def entry_of(tid: str) -> str:
    c, t = tid[1:].split("t")
    return f"eval-{c}-{t}-user"


def store_ops(trace: list) -> list:
    """The text changes of the run: (kind, old text, new text, entry, old links,
    new links). A replace or a merge is a store "update" after a
    "relation_applied" step that allows the change."""
    ops, pending = [], None
    links: dict = {}  # memory text -> its links, from the store steps
    for step in trace:
        if step["step"] == "relation_applied":
            pending = step if step["relation"] in ("replace", "merge") and step["text_change_allowed"] else None
        elif step["step"] == "store":
            if pending and step["action"] == "update":
                ops.append((pending["relation"], pending["old"], step["text"], step.get("entry"),
                            links.get(pending["old"]), step.get("links")))
            links[step["text"]] = step.get("links")
            pending = None
        elif step["step"] in ("fact", "watermark"):
            pending = None
    return ops


def score(gold: dict, memories: list, trace: list) -> dict:
    facts = gold.get("facts", [])
    stale = gold.get("stale", [])
    forbidden = gold.get("forbidden", [])
    by_fact = {f["id"]: [m for m in memories if item_matches(f, m["text"], m.get("links"))]
               for f in facts}
    required = [f for f in facts if f["required"]]
    found = [f["id"] for f in required if by_fact[f["id"]]]
    missed = [f["id"] for f in required if not by_fact[f["id"]]]
    matched_mem = [m for m in memories if any(item_matches(f, m["text"], m.get("links")) for f in facts)]
    leaks = []
    for item in forbidden:
        for m in memories:
            if matches(item["match"], m["text"]) or matches(item["match"], m.get("previous")):
                leaks.append({"id": item["id"], "text": m["text"]})
    stale_kept = [{"id": s["id"], "text": m["text"]} for s in stale for m in memories
                  if item_matches(s, m["text"], m.get("links"))]
    duplicates = {fid: [m["text"] for m in ms] for fid, ms in by_fact.items() if len(ms) > 1}
    labels = [0, 0]
    label_errors = []
    for f in facts:
        for m in by_fact[f["id"]]:
            for key in ("type", "category"):
                if f.get(key):
                    labels[1] += 1
                    if m[key] == f[key]:
                        labels[0] += 1
                    else:
                        label_errors.append(f"{f['id']}: {key} {m[key]} (gold {f[key]})")
    items = facts + stale + forbidden
    # Leaks count from any turn, so forbidden items match by their words only.
    extras = [m for m in memories
              if not any(item_matches(i, m["text"], m.get("links")) for i in facts + stale)
              and not any(matches(i["match"], m["text"]) for i in forbidden)]

    # Ops: each text change of the trace against the gold ops.
    def ids(text, links):
        return {i["id"] for i in facts + stale if item_matches(i, text, links)}

    done, wrong = [], []
    gold_ops = gold.get("ops", [])
    for kind, old, new, entry, old_links, new_links in store_ops(trace):
        old_ids, new_ids = ids(old, old_links), ids(new, new_links)
        hit = None
        for n, op in enumerate(gold_ops):
            if op["op"] == "replace" and kind == "replace" and op["old"] in old_ids and op["new"] in new_ids:
                hit = n
            elif op["op"] == "merge" and kind == "merge" and (old_ids & set(op["facts"])) and \
                    set(op["facts"]) <= new_ids | old_ids and (new_ids & set(op["facts"])):
                hit = n
        if hit is not None:
            done.append(hit)
        elif old_ids and old_ids == new_ids and kind in ("replace", "merge"):
            pass  # The same gold fact in better words, for example a plan that came true.
        else:
            wrong.append({"op": kind, "old": old, "new": new, "turn": turn_id(entry),
                          "old_gold": sorted(old_ids), "new_gold": sorted(new_ids)})
    missed_ops = [op for n, op in enumerate(gold_ops) if n not in done and not op.get("optional")]
    return {
        "required": len(required), "found": found, "missed": missed,
        "recall": len(found) / len(required) if required else None,
        "memories": len(memories), "matched_memories": len(matched_mem),
        "precision": len(matched_mem) / len(memories) if memories else None,
        "leaks": leaks, "stale_kept": stale_kept, "wrong_ops": wrong, "missed_ops": missed_ops,
        "duplicates": duplicates, "label_accuracy": labels[0] / labels[1] if labels[1] else None,
        "label_errors": label_errors, "extras": [m["text"] for m in extras],
        "extra_sources": [_sources(m, trace) for m in extras]}


def _split_texts(trace: list) -> dict:
    """entry -> {sentence index: text}."""
    out = {}
    for step in trace:
        if step["step"] == "split":
            out[step["entry"]] = {s["index"]: s["text"] for s in step["sentences"]}
    return out


def _sources(memory: dict, trace: list) -> list:
    texts = _split_texts(trace)
    return [f"{turn_id(l['entry'])} #{l['sentence']}: {texts.get(l['entry'], {}).get(l['sentence'], '?')}"
            for l in memory.get("links", [])]


def _by_entry(trace: list) -> dict:
    out: dict = {}
    for step in trace:
        if step.get("entry"):
            out.setdefault(step["entry"], []).append(step)
    return out


def _turn_stage(steps: list, match: dict) -> tuple:
    """How far the fact of one source turn went: (stage, reason)."""
    if not steps or not any(s["step"] == "turn" for s in steps):
        return "not_read", "not_read"
    gate = next((s for s in steps if s["step"] == "gate"), None)
    if gate is None:
        return "not_read", "not_read: the run stopped"
    if gate["result"] == "skip":
        return "gate", f"gate ({gate['probabilities'].get('yes', 0):.2f})"
    split = next((s for s in steps if s["step"] == "split"), {"sentences": []})
    sentences = split["sentences"]
    rel = [s["index"] for s in sentences if _sentence_match(match, s["text"])] or \
          [s["index"] for s in sentences]
    if all(s["denylist"] for s in sentences if s["index"] in rel):
        return "denylist", "denylist"
    sd = next((s for s in steps if s["step"] == "sentence_decisions"), None)
    if sd is None:
        return "denylist", "denylist"
    decisions = {s["index"]: s for s in sd["sentences"]}
    kept = [i for i in rel if i in decisions and decisions[i]["result"] == "keep"]
    if not kept:
        result = next((decisions[i]["result"] for i in rel if i in decisions), "drop: ?")
        m = re.match(r"drop: (\w[\w ]*?) \(", result)
        reason = (m.group(1) if m else result).replace(" ", "_")
        return "sentence", f"sentence:{reason}"
    checked = next((s for s in steps if s["step"] == "rewrite_checked"), {"facts": []})
    matching = [f["text"] for f in checked["facts"] if matches(match, f["text"])]
    if not matching:
        parsed = next((s for s in steps if s["step"] == "rewrite_parsed"), {"facts": []})
        candidates = [s["index"] for s in sd["sentences"] if s["result"] == "keep"]
        positions = {candidates.index(i) for i in kept}
        covered = any(positions & set(f["sources"]) for f in parsed["facts"])
        return "rewrite", "rewrite:changed" if covered else "rewrite:dropped"
    # Follow each matching fact through the relation step to the store.
    reason = "relation:?"
    current = applied = None
    for s in steps:
        if s["step"] == "fact":
            current = s["text"] if s["text"] in matching else None
            applied = None
        elif current is None:
            continue
        elif s["step"] == "relation_applied":
            applied = s
        elif s["step"] == "store":
            # A store keeps the fact only when the stored text has its words. A
            # "same" with another memory, or a merge, can store other words.
            if matches(match, s["text"]):
                return "stored", "stored"
            if applied and applied["relation"] == "merge":
                reason = f"merge (\"{s['text']}\")"
            else:
                reason = f"relation:wrong_same (\"{applied['old'] if applied else s['text']}\")"
            current = None
        elif s["step"] == "fact_dropped":
            reason = ("relation:not_certain" if "not certain" in s["reason"]
                      else "relation:no_llm_answer")
            current = None
    return "relation", reason


def attribute(gold: dict, trace: list, missed: list) -> dict:
    """For each missed fact id: the first step that lost it."""
    facts = {f["id"]: f for f in gold.get("facts", [])}
    by_entry = _by_entry(trace)
    out = {}
    for fid in missed:
        f = facts[fid]
        best = ("not_read", "not_read", None)
        for tid in f.get("sources", []):
            stage, reason = _turn_stage(by_entry.get(entry_of(tid), []), f["match"])
            if STAGES.index(stage) >= STAGES.index(best[0]):
                best = (stage, reason, tid)
        stage, reason, tid = best
        if stage == "stored":
            # Stored, then a later text change removed it.
            reason = "stored, then lost"
            for kind, old, new, entry, old_links, new_links in store_ops(trace):
                if item_matches(f, old, old_links) and not item_matches(f, new, new_links):
                    reason = f"{'replaced' if kind == 'replace' else 'merge'} in {turn_id(entry)}: \"{new}\""
                    break
        out[fid] = {"step": reason.split(" ")[0].split("(")[0], "detail": reason, "turn": tid}
    return out
