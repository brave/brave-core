#!/usr/bin/env python3
"""Makes new labeled cases with a panel of separate agents (plan 7.4).

  generate (agent G, Opus)  one case for each coverage slot, with a label
  review   (agent R, Sonnet) a blind label from the rubric and the case text
                             (for rewrite and merge: a check of the gold)
  compare  (script)          agree and clear → accept; agree, not clear →
                             ambiguous; a privacy dispute → the reviewer; other
                             disagreements → agent J (Opus)
  finish   (script)          accepted cases to the dataset, ambiguous cases to
                             datasets/ambiguous/, the review queue for the reviewer
                             (privacy disputes and 20 random accepted cases)

  ./label_panel.py run sentence --split dev --count 60     # all four passes
  ./label_panel.py generate gate --split holdout --count 40
  ./label_panel.py review <batch>
  ./label_panel.py compare <batch>
  ./label_panel.py finish <batch>
  ./label_panel.py status [<batch>]

Then the reviewer answers the queue: ./review.py <batch>.
Batches are in <results root>/panel/<batch>/, agent tasks in
<results root>/agent_tasks/. A pass that has valid answers is not run again.
"""
from __future__ import annotations

import argparse
import datetime
import json
import random
import re
import subprocess
import sys
from pathlib import Path

import agent_run
from lib import agent_tasks, clients
from lib import panel as P

HERE = Path(__file__).resolve().parent
DATASETS = HERE / "datasets"
CHUNK = {"generate": 30, "review": 40, "judge": 30}
MIN_AGREEMENT = 0.8
MAX_PRIVACY = 30
AUDIT_SIZE = 5  # plan 7.4 had 20; a person reviews a small sample (review.py --quick)


def rubric(step: str) -> tuple:
    text = (DATASETS / "rubrics" / f"{step}.md").read_text()
    m = re.search(r"^Rubric version:\s*(\S+)", text, re.M)
    return text, m.group(1) if m else "?"


def read_jsonl(path: Path) -> list:
    return [json.loads(l) for l in path.read_text().splitlines() if l.strip()] if path.exists() else []


def write_jsonl(path: Path, rows: list) -> None:
    path.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows))


def panel_root(args) -> Path:
    return clients.results_root(getattr(args, "results", None)) / "panel"


def load_batch(args, name: str) -> tuple:
    folder = panel_root(args) / name
    if not (folder / "batch.json").exists():
        sys.exit(f"no batch {name} in {panel_root(args)}")
    return folder, json.loads((folder / "batch.json").read_text())


def save_batch(folder: Path, batch: dict) -> None:
    (folder / "batch.json").write_text(json.dumps(batch, indent=2, ensure_ascii=False))


def models(args) -> dict:
    out = {}
    for role in ("generate", "review", "judge"):
        spec, thinking = agent_run.DEFAULT_MODELS[role]
        out[role] = {"model": getattr(args, f"model_{role}", None) or spec,
                     "thinking": getattr(args, f"thinking_{role}", None) or thinking}
    return out


def run_agents(args, batch: dict, pass_name: str, folders: list, validate) -> dict:
    m = batch["models"][pass_name]
    print(f"{pass_name}: {len(folders)} tasks, {m['model']} ({m['thinking']})", flush=True)
    results = agent_run.run_tasks(folders, m["model"], m["thinking"], validate, pass_name,
                                  parallel=args.parallel, agent_cmd=args.agent_cmd)
    answers, failed = {}, []
    for folder, (by_id, errors) in results.items():
        answers.update(by_id)
        if errors:
            failed.append(f"{folder.name}: {errors[:3]}")
    if failed:
        print(f"WARNING: tasks with errors after all tries (their bad items are left out): {failed}")
    return answers


def chunks(items: list, size: int) -> list:
    return [items[i:i + size] for i in range(0, len(items), size)]


def existing_keys(step: str) -> set:
    keys = set()
    for split in ("dev", "holdout"):
        for row in read_jsonl(DATASETS / "steps" / step / f"{split}.jsonl"):
            keys.add(P.dedupe_key(step, row))
    return keys


def next_number(args, step: str, split: str) -> int:
    pattern = re.compile(rf"^{re.escape(step)}-{split}-g(\d+)$")
    seen = [0]
    for path in [DATASETS / "steps" / step / f"{split}.jsonl", DATASETS / "ambiguous" / f"{step}.jsonl"]:
        seen += [int(m.group(1)) for r in read_jsonl(path) if (m := pattern.match(r.get("id", "")))]
    root = panel_root(args)
    if root.exists():
        for b in root.glob(f"{step}-{split}-b*/batch.json"):
            seen.append(json.loads(b.read_text()).get("last_number", 0))
    return max(seen) + 1


# ---- task texts -------------------------------------------------------------
WHAT = {
    "gate": 'Write "text": one full user turn (one message to the assistant).',
    "sentence": 'Write "text": one sentence, as it can be one sentence of a chat message.',
    "relation": 'Write "old" and "new": two memories in the style that the feature stores, a short fact in '
                'the third person without a subject ("Lives in Berlin", "Has a dog named Luna"). The old '
                'memory is stored. The new memory comes from a later chat.',
    "rewrite": 'Write "sentences": 1 to 4 sentences that the user wrote in one turn, as the feature keeps '
               'them (facts about the user, no questions). Use the "turn_date" of the slot.',
    "merge": 'Write "old" and "new": a stored memory and a new memory that adds to it, in the style of the '
             'feature (short facts in the third person without a subject).',
}


def generate_task(step: str) -> str:
    text, _ = rubric(step)
    tags = ", ".join(sorted({t for t, _ in P.SLOTS[step]}))
    return f"""# Agent task: write test cases (step: {step})

You write test cases for one step of a memory feature of an AI assistant. The
feature reads the chats of a user and stores short facts about the user. The
rubric below says how to label a case. The cases and labels test the feature,
so the labels must follow the rubric exactly.

## Rubric

{text}

## What to write

items.jsonl has one slot on each line: "id", "persona" (an invented person;
write as this person), "tag" and "note" (what the case must be), "lang" (the
language of the user text, "en" is English){', "turn_date"' if step == 'rewrite' else ''}.
Write one case for each slot.

- {WHAT[step]}
- All text is invented. Do not use real people, real user data, or private
  data of famous people.
- Write as users really type: short or medium, natural, sometimes informal.
  Vary the words and the form. Do not start every case the same way.
- Give the label that the rubric gives. Set "clear" to false when the rubric
  does not give one clear label.
- "reason": one short sentence: why this label.
- "tags": coverage tags that fit the case, from this list: {tags}. The list
  can be empty.

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each slot, with the "id" of the slot:

{P.FORMAT[step]}

Write valid JSON (double quotes; true, false and null). Write nothing else in
the file.
"""


def review_task(step: str) -> str:
    text, _ = rubric(step)
    if step in P.CHECK_MODE:
        what = """Each line has a case and "proposed": a proposed gold. Check it against the
rubric and the case text:

- Each "must" item is in the input, or is a correct form of it, and the
  meaning needs it. The "must" items cover each idea of the input.
- Each "must_not" item is a real risk, and a correct answer does not have it.
- "canonical" follows the style rules. A date (if there is one) is the
  correct exact date.

Answer "ok" when the gold is correct. Answer "fix" with the corrected gold in
"fixed" when it is wrong. A small style change in "canonical" is not a reason
to fix."""
        title = "check the gold of test cases"
    else:
        what = """Each line has one case. Give the label that the rubric gives. Label each case
alone. Set "clear" to false when the rubric does not give one clear label.
"reason": one short sentence."""
        title = "label test cases"
    return f"""# Agent task: {title} (step: {step})

You review test cases for one step of a memory feature of an AI assistant.
Use only the rubric below.

## Rubric

{text}

## What to do

{what}

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each case, with the "id" of the case:

{P.REVIEW_FORMAT[step]}

Write valid JSON. Write nothing else in the file.
"""


def judge_task(step: str) -> str:
    text, _ = rubric(step)
    return f"""# Agent task: settle disagreements (step: {step})

Two reviewers labeled each case with the rubric below, and they disagree.
Each line has the case, and "a" and "b": the two labels with their reasons.

## Rubric

{text}

## What to do

Decide the label that the rubric gives. You can choose the label of a, of b,
or another label. Set "clear" to false when the rubric does not give one clear
label. "reason": one or two short sentences.

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each case, with the "id" of the case:

{P.JUDGE_FORMAT[step]}

Write valid JSON. Write nothing else in the file.
"""


# ---- passes -----------------------------------------------------------------
def cmd_generate(args) -> str:
    step, split = args.step, args.split
    root = panel_root(args)
    n = 1 + max([0] + [int(m.group(1)) for p in (root.glob(f"{step}-{split}-b*") if root.exists() else [])
                       if (m := re.search(r"-b(\d+)$", p.name))])
    name = f"{step}-{split}-b{n}"
    folder = root / name
    folder.mkdir(parents=True)
    first = next_number(args, step, split)
    slots = P.make_slots(step, split, args.count, first, seed=name,
                         only=args.only.split(",") if args.only else None)
    _, version = rubric(step)
    batch = {"batch": name, "step": step, "split": split, "count": args.count,
             "created": datetime.datetime.now().isoformat(timespec="seconds"),
             "rubric_version": version, "models": models(args),
             "last_number": first + args.count - 1, "tasks": {}}
    write_jsonl(folder / "slots.jsonl", slots)
    save_batch(folder, batch)
    generate_pass(args, folder, batch)
    return name


def generate_pass(args, folder: Path, batch: dict) -> None:
    step = batch["step"]
    slots = read_jsonl(folder / "slots.jsonl")
    tasks = []
    for k, part in enumerate(chunks(slots, CHUNK["generate"]), 1):
        task = agent_tasks.task_root(clients.results_root(args.results)) / f"{batch['batch']}-G{k}"
        agent_tasks.write_task(task, generate_task(step), part)
        tasks.append(task)
    batch["tasks"]["generate"] = [t.name for t in tasks]
    save_batch(folder, batch)
    answers = run_agents(args, batch, "generate", tasks, P.validate_generate(step))
    seen = existing_keys(step)
    cases, dropped = [], []
    for slot in slots:
        a = answers.get(slot["id"])
        if a is None:
            dropped.append({"id": slot["id"], "why": "no valid answer"})
            continue
        case = {"id": slot["id"], "slot_tag": slot["tag"], "lang": slot["lang"], **{
            k: v for k, v in a.items() if k not in ("id",)}}
        key = P.dedupe_key(step, case)
        if key in seen:
            dropped.append({"id": slot["id"], "why": "duplicate text"})
            continue
        seen.add(key)
        cases.append(case)
    write_jsonl(folder / "generated.jsonl", cases)
    batch["stats"] = {"generated": len(cases), "dropped": dropped}
    save_batch(folder, batch)
    print(f"generate: {len(cases)} cases, {len(dropped)} dropped", flush=True)


def cmd_review(args) -> None:
    folder, batch = load_batch(args, args.batch)
    step = batch["step"]
    cases = read_jsonl(folder / "generated.jsonl")
    items = []
    for c in cases:
        item = {"id": c["id"], **P.case_input(step, c)}
        if step in P.CHECK_MODE:
            item["proposed"] = c["label"]
        items.append(item)
    tasks = []
    for k, part in enumerate(chunks(items, CHUNK["review"]), 1):
        task = agent_tasks.task_root(clients.results_root(args.results)) / f"{batch['batch']}-R{k}"
        agent_tasks.write_task(task, review_task(step), part)
        tasks.append(task)
    batch["tasks"]["review"] = [t.name for t in tasks]
    save_batch(folder, batch)
    answers = run_agents(args, batch, "review", tasks, P.validate_review(step))
    write_jsonl(folder / "reviewed.jsonl", [answers[c["id"]] for c in cases if c["id"] in answers])
    print(f"review: {len(answers)} of {len(cases)} cases", flush=True)


def cmd_compare(args) -> None:
    folder, batch = load_batch(args, args.batch)
    step = batch["step"]
    cases = {c["id"]: c for c in read_jsonl(folder / "generated.jsonl")}
    reviews = {r["id"]: r for r in read_jsonl(folder / "reviewed.jsonl")}
    rows, to_judge = [], []
    rng = random.Random(batch["batch"])
    for cid, c in cases.items():
        r = reviews.get(cid)
        if r is None:
            rows.append({"id": cid, "outcome": "no_review"})
            continue
        g_lab, r_lab = P.label_of(step, c), P.label_of(step, r)
        outcome = P.compare(step, g_lab, r_lab)
        row = {"id": cid, "outcome": outcome, "g": g_lab, "r": r_lab,
               "g_reason": c["reason"], "r_reason": r["reason"],
               "clear": bool(c["clear"] and r["clear"])}
        rows.append(row)
        if outcome == "disagree":
            mine = {**g_lab, "reason": c["reason"]}
            theirs = ({"label": r_lab["label"] if r_lab.get("label") is not None else "(see reason)",
                       "reason": r["reason"]} if step in P.CHECK_MODE else {**r_lab, "reason": r["reason"]})
            a, b = (mine, theirs) if rng.random() < 0.5 else (theirs, mine)
            to_judge.append({"id": cid, **P.case_input(step, c), "a": a, "b": b})
    write_jsonl(folder / "compare.jsonl", rows)
    judged = {}
    if to_judge:
        tasks = []
        for k, part in enumerate(chunks(to_judge, CHUNK["judge"]), 1):
            task = agent_tasks.task_root(clients.results_root(args.results)) / f"{batch['batch']}-J{k}"
            agent_tasks.write_task(task, judge_task(step), part)
            tasks.append(task)
        batch["tasks"]["judge"] = [t.name for t in tasks]
        save_batch(folder, batch)
        judged = run_agents(args, batch, "judge", tasks, P.validate_judge(step))
    write_jsonl(folder / "judged.jsonl", list(judged.values()))
    compared = [r for r in rows if r["outcome"] != "no_review"]
    agree = sum(1 for r in compared if r["outcome"] == "agree")
    batch["stats"].update({
        "compared": len(compared), "agree": agree,
        "agreement": round(agree / len(compared), 3) if compared else None,
        "agree_unclear": sum(1 for r in compared if r["outcome"] == "agree" and not r["clear"]),
        "privacy": sum(1 for r in compared if r["outcome"] == "privacy"),
        "disagree": len(to_judge), "judged_clear": sum(1 for j in judged.values() if j["clear"])})
    save_batch(folder, batch)
    s = batch["stats"]
    print(f"compare: agreement {agree}/{len(compared)} ({s['agreement']}), {s['agree_unclear']} agreed "
          f"but not clear, {s['privacy']} privacy disputes, {s['disagree']} to the judge "
          f"({s['judged_clear']} settled)", flush=True)


def cmd_finish(args) -> None:
    folder, batch = load_batch(args, args.batch)
    step, split, s = batch["step"], batch["split"], batch["stats"]
    if batch.get("finished"):
        sys.exit(f"{batch['batch']} is finished already")
    problems = []
    if s.get("agreement") is not None and s["agreement"] < MIN_AGREEMENT:
        problems.append(f"agreement {s['agreement']} is below {MIN_AGREEMENT}: the rubric is not clear. "
                        "Fix the rubric, raise its version and run the batch again (plan 7.4, rule 4)")
    if s.get("privacy", 0) > MAX_PRIVACY:
        problems.append(f"{s['privacy']} privacy disputes (more than {MAX_PRIVACY}): the rubric is not clear")
    if problems and not args.force:
        sys.exit("Not finished:\n- " + "\n- ".join(problems) + "\n(--force writes the cases anyway)")
    cases = {c["id"]: c for c in read_jsonl(folder / "generated.jsonl")}
    judged = {j["id"]: j for j in read_jsonl(folder / "judged.jsonl")}
    origin = f"generated:{batch['batch']}"
    accepted, ambiguous, queue = [], [], []
    for row in read_jsonl(folder / "compare.jsonl"):
        c = cases[row["id"]]
        base = {"id": row["id"], "split": split, "step": step, "batch": batch["batch"],
                "case": P.case_input(step, c)}
        if row["outcome"] == "agree" and row["clear"]:
            accepted.append(P.to_row(step, split, c, P.final_label(step, row["g"], row["r"]), origin, []))
        elif row["outcome"] == "agree":
            ambiguous.append({**base, "why": "agreed, not clear", "labels": {"g": row["g"], "r": row["r"]},
                              "reasons": {"g": row["g_reason"], "r": row["r_reason"]}})
        elif row["outcome"] == "privacy":
            queue.append({**base, "kind": "privacy", "slot_tag": c["slot_tag"], "lang": c.get("lang", "en"),
                          "tags": c.get("tags", []),
                          "options": [{**row["g"], "reason": row["g_reason"]},
                                      {**row["r"], "reason": row["r_reason"]}]})
        elif row["outcome"] == "disagree":
            j = judged.get(row["id"])
            if j and j["clear"]:
                accepted.append(P.to_row(step, split, c, P.label_of(step, j), origin, ["adjudicated"]))
            else:
                ambiguous.append({**base, "why": "judge not clear" if j else "no judge answer",
                                  "labels": {"g": row["g"], "r": row["r"], "j": P.label_of(step, j) if j else None},
                                  "reasons": {"g": row["g_reason"], "r": row["r_reason"],
                                              "j": j["reason"] if j else None}})
    rng = random.Random(batch["batch"] + "-audit")
    for row in rng.sample(accepted, min(args.audit, len(accepted))):
        queue.append({"kind": "audit", "id": row["id"], "step": step, "split": split,
                      "batch": batch["batch"], "row": row})
    dataset = DATASETS / "steps" / step / f"{split}.jsonl"
    write_jsonl(dataset, read_jsonl(dataset) + accepted)
    amb_path = DATASETS / "ambiguous" / f"{step}.jsonl"
    amb_path.parent.mkdir(exist_ok=True)
    write_jsonl(amb_path, read_jsonl(amb_path) + ambiguous)
    write_jsonl(folder / "review_queue.jsonl", queue)
    batch.update(finished=datetime.datetime.now().isoformat(timespec="seconds"))
    s.update({"accepted": len(accepted), "ambiguous": len(ambiguous),
              "adjudicated": sum(1 for r in accepted if "adjudicated" in r["tags"]),
              "queue_privacy": sum(1 for q in queue if q["kind"] == "privacy"),
              "queue_audit": sum(1 for q in queue if q["kind"] == "audit")})
    save_batch(folder, batch)
    check = subprocess.run([sys.executable, str(HERE / "validate_datasets.py")], capture_output=True, text=True)
    print(f"finish: {len(accepted)} cases added to {dataset.relative_to(HERE)}, {len(ambiguous)} ambiguous, "
          f"{s['queue_privacy']} privacy disputes and {s['queue_audit']} audit cases for the reviewer "
          f"(./review.py {batch['batch']}). Validator: {check.stdout.strip().splitlines()[-1]}")
    if check.returncode:
        print(check.stdout)


def cmd_status(args) -> None:
    root = panel_root(args)
    names = [args.batch] if args.batch else sorted(
        p.name for p in root.iterdir() if (p / "batch.json").exists()) if root.exists() else []
    print("| Batch | Rubric | Generated | Agreement | Privacy | Judge | Accepted | Ambiguous | Review |")
    print("|---|---|---|---|---|---|---|---|---|")
    for name in names:
        folder, b = load_batch(args, name)
        s = b.get("stats", {})
        review = json.loads((folder / "review_result.json").read_text()) if (folder / "review_result.json").exists() else None
        rv = "open" if b.get("finished") and not review else (
            f"{'sampled: ' if review.get('quick') else ''}audit {review['audit_wrong']}/{review['audit_n']} wrong"
            if review else "")
        print(f"| {name} | v{b['rubric_version']} | {s.get('generated', '')} | {s.get('agreement', '')} | "
              f"{s.get('privacy', '')} | {s.get('judged_clear', '')}/{s.get('disagree', '')} | "
              f"{s.get('accepted', '')} | {s.get('ambiguous', '')} | {rv} |")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    for name in ("run", "generate"):
        p = sub.add_parser(name)
        p.add_argument("step", choices=P.STEPS)
        p.add_argument("--split", choices=["dev", "holdout"], required=True)
        p.add_argument("--count", type=int, required=True)
        p.add_argument("--only", default=None, metavar="TAG,TAG",
                       help="use only these slot tags (to fill a coverage gap)")
        if name == "run":
            p.add_argument("--no-finish", action="store_true",
                           help="stop after compare, for a pilot (use another --results root)")
    for name in ("review", "compare", "finish"):
        sub.add_parser(name).add_argument("batch")
    sub.add_parser("status").add_argument("batch", nargs="?")
    for p in sub.choices.values():
        p.add_argument("--audit", type=int, default=AUDIT_SIZE,
                       help="audit cases for each batch in the review queue (default %(default)s)")
        p.add_argument("--results", default=None)
        p.add_argument("--parallel", type=int, default=4)
        p.add_argument("--agent-cmd", default="pi")
        p.add_argument("--force", action="store_true")
        for role in ("generate", "review", "judge"):
            p.add_argument(f"--model-{role}", default=None)
            p.add_argument(f"--thinking-{role}", default=None)
    args = parser.parse_args()
    if args.cmd == "run":
        args.batch = cmd_generate(args)
        cmd_review(args)
        cmd_compare(args)
        if not args.no_finish:
            cmd_finish(args)
    elif args.cmd == "generate":
        cmd_generate(args)
    else:
        {"review": cmd_review, "compare": cmd_compare, "finish": cmd_finish, "status": cmd_status}[args.cmd](args)


if __name__ == "__main__":
    main()
