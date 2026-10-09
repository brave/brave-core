#!/usr/bin/env python3
"""Makes end-to-end chat sets with gold with a panel of separate agents
(plan 7.4 for e2e, plan 7.7, P1.4). README section 22.

  G (Opus)    writes chats.json and gold.json for a set spec (lib/e2e_panel.py)
  check       (script) rejects a set with a mechanical error; G gets the error list and tries again
  R (Sonnet)  checks the gold turn by turn against the chats: ok or fix
  J (Opus)    settles the complaints of R and writes the corrected gold
  finish      writes datasets/e2e/<split>/<name>.json

  ./e2e_panel.py list
  ./e2e_panel.py run                          # every set that is not finished
  ./e2e_panel.py run --sets noisy_style injection
  ./e2e_panel.py status

Batches are in <results root>/panel/e2e-<name>/, agent tasks in
<results root>/agent_tasks/e2e-<name>-G1, -R1.., -J1. A pass that has valid
answers is not run again. The sets are not reviewed by a person: R and J check
them. A person can read a set in datasets/e2e/ and its notes in the batch folder.
"""
from __future__ import annotations

import argparse
import datetime
import json
import shutil
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import agent_run
from lib import agent_tasks, clients
from lib import e2e_panel as E

HERE = Path(__file__).resolve().parent
DATASETS = HERE / "datasets"
R_CHUNK = 30  # turn checks for each R task
R_BYTES = 25_000


def folder_of(args, name: str) -> Path:
    return clients.results_root(args.results) / "panel" / f"e2e-{name}"


def task_dir(args, name: str, pass_id: str) -> Path:
    return agent_tasks.task_root(clients.results_root(args.results)) / f"e2e-{name}-{pass_id}"


def role(args, which: str) -> tuple:
    spec, thinking = agent_run.DEFAULT_MODELS[which]
    return (getattr(args, f"model_{which}", None) or spec, getattr(args, f"thinking_{which}", None) or thinking)


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def run_one(args, spec: dict, log) -> dict:
    name = spec["name"]
    batch_dir = folder_of(args, name)
    batch_dir.mkdir(parents=True, exist_ok=True)
    if args.redo_review:
        for old in agent_tasks.task_root(clients.results_root(args.results)).glob(f"e2e-{name}-[RJ]*"):
            shutil.rmtree(old)
    rubric, version = E.rubric_text(HERE)
    models = {r: role(args, r) for r in ("generate", "review", "judge")}
    stats = {"set": name, "split": spec["split"], "rubric_version": version,
             "models": {r: {"model": m[0], "thinking": m[1]} for r, m in models.items()}}

    # G
    g_dir = task_dir(args, name, "G1")
    agent_tasks.write_task(g_dir, E.generate_task(spec, rubric), [{"id": name, "spec": spec["description"]}])
    res = agent_run.run_tasks([g_dir], models["generate"][0], models["generate"][1],
                              E.validate_generate(spec, g_dir), "generate", parallel=1, agent_cmd=args.agent_cmd,
                              log=lambda m: log(f"[{name}] {m}"))
    _, errors = res[g_dir]
    if errors:
        stats["failed"] = f"generate: {errors[:3]}"
        (batch_dir / "stats.json").write_text(json.dumps(stats, indent=2))
        log(f"[{name}] FAILED in generate: {errors[:2]}")
        return stats
    chats = read_json(g_dir / "chats.json")
    gold = read_json(g_dir / "gold.json")
    shutil.copy(g_dir / "chats.json", batch_dir / "chats.json")
    shutil.copy(g_dir / "gold.json", batch_dir / "gold.json")
    turns = sum(len(c["turns"]) for c in chats)
    stats.update(chats=len(chats), turns=turns, facts=len(gold["facts"]),
                 required=sum(1 for f in gold["facts"] if f["required"]), stale=len(gold["stale"]),
                 forbidden=len(gold["forbidden"]), ops=len(gold["ops"]))
    log(f"[{name}] G done: {len(chats)} chats, {turns} turns, {stats['facts']} facts ({stats['required']} required), "
        f"{stats['forbidden']} forbidden, {stats['ops']} ops")

    # R
    items = E.review_items(spec, chats, gold)
    turn_items, global_items = items[:-2], items[-2:]
    parts, size = [[]], 0
    for item in turn_items:
        n = len(json.dumps(item, ensure_ascii=False))
        if parts[-1] and (len(parts[-1]) >= R_CHUNK or size + n > R_BYTES):
            parts.append([])
            size = 0
        parts[-1].append(item)
        size += n
    parts.append(global_items)
    set_json = json.dumps(E.assemble(spec, chats, gold), ensure_ascii=False, indent=1)
    r_dirs = []
    for k, part in enumerate(parts, 1):
        d = task_dir(args, name, f"R{k}")
        agent_tasks.write_task(d, E.review_task(spec, rubric), part)
        (d / "set.json").write_text(set_json)
        r_dirs.append(d)
    res = agent_run.run_tasks(r_dirs, models["review"][0], models["review"][1], E.validate_review, "review",
                              parallel=2, agent_cmd=args.agent_cmd, log=lambda m: log(f"[{name}] {m}"))
    answers = {}
    for d in r_dirs:
        answers.update(res[d][0])
    missing = [i["id"] for i in items if i["id"] not in answers]
    complaints = [{"id": i, "complaint": a["reason"], "suggested_fix": a["fix"]}
                  for i, a in answers.items() if a["verdict"] == "fix"]
    stats.update(r_checked=len(answers), r_missing=len(missing), r_complaints=len(complaints),
                 r_unclear=sum(1 for a in answers.values() if not a.get("clear", True)))
    (batch_dir / "r_answers.json").write_text(json.dumps(list(answers.values()), indent=1, ensure_ascii=False))
    log(f"[{name}] R done: {len(answers)} of {len(items)} checks, {len(complaints)} complaints")

    # J
    final_gold = gold
    if complaints:
        j_dir = task_dir(args, name, "J1")
        agent_tasks.write_task(j_dir, E.judge_task(spec, rubric), complaints)
        (j_dir / "set.json").write_text(set_json)
        res = agent_run.run_tasks([j_dir], models["judge"][0], models["judge"][1],
                                  E.validate_judge(spec, j_dir, chats), "judge", parallel=1,
                                  agent_cmd=args.agent_cmd, log=lambda m: log(f"[{name}] {m}"))
        j_answers, errors = res[j_dir]
        if errors:
            stats["failed"] = f"judge: {errors[:3]}"
            (batch_dir / "stats.json").write_text(json.dumps(stats, indent=2))
            log(f"[{name}] FAILED in judge: {errors[:2]}")
            return stats
        else:
            final_gold = read_json(j_dir / "gold_final.json")
            stats["j_accepted"] = sum(1 for a in j_answers.values() if a["decision"] == "accept")
            stats["j_rejected"] = len(j_answers) - stats["j_accepted"]
            (batch_dir / "j_answers.json").write_text(json.dumps(list(j_answers.values()), indent=1,
                                                                  ensure_ascii=False))
            log(f"[{name}] J done: {stats['j_accepted']} accepted, {stats['j_rejected']} rejected")
    problems = E.check_set(spec, chats, final_gold, batch_dir)
    if problems:
        stats["failed"] = f"final check: {problems[:3]}"
        log(f"[{name}] FAILED the final check: {problems[:2]}")
    else:
        out = DATASETS / "e2e" / spec["split"] / f"{name}.json"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(E.assemble(spec, chats, final_gold), indent=1, ensure_ascii=False) + "\n")
        stats["finished"] = datetime.datetime.now().isoformat(timespec="seconds")
        stats["file"] = str(out.relative_to(HERE))
        log(f"[{name}] finished: {stats['file']}")
    (batch_dir / "stats.json").write_text(json.dumps(stats, indent=2))
    return stats


def cmd_run(args) -> None:
    specs = [s for s in E.SPECS if (not args.sets or s["name"] in args.sets) and
             (not args.split or s["split"] == args.split)]
    if not specs:
        sys.exit("no set matches")
    todo = []
    for s in specs:
        done = DATASETS / "e2e" / s["split"] / f"{s['name']}.json"
        if done.exists() and not (args.force or args.redo_review):
            print(f"{s['name']}: finished already ({done.relative_to(HERE)}); --force to run it again")
        else:
            todo.append(s)
    with ThreadPoolExecutor(max(1, args.parallel)) as pool:
        results = list(pool.map(lambda s: run_one(args, s, lambda m: print(m, flush=True)), todo))
    failed = [r["set"] for r in results if "failed" in r]
    print(f"\n{len(results) - len(failed)} of {len(results)} sets finished." +
          (f" Failed: {', '.join(failed)}" if failed else ""))
    cmd_status(args)
    sys.exit(1 if failed else 0)


def cmd_status(args) -> None:
    print("| Set | Split | Chats | Turns | Facts (required) | Stale | Forbidden | Ops | R complaints | J accepted | State |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    for s in E.SPECS:
        path = folder_of(args, s["name"]) / "stats.json"
        st = json.loads(path.read_text()) if path.exists() else {}
        state = "finished" if st.get("finished") else (st.get("failed") or "not run")
        print(f"| {s['name']} | {s['split']} | {st.get('chats', '')} | {st.get('turns', '')} | "
              f"{st.get('facts', '')} ({st.get('required', '')}) | {st.get('stale', '')} | {st.get('forbidden', '')} | "
              f"{st.get('ops', '')} | {st.get('r_complaints', '')} | {st.get('j_accepted', '')} | {state} |")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    run = sub.add_parser("run")
    run.add_argument("--sets", nargs="*", default=None)
    run.add_argument("--split", choices=["dev", "holdout"], default=None)
    run.add_argument("--force", action="store_true", help="make a set again, also when its file exists")
    run.add_argument("--redo-review", action="store_true",
                     help="run the R and J passes again (the G files are kept); implies --force")
    run.add_argument("--parallel", type=int, default=4, help="sets at the same time")
    sub.add_parser("status")
    for p in sub.choices.values():
        p.add_argument("--results", default=None)
        p.add_argument("--agent-cmd", default="pi")
        for r in ("generate", "review", "judge"):
            p.add_argument(f"--model-{r}", default=None)
            p.add_argument(f"--thinking-{r}", default=None)
    args = parser.parse_args()
    if args.cmd == "list":
        for s in E.SPECS:
            print(f"{s['name']:30} {s['split']:8} {s['size']}")
    elif args.cmd == "run":
        cmd_run(args)
    else:
        cmd_status(args)


if __name__ == "__main__":
    main()
