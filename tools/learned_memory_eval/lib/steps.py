"""The step evals of step_eval.py (plan P2): gate, sentence, relation,
relevance (decision model) and rewrite, relation-llm, merge (local LLM).

Each step loads its cases, sends one request for each case (lib/requests.py,
the exact bytes of the C++ code), and scores the answers with the rules of
plan section 8.1. A record is one case, one variant and one repeat. Its "ok"
is the primary outcome of the case (None when the case is not scored), and
compare tools use it for paired flips.
"""

from __future__ import annotations

import json
import statistics
from pathlib import Path

from . import requests as rq
from . import scoring as sc

EVAL_ROOT = Path(__file__).resolve().parents[1]
DATASETS = EVAL_ROOT / "datasets" / "steps"
UNSAFE = ("sensitive", "instruction")


def read_jsonl(path: Path) -> list:
    return [
        json.loads(line)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]


def answer_status(res: dict) -> str:
    """ok, too_long (a model limit, for example Laya: HTTP 400 "...; limit is 48")
    or error."""
    if res.get("error") is None:
        return "ok"
    if (
        400 <= (res.get("status") or 0) < 500
        and "limit" in str(res["error"]).lower()
    ):
        return "too_long"
    return "error"


def _common(res: dict) -> dict:
    return {
        "status": answer_status(res),
        "latency_ms": res.get("latency_ms"),
        "cached": res.get("cached"),
        "error": res.get("error"),
    }


def _status_counts(records: list) -> dict:
    out: dict = {}
    for r in records:
        out[r["status"]] = out.get(r["status"], 0) + 1
    return out


class Step:
    name = ""
    kind = "dm"  # dm or llm
    primary = ""

    def __init__(self, args):
        self.args = args

    def dataset_files(self, split: str) -> list:
        if getattr(self.args, "cases", None):
            return [Path(self.args.cases)]
        return [DATASETS / self.name.replace("-llm", "") / f"{split}.jsonl"]

    def load(self, split: str) -> list:
        path = self.dataset_files(split)[0]
        if not path.exists():
            raise FileNotFoundError(f"no {split} cases for {self.name}: {path}")
        return read_jsonl(path)

    def params(self) -> dict:
        return {}

    def run(self, case: dict, variant: str, repeat: int, ctx) -> dict:
        raise NotImplementedError

    def score(self, records: list, cases: dict) -> dict:
        raise NotImplementedError

    def render(self, variant: str, m: dict) -> list:
        raise NotImplementedError

    def errors(self, records: list, cases: dict) -> list:
        """Markdown lines for the wrong cases."""
        return []


# ---- decision model steps ---------------------------------------------------
class Gate(Step):
    name = "gate"
    primary = "keep_recall"

    def params(self):
        return {
            "gate_threshold": self.args.gate_threshold,
            "gate_thresholds": self.args.gate_thresholds,
        }

    def run(self, case, variant, repeat, ctx):
        res = ctx.dm.ask(
            ctx.model, rq.gate_body(ctx.model, case["text"], variant), repeat
        )
        p = (
            rq.parse_noul(res["answers"], "gate")
            if res.get("answers")
            else None
        )
        rec = {"p": p, **_common(res)}
        if res.get("answers") and p is None:
            rec["status"] = "error"
        t = self.args.gate_threshold
        if case["gold"] == "keep":
            rec["ok"] = p is not None and p >= t
        elif case["gold"] == "skip":
            rec["ok"] = None if p is None else p < t
        else:
            rec["ok"] = None
        return rec

    def score(self, records, cases):
        t = self.args.gate_threshold
        keep = [r for r in records if cases[r["id"]]["gold"] == "keep"]
        skip = [r for r in records if cases[r["id"]]["gold"] == "skip"]
        sweep = []
        for th in sorted(set(self.args.gate_thresholds) | {t}):
            sweep.append(
                {
                    "threshold": th,
                    "keep": sc.rate(
                        sum(
                            1
                            for r in keep
                            if r["p"] is not None and r["p"] >= th
                        ),
                        len(keep),
                    ),
                    "wrong_keeps": sum(
                        1 for r in skip if r["p"] is not None and r["p"] >= th
                    ),
                }
            )
        keep_p = [r["p"] for r in keep if r["p"] is not None]
        skip_p = [r["p"] for r in skip if r["p"] is not None]
        recall = sc.rate(sum(1 for r in keep if r["ok"]), len(keep))
        return {
            "keep_recall": recall,
            "skip_rate": sc.rate(sum(1 for r in skip if r["ok"]), len(skip)),
            "hard_gates": {
                "keep_recall>=0.98": recall["p"] is not None
                and recall["p"] >= 0.98
            },
            "keep_min_p": min(keep_p) if keep_p else None,
            "skip_max_p": max(skip_p) if skip_p else None,
            "sweep": sweep,
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        md = [
            f"| Keep recall @ {self.args.gate_threshold} | Skip rate | Keep min p | Skip max p | "
            "Hard gate (recall >= 0.98) | Status | Latency |",
            "|---|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['keep_recall'])} | {sc.fmt_rate(m['skip_rate'])} | "
            f"{_f(m['keep_min_p'])} | {_f(m['skip_max_p'])} | "
            f"{'pass' if m['hard_gates']['keep_recall>=0.98'] else '**FAIL**'} | "
            f"{_status(m['status'])} | {sc.fmt_latency(m['latency'])} |",
            "",
            "| Threshold | Keep turns kept | Task turns kept (wrong) |",
            "|---|---|---|",
        ]
        for s in m["sweep"]:
            md.append(
                f"| {s['threshold']} | {sc.fmt_rate(s['keep'], False)} | {s['wrong_keeps']} |"
            )
        return md

    def errors(self, records, cases):
        out = []
        for r in records:
            if r["ok"] is False:
                c = cases[r["id"]]
                out.append(
                    f"- `{r['id']}` gold {c['gold']}, p {_f(r['p'])} ({r['status']}): "
                    f"{c['text'][:110]}"
                )
        return out


def keep_rule(
    x: dict, fact_t: float, max_sens: float, max_instr: float, max_short: float
):
    """DreamingRun::OnSentenceDecisions(): returns (keep, drop reason)."""
    if x["fact"] < fact_t:
        return False, "no fact"
    for answer, limit in (
        ("sensitive", max_sens),
        ("instruction", max_instr),
        ("short_lived", max_short),
    ):
        if x["safety"].get(answer, 0.0) >= limit:
            return False, answer
    return True, ""


def _argmax(probs: dict, order: list) -> str:
    """The first answer with the highest probability, in |order|."""
    return max(order, key=lambda k: probs.get(k, -1))


class Sentence(Step):
    name = "sentence"
    primary = "keep_recall"

    def params(self):
        a = self.args
        return {
            "fact_threshold": a.fact_threshold,
            "max_sensitive": a.max_sensitive,
            "max_instruction": a.max_instruction,
            "max_short_lived": a.max_short_lived,
            "certain_threshold": a.threshold,
            "certain_margin": a.margin,
            "grid_fact": a.grid_fact,
            "grid_sensitive": a.grid_sensitive,
        }

    def run(self, case, variant, repeat, ctx):
        res = ctx.dm.ask(
            ctx.model,
            rq.sentence_body(ctx.model, case["text"], variant),
            repeat,
        )
        x = (
            rq.parse_sentence(res["answers"], variant)
            if res.get("answers")
            else None
        )
        rec = {"probs": x, **_common(res)}
        if res.get("answers") and x is None:
            rec["status"] = "error"
        if x is None:
            rec.update(
                kept=None,
                reason=None,
                ok=False if case["gold"]["keep"] else None,
            )
            return rec
        a = self.args
        kept, reason = keep_rule(
            x,
            a.fact_threshold,
            a.max_sensitive,
            a.max_instruction,
            a.max_short_lived,
        )
        cat = (
            rq.certain(x["category"], a.threshold, a.margin, "category")
            or "personal_fact"
        )
        temp = rq.certain(
            {True: x["temporary"], False: 1 - x["temporary"]},
            a.threshold,
            a.margin,
            "bool",
        )
        rec.update(
            kept=kept,
            reason=reason,
            label_category=cat,
            label_type="short_term" if temp is True else "long_term",
            ok=kept == case["gold"]["keep"],
        )
        return rec

    def score(self, records, cases):
        a = self.args
        rows = [(cases[r["id"]]["gold"], r) for r in records]
        scored = [(g, r) for g, r in rows if r["probs"] is not None]
        want = [(g, r) for g, r in rows if g["keep"]]
        kept_ok = sum(1 for g, r in want if r["kept"])
        wrong = [(g, r) for g, r in scored if r["kept"] and not g["keep"]]
        unsafe = [(g, r) for g, r in wrong if g["safety"] in UNSAFE]
        drops: dict = {}
        for g, r in want:
            if not r["kept"]:
                key = r["reason"] or r["status"]
                drops[key] = drops.get(key, 0) + 1
        wrong_by: dict = {}
        for g, _ in wrong:
            wrong_by[g["safety"]] = wrong_by.get(g["safety"], 0) + 1
        fact_pred = [(g["fact"], r["probs"]["fact"] > 0.5) for g, r in scored]
        tp = sum(1 for gold, pred in fact_pred if gold and pred)
        should = [(g, r) for g, r in scored if g["keep"]]
        sens = [(g, r) for g, r in scored if g["safety"] == "sensitive"]
        grid = []
        for ft in a.grid_fact:
            for ms in a.grid_sensitive:
                k = w = u = 0
                for g, r in scored:
                    keep, _ = keep_rule(
                        r["probs"], ft, ms, a.max_instruction, a.max_short_lived
                    )
                    k += keep and g["keep"]
                    w += keep and not g["keep"]
                    u += keep and g["safety"] in UNSAFE
                grid.append(
                    {
                        "fact": ft,
                        "sensitive": ms,
                        "keep": sc.rate(k, len(want)),
                        "wrong_keeps": w,
                        "unsafe_keeps": u,
                    }
                )
        return {
            "keep_recall": sc.rate(kept_ok, len(want)),
            "unsafe_keeps": len(unsafe),
            "hard_gates": {"unsafe_keeps==0": not unsafe},
            "wrong_keeps": len(wrong),
            "wrong_keeps_by_gold_safety": wrong_by,
            "false_drops_by_reason": drops,
            "fact_accuracy": sc.rate(
                sum(1 for gold, pred in fact_pred if gold == pred),
                len(fact_pred),
            ),
            "fact_precision": sc.rate(
                tp, sum(1 for _, pred in fact_pred if pred)
            ),
            "fact_recall": sc.rate(tp, sum(1 for gold, _ in fact_pred if gold)),
            "safety_accuracy": sc.rate(
                sum(
                    1
                    for g, r in scored
                    if _argmax(r["probs"]["safety"], rq.SAFETY_NAMES)
                    == g["safety"]
                ),
                len(scored),
            ),
            "sensitive_caught": sc.rate(
                sum(
                    1
                    for g, r in sens
                    if r["probs"]["safety"].get("sensitive", 0)
                    >= a.max_sensitive
                ),
                len(sens),
            ),
            "category_accuracy": sc.rate(
                sum(
                    1
                    for g, r in should
                    if _argmax(r["probs"]["category"], rq.CATEGORY_NAMES)
                    == g["category"]
                ),
                len(should),
            ),
            "category_label_accuracy": sc.rate(
                sum(
                    1 for g, r in should if r["label_category"] == g["category"]
                ),
                len(should),
            ),
            "temporary_accuracy": sc.rate(
                sum(
                    1
                    for g, r in should
                    if (r["probs"]["temporary"] > 0.5) == g["temporary"]
                ),
                len(should),
            ),
            "type_label_accuracy": sc.rate(
                sum(
                    1
                    for g, r in should
                    if (r["label_type"] == "short_term") == g["temporary"]
                ),
                len(should),
            ),
            "grid": grid,
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        a = self.args
        md = [
            f"Keep rule: fact >= {a.fact_threshold}, sensitive < {a.max_sensitive}, "
            f"instruction < {a.max_instruction}, short_lived < {a.max_short_lived}.",
            "",
            "| Keep recall | Unsafe keeps (hard gate 0) | Wrong keeps | False drops by reason | Status | Latency |",
            "|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['keep_recall'])} | {m['unsafe_keeps']}"
            f"{'' if m['hard_gates']['unsafe_keeps==0'] else ' **FAIL**'} | {m['wrong_keeps']} "
            f"{_dict(m['wrong_keeps_by_gold_safety'])} | {_dict(m['false_drops_by_reason'])} | "
            f"{_status(m['status'])} | {sc.fmt_latency(m['latency'])} |",
            "",
            "| Question | Metric | Value |",
            "|---|---|---|",
            f"| fact | accuracy (yes > 0.5) | {sc.fmt_rate(m['fact_accuracy'])} |",
            f"| fact | precision / recall | {sc.fmt_rate(m['fact_precision'], False)} / {sc.fmt_rate(m['fact_recall'], False)} |",
            f"| safety | accuracy (top answer) | {sc.fmt_rate(m['safety_accuracy'])} |",
            f"| safety | sensitive caught (>= {a.max_sensitive}) | {sc.fmt_rate(m['sensitive_caught'])} |",
            f"| category | accuracy (top answer, kept cases) | {sc.fmt_rate(m['category_accuracy'])} |",
            f"| category | label that the pipeline stores | {sc.fmt_rate(m['category_label_accuracy'])} |",
            f"| temporary | accuracy (yes > 0.5, kept cases) | {sc.fmt_rate(m['temporary_accuracy'])} |",
            f"| temporary | type that the pipeline stores | {sc.fmt_rate(m['type_label_accuracy'])} |",
            "",
            "Keep-rule grid (same answers; instruction and short_lived limits unchanged):",
            "",
            "| Fact >= | Sensitive < | Keep recall | Wrong keeps | Unsafe keeps |",
            "|---|---|---|---|---|",
        ]
        for g in m["grid"]:
            md.append(
                f"| {g['fact']} | {g['sensitive']} | {sc.fmt_rate(g['keep'], False)} | "
                f"{g['wrong_keeps']} | {g['unsafe_keeps']} |"
            )
        return md

    def errors(self, records, cases):
        out = []
        for r in records:
            c = cases[r["id"]]
            if r["ok"] is not False:
                continue
            x = r["probs"]
            probs = (
                "no answer"
                if x is None
                else (
                    f"fact {x['fact']:.2f}, "
                    + ", ".join(
                        f"{k} {v:.2f}"
                        for k, v in sorted(
                            x["safety"].items(), key=lambda kv: -kv[1]
                        )[:2]
                    )
                )
            )
            what = (
                "missed"
                if c["gold"]["keep"]
                else f"wrong keep [{c['gold']['safety']}]"
            )
            out.append(
                f"- `{r['id']}` {what} ({r['reason'] or r['status']}; {probs}): {c['text']}"
            )
        return out


RELATION_LABELS = rq.RELATION_NAMES


def can_change_text(new_type: str | None, old_type: str | None) -> bool:
    """CanChangeText() of dreaming_run.cc. Unknown types count as long-term."""
    return (new_type or "long_term") != "short_term" and (
        old_type or "long_term"
    ) == "long_term"


class Relation(Step):
    name = "relation"
    primary = "certain_accuracy"

    def params(self):
        return {
            "certain_threshold": self.args.threshold,
            "certain_margin": self.args.margin,
        }

    def run(self, case, variant, repeat, ctx):
        body = rq.relation_body(ctx.model, case["new"], case["old"], variant)
        res = ctx.dm.ask(ctx.model, body, repeat)
        probs = (
            rq.parse_choice(res["answers"], "relation", RELATION_LABELS)
            if res.get("answers")
            else None
        )
        rec = {"probs": probs, **_common(res)}
        if probs is None:
            rec.update(
                status="error" if rec["status"] == "ok" else rec["status"],
                answer=None,
                top=None,
                certain=False,
                applied=None,
                ok=None,
                costly=False,
            )
            return rec
        answer = rq.certain(
            probs, self.args.threshold, self.args.margin, "relation"
        )
        top = _argmax(probs, RELATION_LABELS)
        # The code applies a certain answer. same_topic counts as different, and
        # the types can block a text change.
        applied = answer
        if answer == "same_topic":
            applied = "different"
        if answer in ("replace", "merge") and not can_change_text(
            case.get("new_type"), case.get("old_type")
        ):
            applied = "different"
        correct = answer is not None and (
            answer in case["acceptable"] or applied in case["acceptable"]
        )
        rec.update(
            answer=answer,
            top=top,
            certain=answer is not None,
            applied=applied,
            ok=correct if answer is not None else None,
            costly=answer is not None and applied in case["costly"],
        )
        return rec

    def score(self, records, cases):
        certain = [r for r in records if r["certain"]]
        confusion: dict = {}
        for r in records:
            g = cases[r["id"]]["gold"]
            col = r["answer"] or ("not_certain" if r["probs"] else r["status"])
            row = confusion.setdefault(g, {})
            row[col] = row.get(col, 0) + 1
        return {
            "certain_accuracy": sc.rate(
                sum(1 for r in certain if r["ok"]), len(certain)
            ),
            "certain_rate": sc.rate(len(certain), len(records)),
            "costly_errors": sum(1 for r in records if r["costly"]),
            "tie_break_rate": sc.rate(
                sum(1 for r in records if r["probs"] and not r["certain"]),
                len(records),
            ),
            "top_accuracy": sc.rate(
                sum(
                    1
                    for r in records
                    if r["top"] in cases[r["id"]]["acceptable"]
                ),
                len(records),
            ),
            "hard_gates": {"costly_errors<=baseline": None},
            "confusion": confusion,
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        a = self.args
        md = [
            f"Certain rule: top >= {a.threshold} and {a.margin} above the next answer.",
            "",
            "| Accuracy on certain answers | Certain rate | Costly errors (applied) | To the LLM tie-break | Top answer accuracy | Status | Latency |",
            "|---|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['certain_accuracy'])} | {sc.fmt_rate(m['certain_rate'], False)} | "
            f"{m['costly_errors']} | {sc.fmt_rate(m['tie_break_rate'], False)} | "
            f"{sc.fmt_rate(m['top_accuracy'], False)} | {_status(m['status'])} | "
            f"{sc.fmt_latency(m['latency'])} |",
            "",
        ]
        md += _confusion(m["confusion"], RELATION_LABELS + ["not_certain"])
        return md

    def errors(self, records, cases):
        out = []
        for r in records:
            c = cases[r["id"]]
            if r["ok"] is False or r["costly"]:
                probs = ", ".join(
                    f"{k} {v:.2f}"
                    for k, v in sorted(
                        r["probs"].items(), key=lambda kv: -kv[1]
                    )[:3]
                )
                out.append(
                    f"- `{r['id']}` gold {c['gold']}, certain {r['answer']}"
                    f"{' (COSTLY)' if r['costly'] else ''} ({probs}): new \"{c['new']}\" / old \"{c['old']}\""
                )
        return out


class Relevance(Step):
    name = "relevance"
    primary = "must_recall"

    def __init__(self, args):
        super().__init__(args)
        self.memories = {
            m["id"]: m["text"]
            for m in read_jsonl(DATASETS / "relevance" / "memories.jsonl")
        }

    def params(self):
        return {
            "relevance_threshold": self.args.relevance_threshold,
            "max_turn_memories": self.args.max_turn_memories,
            "thresholds": self.args.thresholds,
        }

    def select(self, probs: dict, threshold: float, cap: int | None) -> list:
        """TurnMemoryLookup: keep yes >= threshold, best |cap| first."""
        kept = sorted(
            (
                (p, mid)
                for mid, p in probs.items()
                if p is not None and p >= threshold
            ),
            key=lambda x: -x[0],
        )
        return [mid for _, mid in (kept[:cap] if cap else kept)]

    def run(self, case, variant, repeat, ctx):
        texts = [self.memories[m] for m in case["memories"]]
        body = rq.relevance_body(
            ctx.model,
            case["message"],
            case.get("previous_message", ""),
            texts,
            variant,
        )
        res = ctx.dm.ask(ctx.model, body, repeat)
        probs = None
        if res.get("answers"):
            probs = {
                m: rq.parse_noul(res["answers"], f"m{i}")
                for i, m in enumerate(case["memories"])
            }
        rec = {"probs": probs, **_common(res)}
        if probs is None:
            rec["ok"] = False if case["must"] else None
            return rec
        sel = set(
            self.select(
                probs,
                self.args.relevance_threshold,
                self.args.max_turn_memories,
            )
        )
        rec["selected"] = sorted(sel)
        rec["ok"] = set(case["must"]) <= sel and not (
            set(case["forbidden"]) & sel
        )
        return rec

    def _at(self, records, cases, threshold, cap):
        found = total = bad = bad_total = kept = 0
        for r in records:
            c = cases[r["id"]]
            total += len(c["must"])
            bad_total += len(c["forbidden"])
            if r["probs"] is None:
                continue
            sel = set(self.select(r["probs"], threshold, cap))
            found += len(set(c["must"]) & sel)
            bad += len(set(c["forbidden"]) & sel)
            kept += len(sel)
        return {
            "threshold": threshold,
            "cap": cap,
            "must": sc.rate(found, total),
            "forbidden": sc.rate(bad, bad_total),
            "mean_kept": kept / max(1, len(records)),
        }

    def score(self, records, cases):
        a = self.args
        main = self._at(
            records, cases, a.relevance_threshold, a.max_turn_memories
        )
        return {
            "must_recall": main["must"],
            "forbidden_rate": main["forbidden"],
            "mean_kept": main["mean_kept"],
            "hard_gates": {"forbidden_rate<=baseline": None},
            "sweep": [self._at(records, cases, t, None) for t in a.thresholds],
            "sweep_capped": [
                self._at(records, cases, t, a.max_turn_memories)
                for t in a.thresholds
            ],
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        a = self.args
        md = [
            f"Pipeline: yes >= {a.relevance_threshold}, best {a.max_turn_memories}.",
            "",
            "| Must recall | Forbidden rate | Mean kept | Status | Latency |",
            "|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['must_recall'])} | {sc.fmt_rate(m['forbidden_rate'], False)} | "
            f"{m['mean_kept']:.1f} | {_status(m['status'])} | {sc.fmt_latency(m['latency'])} |",
            "",
            "| Threshold | Must found (no cap) | Forbidden found | Mean kept | "
            f"Must found (best {a.max_turn_memories}) | Forbidden | Mean kept |",
            "|---|---|---|---|---|---|---|",
        ]
        for s, c in zip(m["sweep"], m["sweep_capped"]):
            md.append(
                f"| {s['threshold']} | {sc.fmt_rate(s['must'], False)} | {s['forbidden']['k']} | "
                f"{s['mean_kept']:.1f} | {sc.fmt_rate(c['must'], False)} | {c['forbidden']['k']} | "
                f"{c['mean_kept']:.1f} |"
            )
        return md

    def errors(self, records, cases):
        out = []
        for r in records:
            c = cases[r["id"]]
            if r["ok"] is not False or r["probs"] is None:
                continue
            miss = [m for m in c["must"] if m not in r["selected"]]
            bad = [m for m in c["forbidden"] if m in r["selected"]]
            parts = [
                f"missed {self.memories[m]!r} {r['probs'][m]:.2f}" for m in miss
            ]
            parts += [
                f"forbidden {self.memories[m]!r} {r['probs'][m]:.2f}"
                for m in bad
            ]
            out.append(f"- `{r['id']}` {c['message']!r}: " + "; ".join(parts))
        return out


# ---- local LLM steps --------------------------------------------------------
def _ymd(date: str) -> tuple:
    y, m, d = (int(x) for x in date.split("-"))
    return (y, m, d)


class Rewrite(Step):
    name = "rewrite"
    kind = "llm"
    primary = "key_fact_rate"

    def run(self, case, variant, repeat, ctx):
        sentences = case["sentences"]
        system, user = rq.rewrite_messages(
            sentences, case["turn_date"], variant
        )
        res = ctx.llm.complete(
            ctx.model,
            rq.chat_body(
                ctx.model, system, user, rq.llm_extra("rewrite", variant)
            ),
            repeat,
        )
        parsed = (
            rq.parse_rewrite_answer(res["content"] or "", len(sentences))
            if res.get("content") is not None
            else None
        )
        rec = {"content": res.get("content"), **_common(res)}
        facts = []
        for text, sources in parsed or []:
            facts.append(
                {
                    "text": text,
                    "sources": sources,
                    "no_new_details": rq.is_faithful_rewrite(
                        text,
                        [sentences[i] for i in sources],
                        [_ymd(case["turn_date"])],
                    ),
                    "too_long": rq.utf16_length(text)
                    > rq.MAX_MEMORY_TEXT_LENGTH,
                }
            )
        used = [f for f in facts if not f["too_long"]]
        covered = {i for f in used for i in f["sources"]}
        joined = " ".join(f["text"] for f in used)
        gold = case["gold"]
        key = [sc.all_words(g["must"], joined) for g in gold["facts"]]
        bad = sorted(
            {
                str(w)
                for g in gold["facts"]
                for w in sc.any_words(g.get("must_not", []), joined)
            }
        )
        stored = (
            joined
            + " "
            + " ".join(s for i, s in enumerate(sentences) if i not in covered)
        )
        rec.update(
            parsed=parsed is not None,
            facts=facts,
            key_facts=key,
            must_not_hits=bad,
            uncovered=[i for i in range(len(sentences)) if i not in covered],
            pipeline_key=all(
                sc.all_words(g["must"], stored) for g in gold["facts"]
            ),
            date_ok=None
            if "date" not in gold
            else sc.word_matches(gold["date"], joined),
            too_many=None
            if "max_facts" not in gold
            else len(used) > gold["max_facts"],
            ok=parsed is not None and all(key) and not bad,
        )
        return rec

    def score(self, records, cases):
        facts = [f for r in records for f in r.get("facts", [])]
        gold_facts = [k for r in records for k in r.get("key_facts", [])]
        sentences = sum(len(cases[r["id"]]["sentences"]) for r in records)
        dated = [r for r in records if r.get("date_ok") is not None]
        parsed = sc.rate(
            sum(1 for r in records if r.get("parsed")), len(records)
        )
        bad = sum(1 for r in records if r.get("must_not_hits"))
        return {
            "key_fact_rate": sc.rate(
                sum(1 for r in records if r["ok"]), len(records)
            ),
            "gold_fact_rate": sc.rate(sum(gold_facts), len(gold_facts)),
            "pipeline_key_fact_rate": sc.rate(
                sum(1 for r in records if r.get("pipeline_key")), len(records)
            ),
            "json_valid": parsed,
            "must_not_cases": bad,
            "hard_gates": {
                "json_valid>=0.99": parsed["p"] is not None
                and parsed["p"] >= 0.99,
                "must_not==0": bad == 0,
            },
            "no_new_details": sc.rate(
                sum(1 for f in facts if f["no_new_details"]), len(facts)
            ),
            "too_long": sum(1 for f in facts if f["too_long"]),
            "facts_per_sentence": len(facts) / max(1, sentences),
            "uncovered_sentences": sum(
                len(r.get("uncovered", [])) for r in records
            ),
            "date_correct": sc.rate(
                sum(1 for r in dated if r["date_ok"]), len(dated)
            ),
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        hg = m["hard_gates"]
        return [
            "| Key-fact rate (cases) | Gold facts found | JSON valid (hard gate >= 0.99) | must_not cases (hard gate 0) | "
            "No new details (facts) | Facts per sentence | Uncovered sentences | Status | Latency |",
            "|---|---|---|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['key_fact_rate'])} | {sc.fmt_rate(m['gold_fact_rate'], False)} | "
            f"{sc.fmt_rate(m['json_valid'], False)}{'' if hg['json_valid>=0.99'] else ' **FAIL**'} | "
            f"{m['must_not_cases']}{'' if hg['must_not==0'] else ' **FAIL**'} | "
            f"{sc.fmt_rate(m['no_new_details'], False)} | {m['facts_per_sentence']:.2f} | "
            f"{m['uncovered_sentences']} | {_status(m['status'])} | {sc.fmt_latency(m['latency'])} |",
            "",
            f"With the C++ fallback (an uncovered sentence is stored as the user wrote it), the key-fact "
            f"rate is {sc.fmt_rate(m['pipeline_key_fact_rate'], False)}. Too long facts: {m['too_long']}. "
            f"Date correct: {sc.fmt_rate(m['date_correct'], False)}.",
        ]

    def errors(self, records, cases):
        out = []
        for r in records:
            if r["ok"] is not False:
                continue
            c = cases[r["id"]]
            facts = "; ".join(
                f"\"{f['text']}\" {f['sources']}" for f in r.get("facts", [])
            ) or (f"(not parsed: {str(r['content'])[:80]!r})")
            miss = [
                g["canonical"]
                for g, k in zip(c["gold"]["facts"], r.get("key_facts", []))
                if not k
            ]
            out.append(
                f"- `{r['id']}` r{r['repeat']}: {c['sentences']} → {facts}"
                + (f" — missing: {miss}" if miss else "")
                + (
                    f" — must_not: {r['must_not_hits']}"
                    if r.get("must_not_hits")
                    else ""
                )
            )
        return out


class RelationLlm(Step):
    name = "relation-llm"
    kind = "llm"
    primary = "accuracy"

    def run(self, case, variant, repeat, ctx):
        system, user = rq.relation_llm_messages(
            case["new"], case["old"], variant
        )
        res = ctx.llm.complete(
            ctx.model,
            rq.chat_body(
                ctx.model, system, user, rq.llm_extra("relation-llm", variant)
            ),
            repeat,
        )
        answer = (
            rq.parse_relation_answer(res["content"] or "")
            if res.get("content") is not None
            else None
        )
        # The code uses only same and different from the LLM: replace, merge
        # and same_topic count as different (both memories stay).
        applied = (
            None
            if answer is None
            else ("same" if answer == "same" else "different")
        )
        return {
            "content": res.get("content"),
            **_common(res),
            "answer": answer,
            "applied": applied,
            "ok": answer in case["acceptable"],
            "costly": answer in case["costly"],
        }

    def score(self, records, cases):
        confusion: dict = {}
        for r in records:
            g = cases[r["id"]]["gold"]
            col = r["answer"] or "invalid"
            row = confusion.setdefault(g, {})
            row[col] = row.get(col, 0) + 1
        applied_ok = sum(
            1
            for r in records
            if r["applied"]
            and (
                r["applied"] in cases[r["id"]]["acceptable"]
                or (
                    r["applied"] == "different"
                    and cases[r["id"]]["gold"] == "same_topic"
                )
            )
        )
        return {
            "accuracy": sc.rate(
                sum(1 for r in records if r["ok"]), len(records)
            ),
            "applied_accuracy": sc.rate(applied_ok, len(records)),
            "costly_answers": sum(1 for r in records if r["costly"]),
            "invalid": sum(1 for r in records if r["answer"] is None),
            "hard_gates": {},
            "confusion": confusion,
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        md = [
            "| Accuracy | Accuracy of what the code applies | Costly answers (ignored by the code) | Invalid | Status | Latency |",
            "|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['accuracy'])} | {sc.fmt_rate(m['applied_accuracy'], False)} | "
            f"{m['costly_answers']} | {m['invalid']} | {_status(m['status'])} | "
            f"{sc.fmt_latency(m['latency'])} |",
            "",
        ]
        md += _confusion(m["confusion"], RELATION_LABELS + ["invalid"])
        return md

    def errors(self, records, cases):
        return [
            f"- `{r['id']}` r{r['repeat']}: gold {cases[r['id']]['gold']}, LLM {r['answer']}: "
            f"new \"{cases[r['id']]['new']}\" / old \"{cases[r['id']]['old']}\""
            for r in records
            if not r["ok"]
        ]


class Merge(Step):
    name = "merge"
    kind = "llm"
    primary = "both_facts_rate"

    def run(self, case, variant, repeat, ctx):
        system, user = rq.merge_messages(case["old"], case["new"], variant)
        res = ctx.llm.complete(
            ctx.model,
            rq.chat_body(
                ctx.model, system, user, rq.llm_extra("merge", variant)
            ),
            repeat,
        )
        merged = (
            rq.parse_merge_answer(res["content"] or "")
            if res.get("content") is not None
            else None
        )
        rec = {"content": res.get("content"), **_common(res), "text": merged}
        gold = case["gold"]
        if merged is None:
            rec.update(
                ok=False,
                both=False,
                must_not_hits=[],
                no_new_details=None,
                too_long=None,
            )
            return rec
        bad = [str(w) for w in sc.any_words(gold.get("must_not", []), merged)]
        both = sc.all_words(gold["must"], merged)
        too_long = rq.utf16_length(merged) > rq.MAX_MEMORY_TEXT_LENGTH
        rec.update(
            both=both,
            must_not_hits=bad,
            too_long=too_long,
            no_new_details=rq.is_faithful_rewrite(
                merged, [case["old"], case["new"]], []
            ),
            ok=both and not bad and not too_long,
        )
        return rec

    def score(self, records, cases):
        valid = [r for r in records if r["text"] is not None]
        bad = sum(1 for r in records if r["must_not_hits"])
        return {
            "both_facts_rate": sc.rate(
                sum(1 for r in records if r["ok"]), len(records)
            ),
            "valid": sc.rate(len(valid), len(records)),
            "must_not_cases": bad,
            "hard_gates": {"must_not==0": bad == 0},
            "no_new_details": sc.rate(
                sum(1 for r in valid if r["no_new_details"]), len(valid)
            ),
            "too_long": sum(1 for r in valid if r["too_long"]),
            "mean_length": statistics.mean(len(r["text"]) for r in valid)
            if valid
            else None,
            "status": _status_counts(records),
            "latency": sc.latency(
                [r["latency_ms"] for r in records if not r["cached"]]
            ),
        }

    def render(self, variant, m):
        return [
            "| Both facts kept | Valid | must_not cases (hard gate 0) | No new details | Too long | Mean length | Status | Latency |",
            "|---|---|---|---|---|---|---|---|",
            f"| {sc.fmt_rate(m['both_facts_rate'])} | {sc.fmt_rate(m['valid'], False)} | "
            f"{m['must_not_cases']}{'' if m['hard_gates']['must_not==0'] else ' **FAIL**'} | "
            f"{sc.fmt_rate(m['no_new_details'], False)} | {m['too_long']} | "
            f"{_f(m['mean_length'], 0)} chars | {_status(m['status'])} | {sc.fmt_latency(m['latency'])} |",
        ]

    def errors(self, records, cases):
        return [
            f"- `{r['id']}` r{r['repeat']}: \"{cases[r['id']]['old']}\" + \"{cases[r['id']]['new']}\" → "
            f"{r['text']!r}"
            + (f" must_not {r['must_not_hits']}" if r["must_not_hits"] else "")
            for r in records
            if not r["ok"]
        ]


STEPS = {
    s.name: s
    for s in (Gate, Sentence, Relation, Relevance, Rewrite, RelationLlm, Merge)
}


# ---- small formatters -------------------------------------------------------
def _f(x, digits: int = 2) -> str:
    return "n/a" if x is None else f"{x:.{digits}f}"


def _dict(d: dict) -> str:
    return (
        "(" + ", ".join(f"{k} {v}" for k, v in sorted(d.items())) + ")"
        if d
        else ""
    )


def _status(d: dict) -> str:
    return ", ".join(f"{k} {v}" for k, v in sorted(d.items()))


def _confusion(confusion: dict, columns: list) -> list:
    cols = [c for c in columns if any(c in row for row in confusion.values())]
    cols += sorted({c for row in confusion.values() for c in row} - set(cols))
    md = [
        "| Gold \\ answer | " + " | ".join(cols) + " |",
        "|---|" + "---|" * len(cols),
    ]
    for gold in [c for c in columns if c in confusion] + sorted(
        set(confusion) - set(columns)
    ):
        md.append(
            f"| {gold} | "
            + " | ".join(str(confusion[gold].get(c, "")) for c in cols)
            + " |"
        )
    return md


# ---- extra numbers for the LLM steps (X11, X1) -------------------------------
EXAMPLE_WORDS = ("berlin", "violin")  # the examples of the prompt style rules
SUBJECT_PREFIX = ("the user", "user ")  # the style rules say: no subject


def llm_extra_metrics(step: str, records: list, cases: dict) -> dict:
    """Numbers that need all repeats of a case:
    same_across_repeats  cases whose repeats gave the same text (X11)
    example_copies       outputs that contain the example of the prompt, and the
                         input does not (X1)"""
    by_id: dict = {}
    for r in records:
        by_id.setdefault(r["id"], []).append(r)
    multi = [rs for rs in by_id.values() if len(rs) > 1]
    same = sum(1 for rs in multi if len({x.get("content") for x in rs}) == 1)
    copies = 0
    for r in records:
        case = cases[r["id"]]
        source = " ".join(
            case.get("sentences") or [case.get("old", ""), case.get("new", "")]
        ).lower()
        if step == "rewrite":
            texts = [f["text"] for f in r.get("facts", [])]
        elif step == "merge":
            texts = [r["text"]] if r.get("text") else []
        else:
            texts = []
        if any(
            w in t.lower() and w not in source
            for t in texts
            for w in EXAMPLE_WORDS
        ):
            copies += 1
    out = {"same_across_repeats": sc.rate(same, len(multi))}
    if step in ("rewrite", "merge"):
        out["example_copies"] = copies
        texts = (
            [f["text"] for r in records for f in r.get("facts", [])]
            if step == "rewrite"
            else [r["text"] for r in records if r.get("text")]
        )
        with_subject = sum(
            1 for t in texts if t.strip().lower().startswith(SUBJECT_PREFIX)
        )
        out["subject_prefix"] = sc.rate(with_subject, len(texts))
    return out
