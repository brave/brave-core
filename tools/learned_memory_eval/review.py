#!/usr/bin/env python3
"""The review queue of a label panel batch (plan 7.4, step 6).

The queue has two kinds of items:
  privacy  the two reviewers disagree, and one label is sensitive or
           instruction. The case is not in the dataset yet.
  audit    a random accepted case (20 per batch). It is in the dataset.

  ./review.py --quick                 # ONE short session over all open batches (see below)
  ./review.py <batch>                 # answer in the terminal
  ./review.py <batch> --list          # print the queue, answer nothing
  ./review.py <batch> --answers FILE  # apply answers from a JSONL file:
                                      #   {"id": ..., "decision": "1" | "2" | "y" | "n" | "e", "label": ...}

Keys. Privacy: 1 or 2 choose a label, e enter another label, n drop the case,
s skip. Audit: y the label is right, e enter the right label, n drop the case,
s skip.

Quick review (--quick): a person does not answer whole queues. One session asks
every privacy dispute, and a small stratified sample of the audit cases of all
open batches (--n, default 12: about the same number for each step, dev and
holdout mixed), plus the judge checks (--judge-n, default 5) of the judge
calibration, when there is one. The audit passes with at most 1 wrong case. A
wrong case is fixed in the dataset at once. A step with a wrong case needs a
bigger sample: ./review.py --quick --step <step> --n 10. A sample of 12 cases
cannot show a small error rate (0 wrong of 12 still allows about 20% wrong);
it finds a rubric that is broken.

When every item has an answer, the script writes review_result.json, checks
the audit (it passes with at most 1 wrong case of 20), and adds a changelog
line to datasets/DATASETS.md with a new version.
"""

from __future__ import annotations

import argparse
import datetime
import json
import random
import re
import sys
from pathlib import Path

from lib import clients
from lib import panel as P

HERE = Path(__file__).resolve().parent
DATASETS = HERE / "datasets"


def read_jsonl(path: Path) -> list:
    return (
        [json.loads(l) for l in path.read_text().splitlines() if l.strip()]
        if path.exists()
        else []
    )


def write_jsonl(path: Path, rows: list) -> None:
    path.write_text(
        "".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows)
    )


def show(q: dict) -> str:
    if q["kind"] == "judge":
        lines = [f"[judge] {q['id']} ({q['step']})"]
        for k, v in q["input"].items():
            lines.append(f"  {k}: {v}")
        lines.append(
            f"  judge verdict: {json.dumps(q['verdict'], ensure_ascii=False)}"
        )
        return "\n".join(lines)
    if q["step"] == "relevance" and q["kind"] == "audit":
        row = q["row"]
        lines = [f"[audit] {q['id']} (relevance, {q['split']})"]
        if row.get("previous_message"):
            lines.append(f"  previous message: {row['previous_message']}")
        lines.append(f"  message: {row['message']}")
        lines.append("  must (the assistant needs these):")
        lines += [f"    - {q['texts'][i]}" for i in row["must"]] or [
            "    (none)"
        ]
        lines.append("  forbidden (not relevant, tempting):")
        lines += [f"    - {q['texts'][i]}" for i in row["forbidden"]] or [
            "    (none)"
        ]
        lines.append(f"  tags: {', '.join(row['tags'])}")
        return "\n".join(lines)
    lines = [f"[{q['kind']}] {q['id']} ({q['step']}, {q['split']})"]
    case = (
        q["case"]
        if q["kind"] == "privacy"
        else P.case_input(q["step"], q["row"])
    )
    for k, v in case.items():
        lines.append(f"  {k}: {v}")
    if q["kind"] == "privacy":
        for n, opt in enumerate(q["options"], 1):
            label = {k: v for k, v in opt.items() if k != "reason"}
            lines.append(f"  {n}) {json.dumps(label, ensure_ascii=False)}")
            lines.append(f"     reason: {opt['reason']}")
    else:
        gold = {
            k: q["row"][k]
            for k in ("gold", "acceptable", "costly", "canonical")
            if k in q["row"]
        }
        lines.append(f"  label: {json.dumps(gold, ensure_ascii=False)}")
        lines.append(f"  tags: {', '.join(q['row']['tags'])}")
    return "\n".join(lines)


def label_from(step: str, value) -> dict:
    """A label that the reviewer entered, in the form of lib/panel.label_of()."""
    if step == "relevance":
        return {
            "must": list(value.get("must", [])),
            "forbidden": list(value.get("forbidden", [])),
        }
    if step == "relation":
        if isinstance(value, str):
            value = {"label": value, "acceptable": [value]}
        return {
            "label": value["label"],
            "acceptable": value.get("acceptable", [value["label"]]),
        }
    if isinstance(value, dict) and "label" in value and step in ("gate",):
        value = value["label"]
    return {"label": value}


def apply(q: dict, decision: str, label, folder: Path) -> str | None:
    """Applies one answer to the dataset. Returns an error text or None."""
    step, split = q["step"], q["split"]
    path = DATASETS / "steps" / step / f"{split}.jsonl"
    rows = read_jsonl(path)
    origin = f"generated:{q['batch']}"
    if step == "relevance":
        index = next(
            (i for i, r in enumerate(rows) if r["id"] == q["id"]), None
        )
        if index is None:
            return f"{q['id']} is not in {path.name}"
        if decision == "n":
            rows.pop(index)
        elif decision == "e":
            chosen = label_from(step, label)
            pool = set(rows[index]["memories"])
            if (set(chosen["must"]) | set(chosen["forbidden"])) - pool or set(
                chosen["must"]
            ) & set(chosen["forbidden"]):
                return (
                    "must and forbidden need ids of the pool, and no id in both"
                )
            rows[index] = {
                **rows[index],
                **chosen,
                "tags": list(
                    dict.fromkeys(rows[index]["tags"] + ["audit_fixed"])
                ),
            }
        elif decision not in ("y", "s"):
            return f"bad decision {decision!r}"
        write_jsonl(path, rows)
        return None
    if q["kind"] == "privacy":
        if decision in ("n", "s"):
            return None
        if decision in ("1", "2"):
            chosen = {
                k: v
                for k, v in q["options"][int(decision) - 1].items()
                if k != "reason"
            }
        elif decision == "e":
            chosen = label_from(step, label)
        else:
            return f"bad decision {decision!r}"
        bad = P.check_label(step, chosen["label"], chosen)
        if bad:
            return bad
        case = {
            "id": q["id"],
            "slot_tag": q.get("slot_tag", ""),
            "lang": q.get("lang", "en"),
            "tags": q.get("tags", []),
            **q["case"],
        }
        rows.append(
            P.to_row(step, split, case, chosen, origin, ["privacy_reviewed"])
        )
    else:
        index = next(
            (i for i, r in enumerate(rows) if r["id"] == q["id"]), None
        )
        if index is None:
            return f"{q['id']} is not in {path.name}"
        if decision == "n":
            rows.pop(index)
        elif decision == "e":
            chosen = label_from(step, label)
            bad = P.check_label(step, chosen["label"], chosen)
            if bad:
                return bad
            old = rows[index]
            case = {
                "id": old["id"],
                "slot_tag": old["tags"][0] if old["tags"] else "",
                "lang": next(
                    (t[5:] for t in old["tags"] if t.startswith("lang:")), "en"
                ),
                "tags": [
                    t
                    for t in old["tags"]
                    if not t.startswith(("safety:", "category:"))
                    and t != "temporary"
                ]
                + ["audit_fixed"],
                **P.case_input(step, old),
            }
            rows[index] = P.to_row(step, split, case, chosen, origin, [])
        elif decision not in ("y", "s"):
            return f"bad decision {decision!r}"
    write_jsonl(path, rows)
    return None


def finish(folder: Path, queue: list, decisions: dict) -> None:
    batch = json.loads((folder / "batch.json").read_text())
    audits = [q for q in queue if q["kind"] == "audit"]
    wrong = sum(
        1 for q in audits if decisions[q["id"]]["decision"] in ("n", "e")
    )
    privacy = [q for q in queue if q["kind"] == "privacy"]
    kept = sum(
        1 for q in privacy if decisions[q["id"]]["decision"] in ("1", "2", "e")
    )
    result = {
        "finished": datetime.datetime.now().isoformat(timespec="seconds"),
        "audit_n": len(audits),
        "audit_wrong": wrong,
        "audit_pass": wrong <= max(1, len(audits) // 20),
        "privacy_n": len(privacy),
        "privacy_kept": kept,
    }
    (folder / "review_result.json").write_text(json.dumps(result, indent=2))
    s, m = batch["stats"], batch["models"]
    card = DATASETS / "DATASETS.md"
    text = card.read_text()
    version = int(re.search(r"^Version:\s*(\d+)", text, re.M).group(1)) + 1
    text = re.sub(
        r"^Version:\s*\d+", f"Version: {version}", text, count=1, flags=re.M
    )
    line = (
        f"| {version} | {result['finished'][:10]} | Batch `{batch['batch']}` ({batch['step']} "
        f"{batch['split']}, rubric v{batch['rubric_version']}): {s['accepted'] + kept} cases added "
        f"({s['adjudicated']} settled by the judge, {kept} privacy disputes decided by the reviewer), "
        f"{s['ambiguous']} ambiguous. Agreement {s['agree']}/{s['compared']} ({s['agreement']:.0%}). "
        f"Audit: {wrong} wrong of {len(audits)} ({'pass' if result['audit_pass'] else 'FAIL'}). "
        f"Agents: G {m['generate']['model']} ({m['generate']['thinking']}), R {m['review']['model']} "
        f"({m['review']['thinking']}), J {m['judge']['model']} ({m['judge']['thinking']}) |"
    )
    card.write_text(text.rstrip("\n") + "\n" + line + "\n")
    print(
        f"\nReview done. Audit {wrong}/{len(audits)} wrong: {'pass' if result['audit_pass'] else 'FAIL'}. "
        f"DATASETS.md version {version}."
    )
    if not result["audit_pass"]:
        print(
            "The audit failed: fix the rubric, run the panel again and draw a new sample (plan 7.4, rule 6)."
        )


# ---- quick review -----------------------------------------------------------
STEP_ORDER = [
    "rewrite",
    "merge",
    "sentence",
    "relation",
    "relevance",
    "gate",
]  # the most lenient checks first


def open_batches(root: Path, step: str | None) -> list:
    """The batch folders that have a queue and are not reviewed (or only sampled)."""
    out = []
    for q in sorted(root.glob("*/review_queue.jsonl")):
        result = q.parent / "review_result.json"
        if result.exists() and not json.loads(result.read_text()).get("quick"):
            continue
        queue = read_jsonl(q)
        if queue and (step is None or queue[0]["step"] == step):
            out.append((q.parent, queue))
    return out


def sample_audits(batches: list, decided: set, n: int, seed: str) -> list:
    """About n audit items, the same number for each step, dev and holdout mixed."""
    rng = random.Random(seed)
    pools: dict = {}
    for _, queue in batches:
        for q in queue:
            if q["kind"] == "audit" and q["id"] not in decided:
                pools.setdefault(q["step"], {}).setdefault(
                    q["split"], []
                ).append(q)
    steps = [s for s in STEP_ORDER if s in pools]
    if not steps:
        return []
    quota = {s: n // len(steps) for s in steps}
    for s in steps[: n % len(steps)]:
        quota[s] += 1
    picked = []
    for step in steps:
        lists = [rng.sample(v, len(v)) for _, v in sorted(pools[step].items())]
        while len([x for x in picked if x["step"] == step]) < quota[
            step
        ] and any(lists):
            for lst in lists:
                if (
                    lst
                    and len([x for x in picked if x["step"] == step])
                    < quota[step]
                ):
                    picked.append(lst.pop())
    return picked


def ask(q: dict, n: int, total: int) -> tuple:
    """Asks one item in the terminal. Returns (decision, label)."""
    print(f"\n({n}/{total}) " + show(q))
    if q["kind"] == "judge":
        keys = "y/n/s"
    else:
        keys = "1/2/e/n/s" if q["kind"] == "privacy" else "y/e/n/s"
    decision = input(f"  [{keys}] > ").strip().lower()
    label = None
    if decision == "e":
        print(
            f"  Enter the label as JSON, in this form: "
            + (
                '{"must": [ids], "forbidden": [ids]}'
                if q["step"] == "relevance"
                else P.JUDGE_FORMAT[q["step"]]
            )
        )
        try:
            entered = json.loads(input("  label > "))
        except json.JSONDecodeError as e:
            print(f"  not JSON ({e}); skipped")
            return "s", None
        label = (
            entered
            if q["step"] in ("relation", "relevance")
            else entered.get("label", entered)
        )
    return decision, label


def quick(args) -> None:
    root = clients.results_root(args.results) / "panel"
    batches = open_batches(root, args.step)
    dec_path = root / "quick_decisions.jsonl"
    decisions = {d["id"]: d for d in read_jsonl(dec_path)}
    items = [
        q
        for _, queue in batches
        for q in queue
        if q["kind"] == "privacy"
        and (q["id"] not in decisions or decisions[q["id"]]["decision"] == "s")
    ]
    # Items of an earlier, unfinished session come first.
    by_id = {q["id"]: q for _, queue in batches for q in queue}
    items += [
        by_id[i]
        for i, d in decisions.items()
        if d["decision"] == "s" and i in by_id and by_id[i]["kind"] == "audit"
    ]
    # The sample is kept in quick_session.json, so that a stopped session goes on with the same items.
    session_path = root / "quick_session.json"
    session = (
        json.loads(session_path.read_text()) if session_path.exists() else None
    )
    if (
        session is not None
        and not args.step
        and any(
            i not in decisions or decisions[i]["decision"] == "s"
            for i in session["ids"]
        )
    ):
        calibration = sorted(
            (clients.results_root(args.results) / "judge_calibration").glob(
                "*/judge_check.jsonl"
            )
        )
        pool = {q["id"]: q for q in items}
        pool.update(by_id)
        pool.update(
            {c["id"]: c for path in calibration for c in read_jsonl(path)}
        )
        items = [
            pool[i]
            for i in session["ids"]
            if i in pool
            and (i not in decisions or decisions[i]["decision"] == "s")
        ]
    else:
        seed = f"quick-{len(decisions)}"
        items += sample_audits(batches, set(decisions), args.n, seed)
        calibration = sorted(
            (clients.results_root(args.results) / "judge_calibration").glob(
                "*/judge_check.jsonl"
            )
        )
        if calibration and not args.step:
            checks = [
                c
                for c in read_jsonl(calibration[-1])
                if c["id"] not in decisions
            ][: args.judge_n]
            items += checks
        if items and not args.step:
            session_path.write_text(
                json.dumps(
                    {
                        "ids": [q["id"] for q in items]
                        + [
                            i
                            for i in (session or {}).get("ids", [])
                            if i in decisions
                        ]
                    },
                    indent=2,
                )
            )
    if args.list:
        for q in items:
            print(show(q) + "\n")
        print(f"{len(items)} items in this session.")
        return
    if not items:
        sys.exit("nothing to review")
    print(
        f"{len(items)} items: {sum(1 for q in items if q['kind'] == 'privacy')} privacy, "
        f"{sum(1 for q in items if q['kind'] == 'audit')} audit, "
        f"{sum(1 for q in items if q['kind'] == 'judge')} judge checks. Keys: y right, n wrong, "
        "e enter the right label, 1/2 choose a label, s skip."
    )
    done = []
    for n, q in enumerate(items, 1):
        if args.answers:
            given = {a["id"]: a for a in read_jsonl(Path(args.answers))}
            if q["id"] not in given:
                continue
            decision, label = (
                str(given[q["id"]]["decision"]),
                given[q["id"]].get("label"),
            )
        else:
            decision, label = ask(q, n, len(items))
        if q["kind"] != "judge":
            error = apply(q, decision, label, root)
            if error:
                print(f"  {q['id']}: {error}; not applied")
                continue
        decisions[q["id"]] = {
            "id": q["id"],
            "kind": q["kind"],
            "step": q["step"],
            "decision": decision,
            "label": label,
            "by": args.reviewer,
        }
        write_jsonl(dec_path, list(decisions.values()))
        done.append(q)
    # The result counts every answer of the session, also the ones from an earlier stop.
    ids = [
        i
        for i in (
            json.loads(session_path.read_text())["ids"]
            if session_path.exists()
            else []
        )
        if i in decisions
    ] or [q["id"] for q in done]
    answered = [decisions[i] for i in ids if decisions[i]["decision"] != "s"]
    audits = [d for d in answered if d["kind"] == "audit"]
    wrong = [d for d in audits if d["decision"] in ("n", "e")]
    judges = [d for d in answered if d["kind"] == "judge"]
    judge_ok = [d for d in judges if d["decision"] == "y"]
    privacy = [d for d in answered if d["kind"] == "privacy"]
    result = {
        "finished": datetime.datetime.now().isoformat(timespec="seconds"),
        "quick": True,
        "audit_n": len(audits),
        "audit_wrong": len(wrong),
        "audit_pass": len(wrong) <= 1 and len(audits) >= 1,
        "wrong_by_step": {
            s: sum(1 for d in wrong if d["step"] == s)
            for s in sorted({d["step"] for d in wrong})
        },
        "privacy_n": len(privacy),
        "judge_n": len(judges),
        "judge_agree": len(judge_ok),
    }
    for folder, queue in batches:
        batch_ids = {q["id"] for q in queue}
        result_batch = {
            **result,
            "audit_n": sum(1 for d in audits if d["id"] in batch_ids),
            "audit_wrong": sum(1 for d in wrong if d["id"] in batch_ids),
        }
        (folder / "review_result.json").write_text(
            json.dumps(result_batch, indent=2)
        )
    card = DATASETS / "DATASETS.md"
    text = card.read_text()
    version = int(re.search(r"^Version:\s*(\d+)", text, re.M).group(1)) + 1
    text = re.sub(
        r"^Version:\s*\d+", f"Version: {version}", text, count=1, flags=re.M
    )
    people = sum(1 for d in answered if d.get("by", "human") == "human")
    who = (
        "a human"
        if people == len(answered)
        else (
            f"a human ({people} items) and an agent called {args.reviewer} ({len(answered) - people} items; "
            "this part is NOT a human audit)"
        )
    )
    line = (
        f"| {version} | {result['finished'][:10]} | Quick review by {who} of {len(batches)} open batches (a "
        f"stratified sample, plan 7.4 had 20 cases for each batch): audit {len(wrong)} wrong of {len(audits)} "
        f"({'pass' if result['audit_pass'] else 'FAIL'}; wrong by step: {result['wrong_by_step'] or 'none'}), "
        f"{len(privacy)} privacy disputes decided, judge checks {len(judge_ok)} of {len(judges)} agree. "
        "Wrong cases were fixed or dropped in the dataset |"
    )
    card.write_text(text.rstrip("\n") + "\n" + line + "\n")
    print(
        f"\nDone. Audit {len(wrong)}/{len(audits)} wrong: {'pass' if result['audit_pass'] else 'FAIL'}. "
        f"Judge checks: {len(judge_ok)}/{len(judges)} agree. DATASETS.md version {version}."
    )
    if not result["audit_pass"]:
        print(
            "The audit failed. Look at the steps with wrong cases, fix their rubric, and run a bigger "
            "sample of that step: ./review.py --quick --step <step> --n 10"
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("batch", nargs="?")
    parser.add_argument(
        "--quick",
        action="store_true",
        help="one short session over all open batches",
    )
    parser.add_argument(
        "--n",
        type=int,
        default=12,
        help="with --quick: audit cases in the sample",
    )
    parser.add_argument(
        "--judge-n", type=int, default=5, help="with --quick: judge checks"
    )
    parser.add_argument(
        "--step", default=None, help="with --quick: only this step"
    )
    parser.add_argument(
        "--reviewer",
        default="human",
        help="with --quick: who answers. \"human\" is a person; an agent must use its own name",
    )
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--answers", default=None)
    parser.add_argument("--results", default=None)
    args = parser.parse_args()
    if args.quick:
        quick(args)
        return
    if not args.batch:
        parser.error("give a batch, or --quick")
    folder = clients.results_root(args.results) / "panel" / args.batch
    queue = read_jsonl(folder / "review_queue.jsonl")
    if not queue:
        sys.exit(
            f"no review queue in {folder} (run label_panel.py finish first)"
        )
    dec_path = folder / "review_decisions.jsonl"
    decisions = {d["id"]: d for d in read_jsonl(dec_path)}
    if args.list:
        for q in queue:
            done = decisions.get(q["id"])
            print(
                show(q)
                + (f"\n  -> answered: {done['decision']}" if done else "")
                + "\n"
            )
        print(f"{len(decisions)} of {len(queue)} answered.")
        return
    todo = [
        q
        for q in queue
        if q["id"] not in decisions or decisions[q["id"]]["decision"] == "s"
    ]
    given = (
        {a["id"]: a for a in read_jsonl(Path(args.answers))}
        if args.answers
        else None
    )
    for n, q in enumerate(todo, 1):
        if given is not None:
            if q["id"] not in given:
                continue
            decision, label = (
                str(given[q["id"]]["decision"]),
                given[q["id"]].get("label"),
            )
        else:
            print(f"\n({n}/{len(todo)}) " + show(q))
            keys = "1/2/e/n/s" if q["kind"] == "privacy" else "y/e/n/s"
            decision = input(f"  [{keys}] > ").strip().lower()
            label = None
            if decision == "e":
                print(
                    f"  Enter the label as JSON, in this form: {P.JUDGE_FORMAT[q['step']]}"
                )
                try:
                    entered = json.loads(input("  label > "))
                except json.JSONDecodeError as e:
                    print(f"  not JSON ({e}); skipped")
                    continue
                label = (
                    entered.get("label", entered)
                    if q["step"] != "relation"
                    else entered
                )
        error = apply(q, decision, label, folder)
        if error:
            print(f"  {q['id']}: {error}; not applied")
            continue
        decisions[q["id"]] = {
            "id": q["id"],
            "decision": decision,
            "label": label,
        }
        write_jsonl(dec_path, list(decisions.values()))
    if all(
        q["id"] in decisions and decisions[q["id"]]["decision"] != "s"
        for q in queue
    ):
        if not (folder / "review_result.json").exists():
            finish(folder, queue, decisions)
    else:
        left = sum(
            1
            for q in queue
            if q["id"] not in decisions or decisions[q["id"]]["decision"] == "s"
        )
        print(
            f"\n{left} items are left. Run ./review.py {args.batch} again to answer them."
        )


if __name__ == "__main__":
    main()
