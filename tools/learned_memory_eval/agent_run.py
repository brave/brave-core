#!/usr/bin/env python3
"""Starts separate agents on task folders (plan 7.5).

Each task is a new `pi` process with no session, no extensions, no skills, no
context files and only the read and write tools. The agent reads task.md and
items.jsonl in its folder and writes answers.jsonl. The script checks the
answers, and starts the agent again with the error list (2 more tries). It
never reads an API key: pi uses its own login.

  ./agent_run.py <task folder> ... --model openrouter/anthropic/claude-opus-5.5 --thinking high
  ./agent_run.py <task folder> ... --model bedrock:claude-sonnet-5-5 --thinking low

A model is "openrouter/<id>", "bedrock:<model id>" (the profile ARN comes from
the DynamoDB table of the Bedrock extension, so each person uses their own
profile) or "bedrock:<profile ARN>". Other values go to `pi --model` as they
are. Bedrock needs a current AWS SSO login; the script checks it first and
prints the spend of the month before and after.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from lib import agent_tasks

DEFAULT_EXTENSION = "~/.pi/agent/extensions/bedrock-profiles.ts"
AWS_PROFILE = os.environ.get("AWS_PROFILE", "claude-code-bedrock-sso")
AWS_REGION = os.environ.get("AWS_REGION", "us-west-2")
CAPS_TABLE = "claude-code-bedrock-caps"

# Plan 7.5, agent models. The colleague sets his own values with the flags or
# these environment variables.
DEFAULT_MODELS = {
    "generate": (
        os.environ.get(
            "LM_EVAL_GENERATE_MODEL", "openrouter/anthropic/claude-opus-5.5"
        ),
        "high",
    ),
    "review": (
        os.environ.get("LM_EVAL_REVIEW_MODEL", "bedrock:claude-sonnet-5-5"),
        "low",
    ),
    "judge": (
        os.environ.get(
            "LM_EVAL_JUDGE_MODEL", "openrouter/anthropic/claude-opus-5.5"
        ),
        "high",
    ),
    "judge-fast": (
        os.environ.get("LM_EVAL_JUDGE_FAST_MODEL", "bedrock:claude-sonnet-5-5"),
        "low",
    ),
}

PROMPT = (
    "Do the agent task in {folder}. Read task.md and items.jsonl there, and the other files in that "
    "folder that task.md names. Then write the files that task.md asks for, with answers.jsonl last. "
    "Do not read any file outside that folder."
)
RETRY_PROMPT = (
    "Do the agent task in {folder}. Read task.md, items.jsonl and errors.md there, and the other "
    "files in that folder that task.md names. Your earlier answers.jsonl (or another file that "
    "task.md asks for) had the errors that errors.md lists. Write the files again: complete, one "
    "line in answers.jsonl for each item, without these errors. Do not read any file outside "
    "that folder."
)


def _aws(*args: str) -> dict:
    out = subprocess.run(
        [
            "aws",
            *args,
            "--profile",
            AWS_PROFILE,
            "--region",
            AWS_REGION,
            "--output",
            "json",
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:300] or "aws failed")
    return json.loads(out.stdout)


def bedrock_allowance() -> dict:
    """The spend, the cap and the profile ARNs of the current SSO user."""
    try:
        identity = _aws("sts", "get-caller-identity")
    except (RuntimeError, OSError, subprocess.SubprocessError) as e:
        raise SystemExit(
            f"Bedrock needs a current AWS SSO login: aws sso login --profile "
            f"{AWS_PROFILE} ({e})"
        )
    email = str(identity["UserId"]).split(":")[1]
    item = (
        _aws(
            "dynamodb",
            "get-item",
            "--table-name",
            CAPS_TABLE,
            "--consistent-read",
            "--key",
            json.dumps({"email": {"S": email}}),
        ).get("Item")
        or {}
    )
    return {
        "spend": float(item.get("mtd_spend_usd", {}).get("N", 0)),
        "cap": float(item.get("monthly_cap_usd", {}).get("N", 0)),
        "capped": item.get("capped", {}).get("S") == "true",
        "models": {
            m["M"]["model_id"]["S"]: m["M"]["arn"]["S"]
            for m in item.get("aips", {}).get("L", [])
        },
    }


def route_flags(
    spec: str, extension: str, allowance: dict | None = None
) -> list:
    if spec.startswith("bedrock:"):
        target = spec[len("bedrock:") :]
        if not target.startswith("arn:"):
            models = (allowance or bedrock_allowance())["models"]
            if target not in models:
                raise SystemExit(
                    f"no Bedrock profile for {target}; you have: {', '.join(sorted(models))}"
                )
            target = models[target]
        return [
            "-e",
            str(Path(extension).expanduser()),
            "--provider",
            "amazon-bedrock",
            "--model",
            target,
        ]
    return ["--model", spec]


def _run_one(folder: Path, cmd: list, prompt: str, timeout: int) -> tuple:
    start = time.time()
    try:
        out = subprocess.run(
            cmd + [prompt.format(folder=folder)],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        log = (out.stdout or "") + (out.stderr or "")
        code = out.returncode
    except subprocess.TimeoutExpired:
        log, code = f"timed out after {timeout} s", -1
    (folder / "agent.log").write_text(log[-20000:])
    return code, time.time() - start


def run_tasks(
    folders: list,
    spec: str,
    thinking: str,
    validate=agent_tasks.basic_validate,
    pass_name: str = "",
    parallel: int = 4,
    agent_cmd: str = "pi",
    extension: str = DEFAULT_EXTENSION,
    timeout: int = 1800,
    tries: int = 3,
    log=print,
) -> dict:
    """Runs one agent for each folder. Returns {folder: (answers by id, errors)}.
    A folder that has valid answers already is not run again."""
    allowance = bedrock_allowance() if spec.startswith("bedrock:") else None
    if allowance:
        if allowance["capped"]:
            raise SystemExit("Bedrock spend is at the monthly cap")
        log(
            f"Bedrock spend this month: ${allowance['spend']:.2f} of ${allowance['cap']:.0f}"
        )
    base = [
        agent_cmd,
        "--print",
        "--no-session",
        "--no-extensions",
        "--no-skills",
        "--no-context-files",
        "--no-prompt-templates",
        "--tools",
        "read,write",
        *route_flags(spec, extension, allowance),
        "--thinking",
        thinking,
    ]

    def work(folder: Path):
        folder = Path(folder)
        answers, errors = agent_tasks.check(folder, validate)
        if not errors:
            return folder, answers, errors, "cached"
        started = datetime.datetime.now().isoformat(timespec="seconds")
        seconds = 0.0
        for attempt in range(tries):
            prompt = PROMPT if attempt == 0 else RETRY_PROMPT
            if attempt:
                (folder / "errors.md").write_text(
                    "# Errors in answers.jsonl\n\n"
                    + "\n".join(f"- {e}" for e in errors[:50])
                    + "\n"
                )
            _, took = _run_one(folder, base, prompt, timeout)
            seconds += took
            answers, errors = agent_tasks.check(folder, validate)
            if not errors:
                break
        if (folder / "answers.jsonl").exists():
            agent_tasks.write_meta(
                folder,
                {
                    "task_id": folder.name,
                    "pass": pass_name,
                    "model": spec,
                    "thinking": thinking,
                    "started": started,
                    "seconds": round(seconds, 1),
                    "tries": attempt + 1,
                    "errors": errors[:20],
                },
            )
        if not errors:
            (folder / "errors.md").unlink(missing_ok=True)
        return folder, answers, errors, f"{attempt + 1} tries, {seconds:.0f} s"

    results = {}
    with ThreadPoolExecutor(max(1, parallel)) as pool:
        for folder, answers, errors, how in pool.map(work, folders):
            results[folder] = (answers, errors)
            log(
                f"  {folder.name}: {len(answers)} answers, {len(errors)} errors ({how})"
            )
    if allowance:
        after = bedrock_allowance()
        log(
            f"Bedrock spend this month: ${after['spend']:.2f} of ${after['cap']:.0f} "
            f"(the table can lag behind the calls)"
        )
    return results


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("folders", nargs="+")
    parser.add_argument("--model", default=DEFAULT_MODELS["review"][0])
    parser.add_argument("--thinking", default="low")
    parser.add_argument("--pass", dest="pass_name", default="")
    parser.add_argument("--parallel", type=int, default=4)
    parser.add_argument("--agent-cmd", default="pi")
    parser.add_argument("--bedrock-extension", default=DEFAULT_EXTENSION)
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args()
    results = run_tasks(
        [Path(f) for f in args.folders],
        args.model,
        args.thinking,
        pass_name=args.pass_name,
        parallel=args.parallel,
        agent_cmd=args.agent_cmd,
        extension=args.bedrock_extension,
        timeout=args.timeout,
    )
    sys.exit(1 if any(errors for _, errors in results.values()) else 0)


if __name__ == "__main__":
    main()
