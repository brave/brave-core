"""Task folders for separate agents, and the check of their answers (plan 7.5).

A task folder has:
  task.md        the pass, the rubric and the answer format
  items.jsonl    the items (with no labels for a blind review)
  answers.jsonl  written by the agent: a "_meta" line (added by agent_run.py),
                 then one JSON line for each item
  errors.md      the errors of the last try, when the agent must try again

The agent sees only its folder. No API key is read or passed.
"""

from __future__ import annotations

import json
from pathlib import Path

from . import clients

MAX_ITEMS = 100
MAX_BYTES = 40_000


def task_root(root: Path | None = None) -> Path:
    return (root or clients.results_root()) / "agent_tasks"


def write_task(folder: Path, task_md: str, items: list) -> Path:
    """Writes task.md and items.jsonl. Keeps a valid answers.jsonl of the same
    items, so that a rerun does not start the agent again."""
    lines = [json.dumps(i, ensure_ascii=False) for i in items]
    data = "\n".join(lines) + "\n"
    if len(items) >= MAX_ITEMS or len(data.encode()) > MAX_BYTES:
        raise ValueError(
            f"{folder.name}: {len(items)} items, {len(data.encode())} bytes; "
            f"split the task (at most {MAX_ITEMS - 1} items, {MAX_BYTES} bytes)"
        )
    folder.mkdir(parents=True, exist_ok=True)
    old = (
        (folder / "items.jsonl").read_text()
        if (folder / "items.jsonl").exists()
        else None
    )
    if old is not None and old != data:
        # Other items: the old answers do not belong to them.
        for name in ("answers.jsonl", "errors.md"):
            (folder / name).unlink(missing_ok=True)
    (folder / "task.md").write_text(task_md)
    (folder / "items.jsonl").write_text(data)
    return folder


def read_items(folder: Path) -> list:
    return [
        json.loads(l)
        for l in (folder / "items.jsonl").read_text().splitlines()
        if l.strip()
    ]


def read_answers(folder: Path) -> tuple:
    """Returns (meta, lines, parse errors)."""
    path = folder / "answers.jsonl"
    meta, lines, errors = None, [], []
    if not path.exists():
        return meta, lines, ["answers.jsonl is missing"]
    for n, text in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not text.strip():
            continue
        try:
            value = json.loads(text)
        except json.JSONDecodeError as e:
            errors.append(f"line {n}: not valid JSON ({e.msg})")
            continue
        if isinstance(value, dict) and "_meta" in value:
            meta = value["_meta"]
        elif isinstance(value, dict):
            lines.append(value)
        else:
            errors.append(f"line {n}: a line must be a JSON object")
    return meta, lines, errors


def check(folder: Path, validate) -> tuple:
    """Checks the answers against the items. |validate(item, answer)| returns
    an error text or None. Returns (answers by id, errors)."""
    items = {i["id"]: i for i in read_items(folder)}
    _, lines, errors = read_answers(folder)
    by_id: dict = {}
    for a in lines:
        aid = a.get("id")
        if aid not in items:
            errors.append(f"unknown id {aid!r}")
            continue
        if aid in by_id:
            errors.append(f"id {aid} is answered more than one time")
            continue
        problem = validate(items[aid], a)
        if problem:
            errors.append(f"id {aid}: {problem}")
            continue
        by_id[aid] = a
    missing = [
        i
        for i in items
        if i not in by_id and not any(e.startswith(f"id {i}:") for e in errors)
    ]
    if missing:
        errors.append(
            f"no answer for {len(missing)} ids: {', '.join(missing[:20])}"
        )
    return by_id, errors


def write_meta(folder: Path, meta: dict) -> None:
    """Puts the "_meta" line first in answers.jsonl."""
    path = folder / "answers.jsonl"
    rest = [
        l
        for l in path.read_text(encoding="utf-8").splitlines()
        if l.strip() and not l.lstrip().startswith('{"_meta"')
    ]
    path.write_text(
        "\n".join([json.dumps({"_meta": meta}, ensure_ascii=False)] + rest)
        + "\n"
    )


def basic_validate(item: dict, answer: dict):
    return (
        None
        if isinstance(answer.get("reason", ""), str)
        else "reason must be a string"
    )
