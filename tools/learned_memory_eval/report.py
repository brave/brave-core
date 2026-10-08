"""Scores a report.json of the browser and writes report.md.

See README.md. The expectations of a chat set are in its "expect" key.
"""
from __future__ import annotations

import statistics

from lib import gold as gold_lib
from lib import scoring as sc


def matches(item: dict, text: str) -> bool:
    """A memory matches when it has all words of "all" and one word of "any"."""
    t = text.lower()
    all_words = [w.lower() for w in item.get("all", [])]
    any_words = [w.lower() for w in item.get("any", [])]
    return all(w in t for w in all_words) and (
        not any_words or any(w in t for w in any_words))


def _haystack(memory: dict) -> str:
    return f'{memory["text"]} {memory.get("previous") or ""}'


def score(expect: dict, memories: list) -> list:
    """Returns rows of (kind, label, ok, detail)."""
    rows = []
    for item in expect.get("memories", []):
        found = [m for m in memories if matches(item, m["text"]) and
                 (not item.get("type") or m["type"] == item["type"])]
        rows.append(("expected", item["label"], bool(found),
                     f'"{found[0]["text"]}"' if found else "no memory matches"))
    for item in expect.get("absent", []):
        # Only the text. The old text of a replaced memory is expected.
        found = [m for m in memories if matches(item, m["text"])]
        rows.append(("absent", item["label"], not found,
                     f'found "{found[0]["text"]}"' if found else "none found"))
    for item in expect.get("types", []):
        found = [m for m in memories if matches(item, m["text"])]
        bad = [m for m in found if m["type"] != item["type"]]
        detail = ("not stored" if not found else
                  f'"{bad[0]["text"]}" is {bad[0]["type"]}' if bad else
                  f'{len(found)} stored, all {item["type"]}')
        rows.append(("type", item["label"], not bad, detail))
    for item in expect.get("at_most_one", []):
        found = [m for m in memories if matches(item, m["text"])]
        rows.append(("at most one", item["label"], len(found) <= 1,
                     f"{len(found)} memories" +
                     ("" if len(found) <= 1 else ": " +
                      "; ".join(f'"{m["text"]}"' for m in found))))
    for word in expect.get("forbidden", []):
        hits = [m for m in memories if word.lower() in _haystack(m).lower()]
        rows.append(("forbidden", word, not hits,
                     f'in "{hits[0]["text"]}"' if hits else "not in any memory"))
    if "max_memories" in expect:
        limit = expect["max_memories"]
        rows.append(("count", f"at most {limit} memories",
                     len(memories) <= limit, f"{len(memories)} memories"))
    return rows


def _probs(d: dict) -> str:
    return " · ".join(f"{k} {v:.2f}" for k, v in
                      sorted(d.items(), key=lambda kv: -kv[1]))


def _quote(text: str, lang: str = "") -> str:
    return f"```{lang}\n{text.rstrip()}\n```"


def _latencies(trace: list) -> dict:
    groups: dict = {}
    for step in trace:
        if "latency_ms" not in step:
            continue
        name = step["step"]
        if name in ("llm_call", "embed"):
            name = f'{name}: {step.get("purpose")}'
        groups.setdefault(name, []).append(step["latency_ms"])
    return groups


def _render_step(step: dict) -> list:
    name = step["step"]
    ms = f' ({step["latency_ms"]} ms)' if "latency_ms" in step else ""
    out = []
    if name == "gate":
        out.append(f'- **gate**{ms}: {step["result"]} — {_probs(step["probabilities"])}')
    elif name == "split":
        out.append("- **split**: " + "; ".join(
            f'#{s["index"]} {"DENYLIST " if s["denylist"] else ""}"{s["text"]}"'
            for s in step["sentences"]))
    elif name == "sentence_decisions":
        out.append(f"- **sentence decisions**{ms}")
        out.append("")
        out.append("  | # | Sentence | Fact | Safety | Category | Temporary | Result |")
        out.append("  |---|---|---|---|---|---|---|")
        for s in step["sentences"]:
            out.append(
                f'  | {s["index"]} | {s["text"]} | {_probs(s["fact"])} | '
                f'{_probs(s["safety"])} | {s["category_answer"]} ({_probs(s["category"])}) | '
                f'{s["temporary_answer"]} ({_probs(s["temporary"])}) | **{s["result"]}** |')
        out.append("")
    elif name == "llm_call":
        out.append(f'- **LLM {step["purpose"]}**{ms}'
                   + (f' — ERROR {step["error"]}' if step.get("error") else ""))
        out.append("  - request:")
        out.append(_quote(step["request"]))
        out.append("  - answer:")
        out.append(_quote(step["answer"] or "(none)", "json"))
    elif name == "rewrite_parsed":
        facts = "; ".join(
            f'"{f["text"]}" from {f["sources"]} '
            f'({"no new details" if f["no_new_details"] else "NEW DETAILS (flag only)"}'
            f'{", TOO LONG" if f.get("too_long") else ""})'
            for f in step["facts"]) or "(no facts)"
        out.append(f'- **rewrite parsed**: used LLM {step["used_llm"]}, '
                   f'parsed {step["parsed"]} — {facts}')
    elif name == "embed":
        out.append(f'- **embed {step["purpose"]}**{ms}: {len(step["passages"])} '
                   f'passages, {step["dimensions"]} dimensions, status {step["status"]}')
    elif name == "rewrite_checked":
        out.append("- **facts for the relation step**: " + ("; ".join(
            f'"{f["text"]}" ({f["category"]}, {f["type"]})'
            for f in step["facts"]) or "(none)"))
    elif name == "fact":
        out.append(f'- **fact**: "{step["text"]}" ({step["category"]}, {step["type"]})')
    elif name == "fact_dropped":
        out.append(f'- **fact dropped**: {step["reason"]}'
                   + (f' (similarity {step["similarity"]:.2f})' if "similarity" in step else ""))
    elif name == "neighbors":
        near = "; ".join(f'"{n["text"]}" {n["similarity"]:.2f}'
                         for n in step["neighbors"]) or "none above the floor"
        out.append(f'- **closest memories** ({step["memories"]} stored, best '
                   f'{step["best_similarity"]:.2f}, floor {step["floor"]}): {near}')
    elif name == "relations":
        out.append(f'- **relation**{ms}, new "{step["new"]}"')
        for p in step["pairs"]:
            out.append(f'  - vs "{p["old"]}": **{p["answer"]}** — {_probs(p["probabilities"])}')
    elif name == "llm_relation":
        out.append(f'- **LLM relation** vs "{step["old"]}": **{step["answer"]}**')
    elif name == "relation_applied":
        out.append(f'- **relation applied**: {step["relation"]} with "{step["old"]}" '
                   f'({step["old_type"]}), text change allowed: {step["text_change_allowed"]}')
    elif name == "merge_parsed":
        out.append(f'- **merge parsed**: "{step["text"]}", no new details: '
                   f'{step["no_new_details"]} (flag only), too long: '
                   f'{step.get("too_long", False)}')
    elif name == "store":
        prev = f' (old text: "{step["previous"]}")' if step.get("previous") else ""
        out.append(f'- **store ({step["action"]})**: "{step["text"]}" '
                   f'({step["category"]}, {step["type"]}){prev}')
    elif name == "index_current":
        out.append(f'- index of the learned memories current after {step["t_ms"]} ms')
    elif name == "watermark":
        out.append(f'- watermark → {step["date"]}')
    else:
        out.append(f"- {name}: {step}")
    return out


def render(chat_set: dict, report: dict, extra: dict | None = None) -> tuple:
    """Returns (markdown, summary dict)."""
    extra = extra or {}
    result = report.get("result", {})
    memories = report.get("memories", [])
    trace = report.get("trace", [])
    rows = score(chat_set.get("expect", {}), memories)
    if "error" in report:
        # A run with no report passes nothing.
        rows = [(k, l, False, "the run failed") for k, l, _, _ in rows]
    passed = sum(1 for r in rows if r[2])
    forbidden_hits = sum(1 for r in rows if r[0] == "forbidden" and not r[2])
    total_ms = next((s["total_ms"] for s in trace if s["step"] == "done"), 0)

    md = [f'# Eval report: {chat_set["name"]}', "", chat_set.get("description", ""), ""]
    if "error" in report:
        md += [f'**Error:** {report["error"]}', ""]
    cfg = report.get("config", {})
    # A run that stops while it waits for the embedder is the usual setup error.
    steps = [x["step"] for x in trace if x["step"] != "done"]
    if (result.get("status") == "timed_out" and steps and
            steps[-1] in ("rewrite_parsed", "merge_parsed")):
        md += ["> **Diagnosis:** the run stopped while it waited for the embedder. "
               "The EmbeddingGemma model is probably not loaded. Check that the "
               "`HistoryEmbeddings` feature is on, and that the profile has the "
               "`BraveLocalAIModels` component.", ""]
    md += ["## Result", "",
           "| Status | Turns read | Turns kept | Memories added | Updated | Stored | Time |",
           "|---|---|---|---|---|---|---|",
           f'| {result.get("status")} | {result.get("turns_read")} | '
           f'{result.get("turns_kept")} | {result.get("memories_added")} | '
           f'{result.get("memories_updated")} | {len(memories)} | {total_ms / 1000:.1f} s |',
           "",
           f'Config: threshold {cfg.get("certain_threshold")}, margin '
           f'{cfg.get("certain_margin")}, time limit {cfg.get("time_limit_s")} s, '
           f'decision model `{cfg.get("decision_model")}`, local LLM `{cfg.get("local_llm_model")}`.',
           ""]

    md += [f"## Expectations: {passed}/{len(rows)} pass", "",
           "| | Check | Item | Detail |", "|---|---|---|---|"]
    for kind, label, ok, detail in rows:
        md.append(f'| {"PASS" if ok else "**FAIL**"} | {kind} | {label} | {detail} |')
    md.append("")

    g = None
    if chat_set.get("gold") is not None and "error" not in report:
        g = gold_lib.score(chat_set["gold"], memories, trace)
        g["attribution"] = gold_lib.attribute(chat_set["gold"], trace, g["missed"])
        md += render_gold(g)

    titles = {}
    for c, chat in enumerate(chat_set["chats"]):
        titles[f"eval-{c}"] = chat["title"]
    md += ["## Memories", "", "| Text | Category | Type | Old text | Sources |",
           "|---|---|---|---|---|"]
    for m in sorted(memories, key=lambda m: m["created"]):
        sources = ", ".join(sorted({
            f'{titles.get(l["conversation"], l["conversation"])} #{l["entry"].split("-")[2] if l["entry"].count("-") >= 3 else "?"}'
            for l in m["links"]}))
        md.append(f'| {m["text"]} | {m["category"]} | {m["type"]} | '
                  f'{m.get("previous") or ""} | {sources} |')
    if not memories:
        md.append("| (none) | | | | |")
    md.append("")

    groups = _latencies(trace)
    md += ["## Latency of the calls", "", "| Call | Count | Median | Max | Total |",
           "|---|---|---|---|---|"]
    for name, values in sorted(groups.items()):
        md.append(f"| {name} | {len(values)} | {statistics.median(values):.0f} ms | "
                  f"{max(values)} ms | {sum(values) / 1000:.1f} s |")
    md.append("")

    md += ["## Turns", ""]
    by_entry: dict = {}
    order = []
    for step in trace:
        entry = step.get("entry")
        if entry is None:
            continue
        if entry not in by_entry:
            by_entry[entry] = []
            order.append(entry)
        by_entry[entry].append(step)
    loaded = next((s for s in trace if s["step"] == "loaded"), None)
    if loaded:
        md.append(f'Start: {len(loaded["memories"])} memories.\n')
    for entry in order:
        steps = by_entry[entry]
        turn = next((s for s in steps if s["step"] == "turn"), None)
        parts = entry.split("-")
        chat_index = int(parts[1]) if len(parts) > 2 and parts[1].isdigit() else None
        title = chat_set["chats"][chat_index]["title"] if chat_index is not None else entry
        md.append(f'### {title} — turn {parts[2] if len(parts) > 2 else "?"}')
        md.append("")
        if turn:
            md.append(f'> {turn["text"]}')
            md.append("")
        for step in steps:
            if step["step"] == "turn":
                continue
            md += _render_step(step)
        md.append("")

    summary = {
        "set": chat_set["name"], "status": result.get("status", "error"),
        "turns_read": result.get("turns_read", 0),
        "turns_kept": result.get("turns_kept", 0),
        "memories": len(memories), "checks_passed": passed,
        "checks_total": len(rows), "forbidden_hits": forbidden_hits,
        "seconds": round(total_ms / 1000, 1), **extra,
    }
    if g is not None:
        summary.update({
            "gold_required": g["required"], "gold_found": len(g["found"]),
            "recall": g["recall"], "precision": g["precision"],
            "leaks": len(g["leaks"]), "stale_kept": len(g["stale_kept"]),
            "wrong_ops": len(g["wrong_ops"]), "missed_ops": len(g["missed_ops"]),
            "duplicates": len(g["duplicates"]), "label_accuracy": g["label_accuracy"],
            "extras": len(g["extras"]),
            "misses": {fid: a["detail"] for fid, a in g["attribution"].items()}})
    return "\n".join(md) + "\n", summary


def _pct(x) -> str:
    return "n/a" if x is None else f"{x:.2f}"


def render_gold(g: dict) -> list:
    """The gold section of report.md (plan 8.2)."""
    md = ["## Gold", "",
          "| Recall | Precision | Leaks | Stale kept | Wrong ops | Missed ops | Duplicates | Label accuracy | Extras |",
          "|---|---|---|---|---|---|---|---|---|",
          f'| {len(g["found"])}/{g["required"]} ({_pct(g["recall"])}) | '
          f'{g["matched_memories"]}/{g["memories"]} ({_pct(g["precision"])}) | '
          f'{len(g["leaks"])}{" **LEAK**" if g["leaks"] else ""} | {len(g["stale_kept"])} | '
          f'{len(g["wrong_ops"])} | {len(g["missed_ops"])} | {len(g["duplicates"])} | '
          f'{_pct(g["label_accuracy"])} | {len(g["extras"])} |', ""]
    if g["attribution"]:
        md += ["Missed facts, and the first step that lost each one:", "",
               "| Fact | Source turn | Step | Detail |", "|---|---|---|---|"]
        for fid, a in g["attribution"].items():
            md.append(f'| {fid} | {a["turn"]} | {a["step"]} | {a["detail"]} |')
        md.append("")
    for title, items in (("Leaks", [f'{x["id"]}: "{x["text"]}"' for x in g["leaks"]]),
                         ("Stale kept", [f'{x["id"]}: "{x["text"]}"' for x in g["stale_kept"]]),
                         ("Wrong ops", [f'{o["op"]} in {o["turn"]}: "{o["old"]}" → "{o["new"]}" '
                                        f'(gold {o["old_gold"]} → {o["new_gold"]})' for o in g["wrong_ops"]]),
                         ("Missed ops", [str(o) for o in g["missed_ops"]]),
                         ("Duplicates", [f'{k}: {v}' for k, v in g["duplicates"].items()]),
                         ("Label errors", g["label_errors"]),
                         ("Extras (no gold match)", [f'"{t}" from {", ".join(src) or "?"}'
                                                     for t, src in zip(g["extras"], g["extra_sources"])])):
        if items:
            md += [f"{title}:", ""] + [f"- {i}" for i in items] + [""]
    return md


def _spread(values: list, digits: int = 2) -> str:
    v = [x for x in values if x is not None]
    if not v:
        return "n/a"
    mean = sum(v) / len(v)
    if len(v) == 1:
        return f"{mean:.{digits}f}"
    return f"{mean:.{digits}f} [{min(v):.{digits}f}, {max(v):.{digits}f}]"


def render_summary(summaries: list, meta: dict) -> str:
    if any("recall" in s for s in summaries):
        return render_gold_summary(summaries, meta)
    return render_expect_summary(summaries, meta)


def render_gold_summary(summaries: list, meta: dict) -> str:
    """One row for each set: the mean and [min, max] over the repeats."""
    by_set: dict = {}
    for s in summaries:
        by_set.setdefault(s["set"], []).append(s)
    md = ["# Learned memory eval summary", "",
          f'Time: {meta.get("time")}. Decision model `{meta.get("decision")}`, '
          f'local LLM `{meta.get("llm")}`, threshold {meta.get("threshold")}, '
          f'margin {meta.get("margin")}. Values are the mean [min, max] over the repeats.', "",
          "| Set | Runs | Status | Memories | Checks | Recall | Precision | Leaks | Stale | "
          "Wrong ops | Missed ops | Duplicates | Extras | Time (s) |",
          "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for name, runs in by_set.items():
        statuses = sorted({r["status"] for r in runs})
        link = runs[0].get("report", f"{name}/report.md")
        col = lambda k, d=1: _spread([r.get(k) for r in runs], d)
        md.append(f'| [{name}]({link}) | {len(runs)} | {", ".join(statuses)} | {col("memories")} | '
                  f'{_spread([r["checks_passed"] / max(1, r["checks_total"]) for r in runs])} | '
                  f'{col("recall", 2)} | {col("precision", 2)} | {col("leaks")} | {col("stale_kept")} | '
                  f'{col("wrong_ops")} | {col("missed_ops")} | {col("duplicates")} | {col("extras")} | '
                  f'{col("seconds", 0)} |')
    gold_runs = [s for s in summaries if "recall" in s]
    found = sum(s["gold_found"] for s in gold_runs)
    required = sum(s["gold_required"] for s in gold_runs)
    leaks = sum(s["leaks"] for s in gold_runs)
    md += ["", f"Pooled recall over all runs: {sc.fmt_rate(sc.rate(found, required))}. "
           f"Leaks: {leaks}{' (HARD GATE FAILS)' if leaks else ''}.", ""]
    steps: dict = {}
    for s in gold_runs:
        for detail in s.get("misses", {}).values():
            key = detail.split(" ")[0].split("(")[0]
            steps[key] = steps.get(key, 0) + 1
    if steps:
        md += ["Missed facts by the step that lost them (all runs):", "",
               "| Step | Misses |", "|---|---|"]
        md += [f"| {k} | {v} |" for k, v in sorted(steps.items(), key=lambda kv: -kv[1])]
        md += ["", "| Set | Run | Fact | Lost at |", "|---|---|---|---|"]
        for s in gold_runs:
            for fid, detail in s.get("misses", {}).items():
                md.append(f'| {s["set"]} | r{s.get("repeat", 1)} | {fid} | {detail} |')
    return "\n".join(md) + "\n"


def render_expect_summary(summaries: list, meta: dict) -> str:
    md = ["# Learned memory eval summary", "",
          f'Time: {meta.get("time")}. Decision model `{meta.get("decision")}`, '
          f'local LLM `{meta.get("llm")}`, threshold {meta.get("threshold")}, '
          f'margin {meta.get("margin")}.', "",
          "| Set | Status | Turns read | Kept | Memories | Checks | Forbidden hits | Time |",
          "|---|---|---|---|---|---|---|---|"]
    for s in summaries:
        md.append(f'| [{s["set"]}]({s["set"]}/report.md) | {s["status"]} | '
                  f'{s["turns_read"]} | {s["turns_kept"]} | {s["memories"]} | '
                  f'{s["checks_passed"]}/{s["checks_total"]} | {s["forbidden_hits"]} | '
                  f'{s["seconds"]} s |')
    return "\n".join(md) + "\n"
