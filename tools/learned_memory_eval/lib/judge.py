"""LLM judge for text quality (plan P2.7, section 7.5).

A judge is a separate agent (agent_run.py) that reads a rubric
(datasets/rubrics/judge_<kind>.md) and sorts outputs of the pipeline:

  rewrite  the memories of a rewrite against the user sentences
           (added detail, lost detail, wrong subject)
  merge    the merged memory against the old and the new memory (same flags)
  extras   a memory of an end-to-end run that matches no gold item
           (sensitive, hallucinated, not_about_user, short_lived, true_unlabeled)

Rules (plan 7.5):
- The agent sees the inputs only: not the variant, not the model, not the case
  id. The order is shuffled with a fixed seed, so a rerun gives the same tasks.
- Verdicts are cached in <results root>/judge_cache/<kind>.jsonl, keyed by the
  judge model, the thinking level, the rubric version and the input. A rerun
  starts agents only for new inputs.
- A run records its judge (model, thinking, rubric version). compare.py refuses
  to compare judge metrics of two different judges.
- Only pipeline outputs on dataset text go to a judge. Never a real chat.
"""
from __future__ import annotations

import hashlib
import json
import random
import re
from pathlib import Path

import agent_run
from . import agent_tasks, clients
from . import scoring as sc

EVAL_ROOT = Path(__file__).resolve().parents[1]
RUBRICS = EVAL_ROOT / "datasets" / "rubrics"
FLAGS = ["added_detail", "lost_detail", "wrong_subject"]
EXTRA_CLASSES = ["sensitive", "hallucinated", "not_about_user", "short_lived", "true_unlabeled"]
CHUNK = 40
ROLES = {"fast": "judge-fast", "final": "judge"}


def rubric(kind: str) -> tuple:
    text = (RUBRICS / f"judge_{kind}.md").read_text()
    m = re.search(r"^Rubric version:\s*(\S+)", text, re.M)
    return text, m.group(1) if m else "?"


def judge_spec(role: str, model: str | None = None, thinking: str | None = None) -> dict:
    """The judge of a role: {"model", "thinking"}. |role| is fast or final."""
    default_model, default_thinking = agent_run.DEFAULT_MODELS[ROLES[role]]
    return {"role": role, "model": model or default_model, "thinking": thinking or default_thinking}


# ---- answer formats and checks ----------------------------------------------
FORMAT = {
    "rewrite": '{"id": ..., "added_detail": true | false, "lost_detail": true | false, "wrong_subject": true | false, "reason": "one short sentence"}',
    "merge": '{"id": ..., "added_detail": true | false, "lost_detail": true | false, "wrong_subject": true | false, "reason": "one short sentence"}',
    "extras": '{"id": ..., "class": "sensitive" | "hallucinated" | "not_about_user" | "short_lived" | "true_unlabeled", "reason": "one short sentence"}',
}


def validate(kind: str):
    def check(item: dict, a: dict):
        if not isinstance(a.get("reason"), str):
            return "reason must be a string"
        if kind == "extras":
            return None if a.get("class") in EXTRA_CLASSES else f"class must be one of {EXTRA_CLASSES}"
        bad = [f for f in FLAGS if not isinstance(a.get(f), bool)]
        return f"{', '.join(bad)} must be true or false" if bad else None
    return check


def task_text(kind: str) -> str:
    text, _ = rubric(kind)
    return f"""# Agent task: judge outputs of a memory feature (kind: {kind})

You judge outputs of a memory feature of an AI assistant with the rubric below.
Each line of items.jsonl is one output with its input. Judge each one alone.
Use only the rubric and the item.

## Rubric

{text}

## Answer format

Write answers.jsonl in this folder: one JSON object on each line, one line for
each item, with the "id" of the item:

{FORMAT[kind]}

Write valid JSON. Write nothing else in the file.
"""


# ---- cache ------------------------------------------------------------------
def _key(kind: str, spec: dict, version: str, item: dict) -> str:
    blob = json.dumps([kind, spec["model"], spec["thinking"], version, item], sort_keys=True, ensure_ascii=False)
    return hashlib.sha256(blob.encode()).hexdigest()


def _cache_path(root: Path, kind: str) -> Path:
    return root / "judge_cache" / f"{kind}.jsonl"


def _load_cache(root: Path, kind: str) -> dict:
    path = _cache_path(root, kind)
    if not path.exists():
        return {}
    out = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            row = json.loads(line)
            out[row["key"]] = row
    return out


def _save_cache(root: Path, kind: str, rows: list) -> None:
    path = _cache_path(root, kind)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "a", encoding="utf-8") as f:
        for row in rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")


# ---- the judge ----------------------------------------------------------------
def judge(kind: str, inputs: dict, spec: dict, root: Path | None = None, parallel: int = 4,
          agent_cmd: str = "pi", log=print) -> dict:
    """|inputs| is {name: input dict}. Returns {name: verdict}. Inputs that
    are the same share one verdict. The agent sees neutral ids and a fixed
    shuffle."""
    root = root or clients.results_root()
    _, version = rubric(kind)
    cache = _load_cache(root, kind)
    keys = {name: _key(kind, spec, version, item) for name, item in inputs.items()}
    todo = {}
    for name, key in keys.items():
        if key not in cache and key not in todo:
            todo[key] = inputs[name]
    log(f"judge {kind}: {len(inputs)} outputs, {len(inputs) - sum(1 for k in keys.values() if k in todo)} "
        f"from the cache, {len(todo)} to judge ({spec['model']}, {spec['thinking']})")
    if todo:
        order = sorted(todo)
        digest = hashlib.sha256((spec["model"] + spec["thinking"] + version + "".join(order)).encode()).hexdigest()[:10]
        random.Random(digest).shuffle(order)
        ids = {f"j{n:03d}": key for n, key in enumerate(order, 1)}
        by_id = {i: todo[k] for i, k in ids.items()}
        folders = []
        names = list(by_id)
        for part, start in enumerate(range(0, len(names), CHUNK), 1):
            folder = agent_tasks.task_root(root) / f"judge-{kind}-{spec['role']}-{digest}-{part}"
            items = [{"id": i, **by_id[i]} for i in names[start:start + CHUNK]]
            agent_tasks.write_task(folder, task_text(kind), items)
            folders.append(folder)
        results = agent_run.run_tasks(folders, spec["model"], spec["thinking"], validate(kind),
                                      f"judge-{kind}", parallel=parallel, agent_cmd=agent_cmd, log=log)
        rows = []
        for folder, (answers, errors) in results.items():
            if errors:
                log(f"WARNING: {folder.name} has errors after all tries: {errors[:2]}")
            for i, a in answers.items():
                verdict = {k: v for k, v in a.items() if k != "id"}
                rows.append({"key": ids[i], "kind": kind, "model": spec["model"], "thinking": spec["thinking"],
                             "rubric_version": version, "verdict": verdict})
        _save_cache(root, kind, rows)
        cache.update({r["key"]: r for r in rows})
    return {name: cache[key]["verdict"] for name, key in keys.items() if key in cache}


def meta(kind: str, spec: dict) -> dict:
    return {"model": spec["model"], "thinking": spec["thinking"], "role": spec["role"],
            "rubric_version": rubric(kind)[1]}


def same_judge(a: dict | None, b: dict | None) -> bool:
    return a is None or b is None or all(a[k] == b[k] for k in ("model", "thinking", "rubric_version"))


# ---- inputs from step records -------------------------------------------------
def step_inputs(step: str, variant: str, records: list, cases: dict) -> dict:
    """The judge inputs of the records of one variant. The name has the
    variant, the case and the repeat; the agent does not see it."""
    out = {}
    for r in records:
        case = cases[r["id"]]
        name = f"{variant}|{r['id']}|{r['repeat']}"
        if step == "rewrite":
            texts = [f["text"] for f in r.get("facts", []) if not f["too_long"]]
            if texts:
                out[name] = {"turn_date": case["turn_date"], "sentences": case["sentences"], "memories": texts}
        elif step == "merge":
            if r.get("text") and not r.get("too_long"):
                out[name] = {"old": case["old"], "new": case["new"], "merged": r["text"]}
    return out


def faithful(verdict: dict) -> bool:
    return not any(verdict.get(f) for f in FLAGS)


def flag_metrics(verdicts: list) -> dict:
    """Rates for the verdicts of one variant. The keys start with judge_ so
    that compare.py shows them."""
    n = len(verdicts)
    return {"judge_faithful": sc.rate(sum(1 for v in verdicts if faithful(v)), n),
            **{f"judge_{f}": sc.rate(sum(1 for v in verdicts if v.get(f)), n) for f in FLAGS}}


def fmt_flag_metrics(m: dict) -> list:
    return ["| Judge: faithful | Added detail | Lost detail | Wrong subject |", "|---|---|---|---|",
            f"| {sc.fmt_rate(m['judge_faithful'])} | {sc.fmt_rate(m['judge_added_detail'], False)} | "
            f"{sc.fmt_rate(m['judge_lost_detail'], False)} | {sc.fmt_rate(m['judge_wrong_subject'], False)} |"]


# ---- inputs from an end-to-end run --------------------------------------------
def e2e_extras(run_dir: Path, sets_dir: Path | None = None) -> dict:
    """The extras of every report.json in |run_dir|: {name: input}."""
    from lib import gold as gold_lib
    out = {}
    paths = sorted(run_dir.glob("*/r*/report.json")) or sorted(run_dir.glob("*/report.json"))
    for path in paths:
        chat_set = json.loads((path.parent / "chats.json").read_text())
        if sets_dir and (sets_dir / f"{chat_set['name']}.json").exists():
            chat_set = json.loads((sets_dir / f"{chat_set['name']}.json").read_text())
        if chat_set.get("gold") is None:
            continue
        data = json.loads(path.read_text())
        if "error" in data:
            continue
        g = gold_lib.score(chat_set["gold"], data.get("memories", []), data.get("trace", []))
        texts = {f"c{c}t{t}": turn["user"] for c, chat in enumerate(chat_set["chats"])
                 for t, turn in enumerate(chat["turns"])}
        rel = path.parent.relative_to(run_dir)
        for n, (memory, sources) in enumerate(zip(g["extras"], g["extra_sources"])):
            turn_id = (sources[0].split(" ")[0] if sources else "")
            sentences = [s.split(": ", 1)[1] for s in sources if ": " in s]
            out[f"{rel}|{n}"] = {"turn": texts.get(turn_id, ""), "sentences": sentences, "memory": memory}
    return out


def extras_metrics(verdicts: dict) -> dict:
    counts = {c: 0 for c in EXTRA_CLASSES}
    for v in verdicts.values():
        counts[v["class"]] = counts.get(v["class"], 0) + 1
    return {"n": len(verdicts), "classes": counts}
