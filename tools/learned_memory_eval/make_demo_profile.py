#!/usr/bin/env python3
"""Makes a Brave profile with a mock chat history, for the learned memory UI.

The profile has the chats of demo/demo_history.json in the Leo history, and no
learned memories. Open brave://settings/leo-ai/customization, then select
"Dream now" to see Dreaming learn from the chats.

How it works: the eval mode of the browser imports the chats (it is the only
code that writes chats with dates in the past). The script lets the eval run
Dreaming one time, so that it knows the import and the models work, then it
stops the browser and removes the learned memories and the watermarks. The
result is a profile in the state "chats stored, Dreaming never ran".

See README.md for the requirements.
"""
from __future__ import annotations

import argparse
import json
import shutil
import sqlite3
import subprocess
import sys
import time
from pathlib import Path

import ollama_setup
import run_eval

HERE = Path(__file__).resolve().parent
MARKER = ".learned_memory_demo"
LAST_DREAMING_PREF = "learned_memory_last_dreaming_time"
SETTINGS_URL = "brave://settings/leo-ai/customization"
# Tables that Dreaming writes. The chat tables stay.
MEMORY_TABLES = ("learned_memory", "memory_source_link", "memory_watermark",
                 "memory_tombstone")
WINDOWS_TO_UNIX_EPOCH_S = 11644473600


def chrome_time_now() -> str:
    """The format of a base::Time pref: microseconds since 1601, as a string."""
    return str(int((time.time() + WINDOWS_TO_UNIX_EPOCH_S) * 1_000_000))


def run_import(args, out: Path) -> dict:
    """Starts the browser in eval mode and waits for the report."""
    profile = out / "profile"
    chats = out / "chats.json"
    report = out / "report.json"
    chats.write_text(Path(args.chats).read_text())
    run_eval.make_profile(args, profile)
    command = run_eval.browser_command(args, profile, chats, report)
    run_eval.log("Importing the chats. The browser runs Dreaming one time.")
    started = time.time()
    data = None
    with open(out / "setup.log", "w") as log_file:
        proc = subprocess.Popen(command, stdout=log_file,
                                stderr=subprocess.STDOUT)
        while time.time() - started < args.timeout:
            data = run_eval.read_report(report)
            if data is not None or proc.poll() is not None:
                break
            time.sleep(2)
        time.sleep(1)
        data = data or run_eval.read_report(report)
        run_eval.stop(proc, profile)
    if data is None:
        sys.exit(f"No report after {int(time.time() - started)} s. "
                 f"See {out / 'setup.log'}.")
    return data


def reset_memories(profile: Path) -> None:
    """Removes what Dreaming wrote. The browser must not run."""
    db = sqlite3.connect(profile / "Default" / "AIChat")
    with db:
        for table in MEMORY_TABLES:
            db.execute(f"DELETE FROM {table}")
    chats = db.execute("SELECT COUNT(*) FROM conversation").fetchone()[0]
    db.close()
    run_eval.log(f"Removed the learned memories. {chats} chats stay.")


def set_last_dreaming_time(profile: Path) -> None:
    """Moves the daily timer one day away, so that it does not run before the
    user selects "Dream now"."""
    path = profile / "Default" / "Preferences"
    prefs = json.loads(path.read_text())
    prefs.setdefault("brave", {}).setdefault("ai_chat", {})[
        LAST_DREAMING_PREF] = chrome_time_now()
    path.write_text(json.dumps(prefs))


def launch_command(args, profile: Path) -> list:
    return [
        args.brave, f"--user-data-dir={profile}",
        "--no-first-run", "--no-default-browser-check",
        "--use-mock-keychain", "--password-store=basic",
        "--component-updater=url-source=http://127.0.0.1:9/",
        run_eval.features_switch(args),
        "--enable-logging=stderr",
        "--vmodule=user_memory_manager=1,dreaming_run=1",
        SETTINGS_URL,
    ]


def write_launch_script(args, out: Path) -> Path:
    """A script that loads the models and starts the browser."""
    profile = out / "profile"
    command = launch_command(args, profile)
    quoted = " \\\n  ".join(f"'{c}'" for c in command)
    setup = Path(__file__).resolve().with_name("ollama_setup.py")
    body = f"""#!/bin/sh
# Starts the demo browser. Loads the Ollama models first, so that
# "Dream now" does not wait for them.
python3 '{setup}' --ollama '{args.ollama}' --decision '{args.decision}' \\
  --llm '{args.llm}' --keep-alive '{args.keep_alive}' \\
  || echo 'Warning: the Ollama models are not loaded.' >&2
exec {quoted} 2> '{out / "browser.log"}'
"""
    script = out / "launch.sh"
    script.write_text(body)
    script.chmod(0o755)
    return script


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default="/tmp/learned_memory_demo",
                        help="folder for the profile and the launch script")
    parser.add_argument("--chats", default=str(HERE / "demo" / "demo_history.json"))
    parser.add_argument("--keep-memories", action="store_true",
                        help="keep what the first Dreaming run learned")
    parser.add_argument("--force", action="store_true",
                        help="replace an existing demo folder")
    parser.add_argument("--no-launch", action="store_true",
                        help="do not start the browser at the end")
    parser.add_argument("--brave", default=run_eval.DEFAULT_BRAVE)
    parser.add_argument("--source-profile", default=str(run_eval.DEFAULT_SOURCE_PROFILE))
    ollama_setup.add_arguments(parser)
    parser.add_argument("--threshold", type=float, default=0.8)
    parser.add_argument("--margin", type=float, default=0.2)
    parser.add_argument("--time-limit", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=1200)
    args = parser.parse_args()
    args.brave = str(args.brave)

    out = Path(args.out).expanduser().resolve()
    if out.exists():
        if not args.force:
            sys.exit(f"{out} exists. Use --force to replace it.")
        if not (out / MARKER).exists():
            sys.exit(f"{out} is not a demo folder (no {MARKER}). "
                     "Remove it by hand.")
        subprocess.run(["pkill", "-f", f"--user-data-dir={out / 'profile'}"],
                       capture_output=True)
        shutil.rmtree(out)
    run_eval.preflight(args)
    run_eval.prepare_ollama(args)
    out.mkdir(parents=True)
    (out / MARKER).write_text("Made by make_demo_profile.py\n")

    report = run_import(args, out)
    profile = out / "profile"
    if not args.keep_memories:
        reset_memories(profile)
    set_last_dreaming_time(profile)
    script = write_launch_script(args, out)

    run_eval.log(f"Demo profile ready: {profile}")
    print(f"\nStart it with:  {script}\n"
          f"Then open:      {SETTINGS_URL}\n"
          f"Browser log:    {out / 'browser.log'}")
    if not args.no_launch:
        run_eval.log("Starting the browser")
        subprocess.Popen([str(script)], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL, start_new_session=True)


if __name__ == "__main__":
    main()
