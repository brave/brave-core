#!/usr/bin/env python3
"""Runs the learned memory (Dreaming) eval in a new Brave profile.

See README.md. For each chat set the script makes a new profile, starts the
browser in eval mode, waits for report.json and writes report.md.
"""
from __future__ import annotations

import argparse
import datetime
import json
import os
import shutil
import subprocess
import sys
import time
import urllib.parse
from pathlib import Path

import ollama_setup
import report as report_lib

HERE = Path(__file__).resolve().parent
SRC = HERE.parents[2]  # .../src
DEFAULT_BRAVE = (SRC / "out/Component_arm64/Brave Browser Development.app"
                 "/Contents/MacOS/Brave Browser Development")
DEFAULT_SOURCE_PROFILE = Path(
    "~/Library/Application Support/BraveSoftware/Brave-Browser-Development"
).expanduser()
MODEL_KEY = "custom:e0a1b2c3"
# A date in the past, as a Chrome time (microseconds since 1601).
ACCEPTED_DISCLAIMER = "13398544097773196"


def log(message: str) -> None:
    print(f"[{time.strftime('%H:%M:%S')}] {message}", flush=True)


def load_sets(names: list | None) -> list:
    sets = []
    for path in sorted((HERE / "chat_sets").glob("*.json")):
        if names and path.stem not in names:
            continue
        sets.append(json.loads(path.read_text()))
    if names:
        missing = set(names) - {s["name"] for s in sets}
        if missing:
            sys.exit(f"Unknown chat sets: {', '.join(sorted(missing))}")
    return sets


def preflight(args) -> None:
    if not Path(args.brave).exists():
        sys.exit(f"Browser not found: {args.brave}\nBuild it with `pnpm build`.")
    if not (Path(args.source_profile) / "BraveLocalAIModels").exists():
        sys.exit(f"No BraveLocalAIModels in {args.source_profile}. Start the dev "
                 "browser one time with local AI on, so that the embedder downloads.")


def prepare_ollama(args) -> None:
    """Checks that the two Ollama models are installed, and loads them so that
    the run does not wait for them (see ollama_setup.py)."""
    try:
        if args.no_warmup:
            ollama_setup.check_installed(args.ollama, [args.decision, args.llm])
        else:
            ollama_setup.load_models(args.ollama, args.decision, args.llm,
                                     args.keep_alive, log=log)
    except ollama_setup.OllamaSetupError as error:
        sys.exit(str(error))


def make_profile(args, profile: Path) -> None:
    (profile / "Default").mkdir(parents=True)
    src = Path(args.source_profile) / "BraveLocalAIModels"
    dst = profile / "BraveLocalAIModels"
    # `cp -c` clones the files on APFS, so the copy is fast and uses no space.
    if subprocess.run(["cp", "-c", "-R", str(src), str(dst)],
                      capture_output=True).returncode != 0:
        shutil.copytree(src, dst)
    prefs = {"brave": {"history_embeddings_enabled": True, "ai_chat": {
        "storage_enabled": True,
        "user_memory_enabled": True,
        "ollama_fetch_enabled": False,
        "last_accepted_disclaimer": ACCEPTED_DISCLAIMER,
        "user_dismissed_storage_notice": True,
        "custom_models": [{
            "key": MODEL_KEY, "label": args.llm, "model_request_name": args.llm,
            "endpoint_url": f"{args.ollama}/v1/chat/completions",
            "api_key": "", "context_size": 32768, "supports_tools": False,
            "vision_support": False,
        }],
    }}}
    (profile / "Default" / "Preferences").write_text(json.dumps(prefs, indent=2))


def features_switch(args) -> str:
    params = {
        "certain_threshold": args.threshold, "certain_margin": args.margin,
        "run_time_limit": f"{args.time_limit}s",
        "decision_model_name": args.decision,
        "decision_model_url": f"{args.ollama}/v1/systemone",
        "local_llm_model_name": args.llm,
    }
    # Feature param values are URL-escaped.
    text = "/".join(f"{k}/{urllib.parse.quote(str(v), safe='')}"
                    for k, v in params.items())
    # HistoryEmbeddings makes Brave register the EmbeddingGemma component. The
    # embedder of Dreaming has no model without it.
    return f"--enable-features=AIChatLearnedMemory:{text},HistoryEmbeddings"


def browser_command(args, profile: Path, chats: Path, output: Path) -> list:
    return [
        args.brave, f"--user-data-dir={profile}",
        "--no-first-run", "--no-default-browser-check",
        "--use-mock-keychain", "--password-store=basic",
        # Not --disable-component-update: with it, Brave does not register the
        # embedder component. A closed port makes the update check fail, so the
        # component stays at the copied version.
        "--component-updater=url-source=http://127.0.0.1:9/",
        features_switch(args),
        "--enable-logging=stderr",
        "--vmodule=user_memory_manager=1,learned_memory_eval=1,dreaming_run=1",
        f"--learned-memory-eval-chats={chats}",
        f"--learned-memory-eval-output={output}",
        "about:blank",
    ]


def read_report(path: Path):
    if not path.exists() or path.stat().st_size == 0:
        return None
    try:
        return json.loads(path.read_text())
    except json.JSONDecodeError:
        return None  # The file is still being written.


def stop(proc: subprocess.Popen, profile: Path) -> None:
    proc.terminate()
    try:
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        proc.kill()
    # Helper processes of this profile, if any.
    subprocess.run(["pkill", "-f", f"--user-data-dir={profile}"],
                   capture_output=True)


def run_set(args, chat_set: dict, out_dir: Path):
    name = chat_set["name"]
    set_dir = out_dir / name
    profile = set_dir / "profile"
    set_dir.mkdir(parents=True)
    chats_path = set_dir / "chats.json"
    chats_path.write_text(json.dumps(chat_set, indent=2, ensure_ascii=False))
    output = set_dir / "report.json"
    make_profile(args, profile)
    command = browser_command(args, profile, chats_path, output)
    (set_dir / "command.txt").write_text(" \\\n  ".join(
        f'"{c}"' if " " in c else c for c in command) + "\n")
    if args.dry_run:
        log(f"{name}: profile ready, command in {set_dir / 'command.txt'}")
        return None

    log(f"{name}: start ({len(chat_set['chats'])} chats)")
    started = time.time()
    with open(set_dir / "brave.log", "w") as log_file:
        proc = subprocess.Popen(command, stdout=log_file, stderr=subprocess.STDOUT)
        data = None
        while time.time() - started < args.timeout:
            data = read_report(output)
            if data is not None or proc.poll() is not None:
                break
            time.sleep(2)
        time.sleep(1)
        data = data or read_report(output)
        stop(proc, profile)
    if data is None:
        fatal = next((l.split("] ", 1)[-1].strip()[:300]
                      for l in (set_dir / "brave.log").read_text(errors="replace").splitlines()
                      if ":FATAL:" in l), "")
        data = {"error": f"no report after {int(time.time() - started)} s"
                         f"{', browser crashed: ' + fatal if fatal else ''} (see brave.log)"}
        log(f"{name}: {data['error']}")
    markdown, summary = report_lib.render(
        chat_set, data, {"wall_seconds": round(time.time() - started)})
    (set_dir / "report.md").write_text(markdown)
    log(f"{name}: {summary['status']}, {summary['memories']} memories, "
        f"checks {summary['checks_passed']}/{summary['checks_total']}, "
        f"{summary['forbidden_hits']} forbidden hits")
    if not args.keep_profiles:
        shutil.rmtree(profile, ignore_errors=True)
    return summary


def report_only(out_dir: Path) -> None:
    summaries = []
    for path in sorted(out_dir.glob("*/report.json")):
        chat_set = json.loads((path.parent / "chats.json").read_text())
        data = json.loads(path.read_text())
        markdown, summary = report_lib.render(chat_set, data)
        (path.parent / "report.md").write_text(markdown)
        summaries.append(summary)
    (out_dir / "summary.md").write_text(
        report_lib.render_summary(summaries, {"time": out_dir.name}))
    log(f"Wrote reports for {len(summaries)} sets in {out_dir}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sets", nargs="*", help="names of chat sets (default: all)")
    parser.add_argument("--list", action="store_true", help="list the chat sets")
    parser.add_argument("--brave", default=str(DEFAULT_BRAVE))
    parser.add_argument("--out", default=None)
    parser.add_argument("--source-profile", default=str(DEFAULT_SOURCE_PROFILE))
    ollama_setup.add_arguments(parser)
    parser.add_argument("--threshold", type=float, default=0.8)
    parser.add_argument("--margin", type=float, default=0.2)
    parser.add_argument("--time-limit", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=1200)
    parser.add_argument("--keep-profiles", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--report-only", metavar="DIR", default=None,
                        help="make new report.md files from an earlier run")
    args = parser.parse_args()

    if args.report_only:
        report_only(Path(args.report_only))
        return
    sets = load_sets(args.sets)
    if args.list:
        for s in sets:
            print(f'{s["name"]:20} {len(s["chats"]):2} chats  {s.get("description", "")}')
        return
    if not args.dry_run:
        preflight(args)
        prepare_ollama(args)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    out_dir = Path(args.out or f"/tmp/learned_memory_eval/{stamp}")
    out_dir.mkdir(parents=True, exist_ok=True)
    log(f"Output: {out_dir}")

    summaries = []
    for chat_set in sets:
        summary = run_set(args, chat_set, out_dir)
        if summary:
            summaries.append(summary)
    if summaries:
        meta = {"time": stamp, "decision": args.decision, "llm": args.llm,
                "threshold": args.threshold, "margin": args.margin}
        (out_dir / "summary.md").write_text(report_lib.render_summary(summaries, meta))
        (out_dir / "summary.json").write_text(json.dumps(summaries, indent=2))
        print("\n" + (out_dir / "summary.md").read_text())


if __name__ == "__main__":
    main()
