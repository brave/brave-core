#!/usr/bin/env python3
"""Makes relevance cases with a panel of separate agents (plan 7.4, 7.3). The
relevance step has a memory pool and messages, so it has its own script;
label_panel.py makes the other steps. README section 22.

  pool (G, Opus)      writes the memories of one invented user
  messages (G, Opus)  writes a message for each coverage slot, with the ids of
                      the memories to use (must) and the tempting ones (forbidden)
  review (R, Sonnet)  blind: marks the memories to use and the maybe ones
  judge (J, Opus)     settles a disagreement
  finish              cases to datasets/steps/relevance/<split>.jsonl, the pool
                      to memories.jsonl, doubtful cases to datasets/ambiguous/

  ./relevance_panel.py run --split dev --count 31
  ./relevance_panel.py run --split holdout --count 15
  ./relevance_panel.py status

Rubric: datasets/rubrics/relevance.md. Batches: <results root>/panel/relevance-<split>-b<n>/.
A pass that has valid answers is not run again.
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
from label_panel import chunks, read_jsonl, write_jsonl
from lib import agent_tasks, clients

HERE = Path(__file__).resolve().parent
DATASETS = HERE / "datasets"
STEP_DIR = DATASETS / "steps" / "relevance"
POOL_SIZE = 40

PERSONAS = {
    "dev": "Lena, 41, a landscape architect in Utrecht. Married to Pieter, two kids (9 and 12), a rescue greyhound "
           "named Fenne. Cycles, bakes bread, plays the cello, plans a sabbatical in Japan. Writes code for her "
           "plant database in Python",
    "holdout": "Tariq, 36, a paramedic turned software tester in Leeds. Lives with his sister, has a parrot named "
               "Kiwi, supports Leeds United, learns to drive a manual car, cooks halal, is saving for a house, "
               "and wants answers in plain language",
}

MEMORY_KINDS = [
    "home city or neighborhood", "job or work project", "a family member", "partner or a close friend", "a pet",
    "a hobby or a sport", "a diet or a food preference", "a wish about the length of answers",
    "a wish about the format or the tone of answers", "a wish about units or about the language of answers",
    "a tool, a language or a technology the user works with", "a plan with a date (use a date like 2026-08-28)",
    "an event that happened with a date", "a belonging (a car, an instrument, a device)", "a long-term goal",
    "a fragment that lost its subject, such as 'Is three years old.' or 'Studies on Tuesdays.'",
    "a place the user visits often", "a habit or a routine", "something the user dislikes", "a project in progress",
]

MESSAGE_SLOTS = [
    ("direct", "the message is about a topic that one or two memories are about (the pet, the project)"),
    ("indirect", "the message does not name the topic, but a memory still helps (a dinner request and a diet, a "
                 "trip and the home city)"),
    ("short_follow_up", "a short follow-up such as 'and for two people?' or 'what about the dog?'. Write the "
                        "previous message too. The topic comes from the previous message"),
    ("style_wish", "a task where a wish about units, tone, language or format clearly applies (a distance question "
                   "and a units wish, a joke and a tone wish)"),
    ("unrelated", "a general question (science, history, maths, a coding question) that no memory helps with. "
                  "must is empty"),
    ("tempting", "a message that shares words or a topic area with some memories that do not help (a trip to "
                 "a city that is not the home; a pet question about another animal)"),
    ("multi", "a message that needs three or more memories"),
    ("lang:de", "any kind of message, written in German"),
    ("lang:es", "any kind of message, written in Spanish"),
    ("lang:fr", "any kind of message, written in French"),
    ("lang:ja", "any kind of message, written in Japanese"),
    ("lang:pt", "any kind of message, written in Portuguese"),
]


def rubric() -> tuple:
    text = (DATASETS / "rubrics" / "relevance.md").read_text()
    m = re.search(r"^Rubric version:\s*(\S+)", text, re.M)
    return text, m.group(1) if m else "?"


# ---- task texts -------------------------------------------------------------
def pool_task() -> str:
    return f"""# Agent task: write the memories of one user (step: relevance)

A memory feature of an AI assistant stores short facts about a user. You write
{POOL_SIZE} of these memories for one invented user. Other tasks use them to
test which memories fit a message, so they must be varied and realistic.

## What to write

items.jsonl has one slot on each line: "id", "persona" (the invented user, the
same on every line), and "kind" (what the memory is about). Write one memory
for each slot.

- Style of the feature: a short fact in the third person, with no subject, in
  English, in one sentence: "Has a beagle named Luna.", "Prefers short
  answers.", "Writes TypeScript with React.", "Plans to move to Berlin on
  2026-08-28.".
- All memories are about the same person and agree with each other. All text is
  invented.
- Make them different. Some are near each other in topic (a dog and a vet, two
  places). A few can share a word and be about different things.
- A fragment that lost its subject is short and has no name.
- Do not write health, money or religion data.
- "tags": 1 to 3 short words for the kind (for example "pet", "style_wish",
  "plan"). Use the tag "style_wish" for each wish about the style of answers
  (length, format, tone, units, language).

## Rubric (for context)

{rubric()[0]}

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each slot, with the "id" of the slot:

{{"id": ..., "text": "the memory", "tags": [...], "reason": "short note", "clear": true}}

Write valid JSON. Write nothing else in the file.
"""


def message_task() -> str:
    return f"""# Agent task: write test messages for a memory feature (step: relevance)

A memory feature of an AI assistant stores facts about a user. When the user
sends a message, the assistant should get the memories that are relevant to it.
`memories.jsonl` in this folder is the pool of memories of one user ("id",
"text"). Read it first. You write messages for this user, with the ids of the
memories that fit.

## Rubric

{rubric()[0]}

## What to write

items.jsonl has one slot on each line: "id", "tag" and "note" (what the message
must be), and "lang" (the language of the message; "en" is English). Write one
case for each slot.

- "message": one message from the user to the assistant, as a real person
  types it: short or medium, natural. For the tag `short_follow_up` also write
  "previous_message" (the message before it); for the other tags,
  "previous_message" is null.
- "must": the ids of the memories that the rubric calls `use`. Keep it small.
  It is empty for an unrelated message.
- "forbidden": 3 to 8 ids of memories that are tempting but have nothing to do
  with the message (the rubric: skip). Do not put a `maybe` memory here.
- "clear": false when the rubric does not give a clear answer for the case.
- "reason": one short sentence.
- "tags": coverage tags that fit, from: direct, indirect, short_follow_up,
  style_wish, unrelated, tempting, multi. The list can be empty.

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each slot, with the "id" of the slot:

{{"id": ..., "message": "...", "previous_message": null | "...", "must": [...], "forbidden": [...], "reason": "...", "clear": true | false, "tags": [...]}}

Write valid JSON. Write nothing else in the file.
"""


def review_task() -> str:
    return f"""# Agent task: label test messages for a memory feature (step: relevance)

`memories.jsonl` in this folder is the pool of memories of one user ("id",
"text"). Read it first. Each line of items.jsonl is a message that the user
sends to an AI assistant ("message", and sometimes "previous_message"). Use
only the rubric below.

## Rubric

{rubric()[0]}

## What to do

For each message, give the ids of the memories in the group `use`, and the ids
in the group `maybe`. The other memories are `skip`. Label each message alone.
Set "clear" to false when the rubric does not give a clear answer. "reason":
one short sentence.

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each message, with the "id" of the message:

{{"id": ..., "use": [...], "maybe": [...], "reason": "...", "clear": true | false}}

Write valid JSON. Write nothing else in the file.
"""


def judge_task() -> str:
    return f"""# Agent task: settle disagreements (step: relevance)

`memories.jsonl` in this folder is the pool of memories of one user ("id",
"text"). Two reviewers labeled each message with the rubric below, and they
disagree. Each line of items.jsonl has the message, and "a" and "b": the two
labels with their reasons ("use", and for b also "maybe").

## Rubric

{rubric()[0]}

## What to do

Decide the groups that the rubric gives: `use` and `maybe`. You can follow a, b
or neither. Set "clear" to false when the rubric does not give a clear answer.

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each message, with the "id" of the message:

{{"id": ..., "use": [...], "maybe": [...], "reason": "...", "clear": true | false}}

Write valid JSON. Write nothing else in the file.
"""


# ---- validators -------------------------------------------------------------
def _ids(v, pool: set) -> str | None:
    if not isinstance(v, list) or not all(isinstance(x, str) for x in v):
        return "must be a list of memory ids"
    bad = [x for x in v if x not in pool]
    return f"unknown memory ids {bad}" if bad else None


def validate_pool(item: dict, a: dict):
    if not (isinstance(a.get("text"), str) and 3 <= len(a["text"].strip()) <= 200):
        return "text must be a short sentence"
    if not isinstance(a.get("tags", []), list):
        return "tags must be a list"
    return None


def validate_messages(pool: set):
    def check(item: dict, a: dict):
        if not (isinstance(a.get("message"), str) and a["message"].strip()):
            return "message is missing"
        prev = a.get("previous_message")
        if item["tag"] == "short_follow_up" and not (isinstance(prev, str) and prev.strip()):
            return "a short_follow_up needs previous_message"
        if prev is not None and not isinstance(prev, str):
            return "previous_message must be text or null"
        for key in ("must", "forbidden"):
            bad = _ids(a.get(key), pool)
            if bad:
                return f"{key} {bad}"
        if set(a["must"]) & set(a["forbidden"]):
            return "an id is in must and in forbidden"
        if item["tag"] == "unrelated" and a["must"]:
            return "an unrelated message has an empty must"
        if item["tag"] in ("direct", "indirect", "style_wish", "multi", "short_follow_up") and not a["must"]:
            return f"a {item['tag']} message needs at least one must id"
        if not isinstance(a.get("clear"), bool) or not isinstance(a.get("reason"), str):
            return "reason must be text and clear must be true or false"
        return None
    return check


def validate_groups(pool: set):
    def check(item: dict, a: dict):
        for key in ("use", "maybe"):
            bad = _ids(a.get(key), pool)
            if bad:
                return f"{key} {bad}"
        if set(a["use"]) & set(a["maybe"]):
            return "an id is in use and in maybe"
        if not isinstance(a.get("clear"), bool) or not isinstance(a.get("reason"), str):
            return "reason must be text and clear must be true or false"
        return None
    return check


# ---- passes -----------------------------------------------------------------
def models(args) -> dict:
    out = {}
    for r in ("generate", "review", "judge"):
        spec, thinking = agent_run.DEFAULT_MODELS[r]
        out[r] = {"model": getattr(args, f"model_{r}", None) or spec,
                  "thinking": getattr(args, f"thinking_{r}", None) or thinking}
    return out


def run(args, folders, m, validate, name) -> dict:
    print(f"{name}: {len(folders)} tasks, {m['model']} ({m['thinking']})", flush=True)
    res = agent_run.run_tasks(folders, m["model"], m["thinking"], validate, name, parallel=args.parallel,
                              agent_cmd=args.agent_cmd)
    answers, bad = {}, []
    for folder, (by_id, errors) in res.items():
        answers.update(by_id)
        if errors:
            bad.append(f"{folder.name}: {errors[:3]}")
    if bad:
        print(f"WARNING: tasks with errors after all tries: {bad}")
    return answers


def with_pool(folder: Path, pool_rows: list) -> Path:
    write_jsonl(folder / "memories.jsonl", [{"id": r["id"], "text": r["text"]} for r in pool_rows])
    return folder


def next_batch(root: Path, split: str) -> str:
    n = 1 + max([0] + [int(m.group(1)) for p in (root.glob(f"relevance-{split}-b*") if root.exists() else [])
                       if (m := re.search(r"-b(\d+)$", p.name))])
    return f"relevance-{split}-b{n}"


def msg_key(c: dict) -> str:
    return re.sub(r"\s+", " ", (c.get("previous_message") or "") + " | " + c["message"]).lower()


def cmd_run(args) -> None:
    results = clients.results_root(args.results)
    root = results / "panel"
    name = next_batch(root, args.split)
    folder = root / name
    folder.mkdir(parents=True)
    tasks_root = agent_tasks.task_root(results)
    m = models(args)
    rng = random.Random(name)
    _, version = rubric()
    batch = {"batch": name, "step": "relevance", "split": args.split, "count": args.count,
             "created": datetime.datetime.now().isoformat(timespec="seconds"), "rubric_version": version,
             "models": m}

    # 1. the pool
    existing = {int(x["id"].split("-g")[1]) for x in read_jsonl(STEP_DIR / "memories.jsonl") if "-g" in x["id"]}
    first_mem = max(existing | {0}) + 1
    mem_slots = [{"id": f"mem-{args.split}-g{first_mem + n:03d}", "persona": PERSONAS[args.split],
                  "kind": MEMORY_KINDS[n % len(MEMORY_KINDS)]} for n in range(POOL_SIZE)]
    task = agent_tasks.write_task(tasks_root / f"{name}-P1", pool_task(), mem_slots)
    answers = run(args, [task], m["generate"], validate_pool, "pool")
    pool, seen = [], set()
    for s in mem_slots:
        a = answers.get(s["id"])
        if a and a["text"].strip().lower() not in seen:
            seen.add(a["text"].strip().lower())
            pool.append({"id": s["id"], "text": a["text"].strip(),
                         "tags": [t for t in a.get("tags", []) if isinstance(t, str)]})
    pool_ids = {p["id"] for p in pool}
    write_jsonl(folder / "pool.jsonl", pool)
    print(f"pool: {len(pool)} memories", flush=True)

    # 2. the messages
    old_numbers = [int(x["id"].rsplit("-g", 1)[1]) for x in read_jsonl(STEP_DIR / f"{args.split}.jsonl")
                   if "-g" in x["id"]]
    first = max(old_numbers + [0]) + 1
    slots = []
    for n in range(args.count):
        tag, note = MESSAGE_SLOTS[n % len(MESSAGE_SLOTS)]
        slots.append({"id": f"relevance-{args.split}-g{first + n:03d}", "tag": tag, "note": note,
                      "lang": tag.split(":")[1] if tag.startswith("lang:") else "en"})
    write_jsonl(folder / "slots.jsonl", slots)
    g_tasks = []
    for k, part in enumerate(chunks(slots, 30), 1):
        t = agent_tasks.write_task(tasks_root / f"{name}-G{k}", message_task(), part)
        with_pool(t, pool)
        g_tasks.append(t)
    g_answers = run(args, g_tasks, m["generate"], validate_messages(pool_ids), "messages")
    cases = {}
    seen_msgs = {msg_key(r) for sp in ("dev", "holdout") for r in read_jsonl(STEP_DIR / f"{sp}.jsonl")}
    for s in slots:
        a = g_answers.get(s["id"])
        if a is None or msg_key(a) in seen_msgs:
            continue
        seen_msgs.add(msg_key(a))
        cases[s["id"]] = {**a, "slot_tag": s["tag"], "lang": s["lang"]}
    write_jsonl(folder / "generated.jsonl", list(cases.values()))
    print(f"messages: {len(cases)} of {len(slots)}", flush=True)

    # 3. the review (blind)
    items = [{"id": cid, "message": c["message"],
              **({"previous_message": c["previous_message"]} if c.get("previous_message") else {})}
             for cid, c in cases.items()]
    r_tasks = []
    for k, part in enumerate(chunks(items, 30), 1):
        t = agent_tasks.write_task(tasks_root / f"{name}-R{k}", review_task(), part)
        with_pool(t, pool)
        r_tasks.append(t)
    r_answers = run(args, r_tasks, m["review"], validate_groups(pool_ids), "review")

    # 4. compare, and the judge
    outcomes, to_judge = {}, []
    for cid, c in cases.items():
        r = r_answers.get(cid)
        if r is None:
            outcomes[cid] = "no_review"
            continue
        g_must, g_forb = set(c["must"]), set(c["forbidden"])
        r_use, r_maybe = set(r["use"]), set(r["maybe"])
        agree = not (r_use - g_must) and not (g_must - (r_use | r_maybe)) and not (g_forb & r_use)
        outcomes[cid] = "agree" if agree else "disagree"
        if not agree:
            mine = {"use": sorted(g_must), "reason": c["reason"]}
            theirs = {"use": sorted(r_use), "maybe": sorted(r_maybe), "reason": r["reason"]}
            a, b = (mine, theirs) if rng.random() < 0.5 else (theirs, mine)
            to_judge.append({"id": cid, "message": c["message"],
                             **({"previous_message": c["previous_message"]} if c.get("previous_message") else {}),
                             "a": a, "b": b})
    j_answers = {}
    if to_judge:
        j_tasks = []
        for k, part in enumerate(chunks(to_judge, 30), 1):
            t = agent_tasks.write_task(tasks_root / f"{name}-J{k}", judge_task(), part)
            with_pool(t, pool)
            j_tasks.append(t)
        j_answers = run(args, j_tasks, m["judge"], validate_groups(pool_ids), "judge")

    # 5. finish
    origin = f"generated:{name}"
    texts = {p["id"]: p["text"] for p in pool}
    accepted, ambiguous = [], []
    for cid, c in cases.items():
        o = outcomes[cid]
        base = {"id": cid, "split": args.split, "step": "relevance", "batch": name,
                "case": {"message": c["message"], "previous_message": c.get("previous_message")}}
        if o == "agree":
            r = r_answers[cid]
            must = [x for x in c["must"] if x in r["use"]]
            forb = [x for x in c["forbidden"] if x not in r["use"] and x not in r["maybe"]]
            if c["clear"] and r["clear"] and (must or not c["must"]):
                accepted.append((c, must, forb, []))
            else:
                ambiguous.append({**base, "why": "agreed, not clear" if not (c["clear"] and r["clear"])
                                  else "must is empty",
                                  "labels": {"g": {"must": c["must"]}, "r": {"use": r["use"], "maybe": r["maybe"]}}})
        elif o == "disagree":
            j = j_answers.get(cid)
            if j and j["clear"]:
                forb = [x for x in c["forbidden"] if x not in j["use"] and x not in j["maybe"]]
                accepted.append((c, list(j["use"]), forb, ["adjudicated"]))
            else:
                ambiguous.append({**base, "why": "judge not clear" if j else "no judge answer",
                                  "labels": {"g": {"must": c["must"]}, "r": {"use": r_answers[cid]["use"]},
                                             "j": j}})
    rows = []
    for c, must, forb, extra in accepted:
        tags = list(dict.fromkeys([c["slot_tag"]] + [t for t in c.get("tags", []) if isinstance(t, str)] + extra))
        tags = [t for t in tags if not t.startswith("lang:")] + [f"lang:{c['lang']}"]
        row = {"id": c["id"], "split": args.split, "tags": tags, "origin": origin, "message": c["message"]}
        if c.get("previous_message"):
            row["previous_message"] = c["previous_message"]
        row.update(memories=[p["id"] for p in pool], must=must, forbidden=forb)
        rows.append(row)
    agree_n = sum(1 for o in outcomes.values() if o == "agree")
    compared = sum(1 for o in outcomes.values() if o != "no_review")
    stats = {"generated": len(cases), "compared": compared, "agree": agree_n,
             "agreement": round(agree_n / compared, 3) if compared else None, "disagree": len(to_judge),
             "judged_clear": sum(1 for j in j_answers.values() if j["clear"]), "accepted": len(rows),
             "ambiguous": len(ambiguous), "pool": len(pool)}
    if stats["agreement"] is not None and stats["agreement"] < 0.8 and not args.force:
        sys.exit(f"agreement {stats['agreement']} is below 0.8: the rubric is not clear "
                 "(--force writes the cases anyway)")
    if not rows:
        sys.exit("no accepted case; nothing is written (look at the agent logs in the task folders)")
    write_jsonl(STEP_DIR / "memories.jsonl", read_jsonl(STEP_DIR / "memories.jsonl") +
                [{"id": p["id"], "text": p["text"], "origin": origin, "tags": p["tags"]} for p in pool])
    write_jsonl(STEP_DIR / f"{args.split}.jsonl", read_jsonl(STEP_DIR / f"{args.split}.jsonl") + rows)
    amb = DATASETS / "ambiguous" / "relevance.jsonl"
    write_jsonl(amb, read_jsonl(amb) + ambiguous)
    audit_rng = random.Random(name + "-audit")
    queue = [{"kind": "audit", "id": r["id"], "step": "relevance", "split": args.split, "batch": name, "row": r,
              "texts": {i: texts[i] for i in r["must"] + r["forbidden"]}}
             for r in audit_rng.sample(rows, min(args.audit, len(rows)))]
    write_jsonl(folder / "review_queue.jsonl", queue)
    batch.update(stats=stats, finished=datetime.datetime.now().isoformat(timespec="seconds"))
    (folder / "batch.json").write_text(json.dumps(batch, indent=2, ensure_ascii=False))
    check = subprocess.run([sys.executable, str(HERE / "validate_datasets.py")], capture_output=True, text=True)
    print(f"finish: {len(rows)} cases and {len(pool)} memories added, {len(ambiguous)} ambiguous, agreement "
          f"{stats['agreement']}, {len(queue)} audit cases. Validator: {check.stdout.strip().splitlines()[-1]}")
    if check.returncode:
        print(check.stdout)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--split", choices=["dev", "holdout"], required=True)
    r.add_argument("--count", type=int, required=True)
    r.add_argument("--audit", type=int, default=5)
    sub.add_parser("status")
    for p in sub.choices.values():
        p.add_argument("--results", default=None)
        p.add_argument("--parallel", type=int, default=4)
        p.add_argument("--agent-cmd", default="pi")
        p.add_argument("--force", action="store_true")
        for role in ("generate", "review", "judge"):
            p.add_argument(f"--model-{role}", default=None)
            p.add_argument(f"--thinking-{role}", default=None)
    args = parser.parse_args()
    if args.cmd == "status":
        root = clients.results_root(args.results) / "panel"
        for b in sorted(root.glob("relevance-*/batch.json")):
            d = json.loads(b.read_text())
            print(d["batch"], d["stats"])
        return
    cmd_run(args)


if __name__ == "__main__":
    main()
