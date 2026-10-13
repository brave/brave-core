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

import eval_proxy
import ollama_setup
import report as report_lib
from lib import clients, manifest

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
# The feature params that the harness sets, with the C++ defaults
# (features.cc). (flag name, param name, type, default)
PARAMS = [
    ("threshold", "certain_threshold", float, 0.8),
    ("margin", "certain_margin", float, 0.2),
    ("gate_threshold", "gate_threshold", float, 0.2),
    ("fact_threshold", "fact_threshold", float, 0.6),
    ("max_sensitive", "max_sensitive", float, 0.3),
    ("max_instruction", "max_instruction", float, 0.25),
    ("max_short_lived", "max_short_lived", float, 0.5),
    ("max_neighbors", "max_neighbors", int, 5),
    ("min_neighbor_similarity", "min_neighbor_similarity", float, 0.5),
    ("max_relation_requests", "max_relation_requests", int, 20),
    ("max_rewrite_requests", "max_rewrite_requests", int, 50),
]


def log(message: str) -> None:
    print(f"[{time.strftime('%H:%M:%S')}] {message}", flush=True)


def load_sets(names: list | None, files: list | None = None, split: str = "dev") -> list:
    """Returns (path, chat set) pairs: the named sets in datasets/e2e/<split>/,
    or the files of --chat-set."""
    if files:
        return [(Path(f).resolve(), json.loads(Path(f).read_text())) for f in files]
    sets = []
    for path in sorted((HERE / "datasets" / "e2e" / split).glob("*.json")):
        if names and path.stem not in names:
            continue
        sets.append((path, json.loads(path.read_text())))
    if names:
        missing = set(names) - {s["name"] for _, s in sets}
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


def model_url(args) -> str:
    """Where the browser sends the model requests: Ollama, or the proxy."""
    return args.proxy_url or args.ollama


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
            "endpoint_url": f"{model_url(args)}/v1/chat/completions",
            "api_key": "", "context_size": 32768, "supports_tools": False,
            "vision_support": False,
        }],
    }}}
    (profile / "Default" / "Preferences").write_text(json.dumps(prefs, indent=2))


def feature_params(args) -> dict:
    params = {name: getattr(args, flag) for flag, name, _, _ in PARAMS}
    params.update({
        "run_time_limit": f"{args.time_limit}s",
        "decision_model_name": args.decision,
        "decision_model_url": f"{model_url(args)}/v1/systemone",
        "local_llm_model_name": args.llm,
    })
    return params


def features_switch(args) -> str:
    params = feature_params(args)
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


def run_set(args, chat_set: dict, out_dir: Path, proxy=None, repeat: int = 1):
    """Runs one repeat of one set in a new profile, in <out>/<set>/r<repeat>/."""
    name = chat_set["name"]
    if proxy:
        proxy.label = f"{name}/r{repeat}"
    set_dir = out_dir / name / f"r{repeat}"
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

    name = f"{name} r{repeat}"
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
        chat_set, data, {"wall_seconds": round(time.time() - started), "repeat": repeat,
                         "report": f"{chat_set['name']}/r{repeat}/report.md"})
    (set_dir / "report.md").write_text(markdown)
    gold = (f", recall {summary['gold_found']}/{summary['gold_required']}, "
            f"{summary['leaks']} leaks, {summary['wrong_ops']} wrong ops"
            if "recall" in summary else "")
    log(f"{name}: {summary['status']}, {summary['memories']} memories, "
        f"checks {summary['checks_passed']}/{summary['checks_total']}, "
        f"{summary['forbidden_hits']} forbidden hits{gold}")
    config = data.get("config", {})
    # report.json names max_rewrite_requests "max_writing_requests".
    keys = {"max_rewrite_requests": "max_writing_requests"}
    missing = [n for _, n, _, _ in PARAMS if keys.get(n, n) not in config]
    if "error" not in data and missing:
        log(f"{name}: WARNING: report.json has no {', '.join(missing)}. "
            "The build is older than the harness, so these params had no effect.")
    if not args.keep_profiles:
        shutil.rmtree(profile, ignore_errors=True)
    return summary


def report_only(out_dir: Path, sets_dir: Path | None = None) -> None:
    """Writes report.md and summary.md again. |sets_dir| gives newer chat sets
    (for example new gold) for the old runs, by set name."""
    summaries = []
    # <set>/r<k>/report.json, or <set>/report.json of older runs.
    paths = sorted(out_dir.glob("*/r*/report.json")) or sorted(out_dir.glob("*/report.json"))
    for path in paths:
        chat_set = json.loads((path.parent / "chats.json").read_text())
        if sets_dir and (sets_dir / f"{chat_set['name']}.json").exists():
            chat_set = json.loads((sets_dir / f"{chat_set['name']}.json").read_text())
        data = json.loads(path.read_text())
        repeat = int(path.parent.name[1:]) if path.parent.name[:1] == "r" and path.parent.name[1:].isdigit() else 1
        rel = path.parent.relative_to(out_dir)
        markdown, summary = report_lib.render(chat_set, data, {"repeat": repeat,
                                                               "report": f"{rel}/report.md"})
        (path.parent / "report.md").write_text(markdown)
        summaries.append(summary)
    (out_dir / "summary.md").write_text(
        report_lib.render_summary(summaries, {"time": out_dir.name}))
    log(f"Wrote reports for {len(summaries)} sets in {out_dir}")


def judge_extras(args, out_dir: Path, sets_dir: Path | None = None) -> None:
    """Sorts the extra memories of a run with a judge agent (plan 8.2, P2.7) and
    adds a section to summary.md."""
    from lib import judge as judge_lib
    spec = judge_lib.judge_spec(args.judge, args.judge_model, args.judge_thinking)
    inputs = judge_lib.e2e_extras(out_dir, sets_dir)
    if not inputs:
        log("judge: the run has no extra memories")
        return
    verdicts = judge_lib.judge("extras", inputs, spec, clients.results_root(args.results), log=log)
    metrics = judge_lib.extras_metrics(verdicts)
    (out_dir / "extras_judge.json").write_text(json.dumps(
        {"judge": judge_lib.meta("extras", spec), "metrics": metrics,
         "rows": [{"name": n, "input": inputs[n], "verdict": v} for n, v in verdicts.items()]},
        indent=2, ensure_ascii=False))
    j = judge_lib.meta("extras", spec)
    md = ["", "## Extras sorted by a judge", "",
          f"Judge: {j['role']} ({j['model']}, {j['thinking']}, rubric v{j['rubric_version']}). "
          f"{metrics['n']} extra memories. `true_unlabeled` memories are true: add them to the gold as "
          "optional facts. The other classes are errors.", "",
          "| " + " | ".join(judge_lib.EXTRA_CLASSES) + " |", "|" + "---|" * len(judge_lib.EXTRA_CLASSES),
          "| " + " | ".join(str(metrics["classes"][c]) for c in judge_lib.EXTRA_CLASSES) + " |", ""]
    for name, v in verdicts.items():
        md.append(f"- {name}: `{v['class']}`: \"{inputs[name]['memory']}\" ({v['reason']})")
    summary = out_dir / "summary.md"
    text = summary.read_text() if summary.exists() else ""
    text = text.split("\n## Extras sorted by a judge")[0].rstrip("\n") + "\n"
    summary.write_text(text + "\n".join(md) + "\n")
    log(f"judge: extras {metrics['classes']}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sets", nargs="*", help="names of chat sets (default: all)")
    parser.add_argument("--chat-set", nargs="*", metavar="FILE",
                        help="chat set files to run instead of datasets/e2e/, for "
                             "example datasets/parity/parity.json")
    parser.add_argument("--split", choices=["dev", "holdout"], default="dev",
                        help="datasets/e2e/<split>/ (default dev). Run the holdout "
                             "only for a final candidate")
    parser.add_argument("--list", action="store_true", help="list the chat sets")
    parser.add_argument("--brave", default=str(DEFAULT_BRAVE))
    parser.add_argument("--out", default=None,
                        help="output folder (default: <results root>/<time>-<tag>)")
    parser.add_argument("--results", default=None,
                        help=f"results root (default: $LM_EVAL_RUNS or "
                             f"{clients.DEFAULT_RESULTS_ROOT})")
    parser.add_argument("--tag", default="e2e", help="short name of the run")
    parser.add_argument("--proxy", action="store_true",
                        help="send the model requests through eval_proxy.py and "
                             "record them in <out>/proxy.jsonl")
    parser.add_argument("--proxy-port", type=int, default=eval_proxy.DEFAULT_PORT)
    parser.add_argument("--source-profile", default=str(DEFAULT_SOURCE_PROFILE))
    ollama_setup.add_arguments(parser)
    for flag, name, kind, default in PARAMS:
        parser.add_argument("--" + flag.replace("_", "-"), dest=flag, type=kind,
                            default=default, help=f"feature param {name} (default {default})")
    parser.add_argument("--time-limit", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=1200)
    parser.add_argument("--keep-profiles", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--report-only", metavar="DIR", default=None,
                        help="make new report.md files from an earlier run")
    parser.add_argument("--gold-from", metavar="DIR", default=None,
                        help="with --report-only: score with the chat sets of this folder "
                             "(for example datasets/e2e/dev), not the copy in the run")
    parser.add_argument("--judge", choices=["fast", "final"], default=None,
                        help="sort the extra memories with a judge agent (fast: Sonnet, final: Opus)")
    parser.add_argument("--judge-model", default=None)
    parser.add_argument("--judge-thinking", default=None)
    parser.add_argument("--repeats", type=int, default=1,
                        help="run each set this many times, each in a new profile")
    args = parser.parse_args()

    if args.report_only:
        report_only(Path(args.report_only), Path(args.gold_from) if args.gold_from else None)
        if args.judge:
            judge_extras(args, Path(args.report_only), Path(args.gold_from) if args.gold_from else None)
        return
    sets = load_sets(args.sets, args.chat_set, args.split)
    if args.list:
        for _, s in sets:
            print(f'{s["name"]:20} {len(s["chats"]):2} chats  {s.get("description", "")}')
        return
    if not args.dry_run:
        preflight(args)
        prepare_ollama(args)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    if args.out:
        out_dir, run_id = Path(args.out), Path(args.out).name
        out_dir.mkdir(parents=True, exist_ok=True)
    else:
        run_id, out_dir = manifest.new_run_dir(clients.results_root(args.results), args.tag)
    log(f"Output: {out_dir}")

    proxy = None
    args.proxy_url = None
    if args.proxy:
        proxy = eval_proxy.RecordingProxy(out_dir / "proxy.jsonl", args.ollama,
                                          args.proxy_port).start()
        args.proxy_url = proxy.url
        log(f"Proxy: {proxy.url} -> {args.ollama}, records in {proxy.log_path}")
    manifest.write_manifest(
        out_dir, run_id=run_id, tag=args.tag, ollama=args.ollama,
        models={"decision": args.decision, "llm": args.llm},
        params=feature_params(args), variants={"all": "current (C++ build)"},
        datasets=[str(p) for p, _ in sets], repeats=args.repeats, cache=False, brave=args.brave,
        extra={"level": "L2", "proxy": bool(args.proxy)})

    summaries = []
    try:
        for _, chat_set in sets:
            for repeat in range(1, args.repeats + 1):
                summary = run_set(args, chat_set, out_dir, proxy, repeat)
                if summary:
                    summaries.append(summary)
    finally:
        if proxy:
            proxy.stop()
            log("Proxy calls: " + ", ".join(
                f"{k} {v}" for k, v in sorted(proxy.counts.items())))
    if summaries:
        meta = {"time": stamp, "decision": args.decision, "llm": args.llm,
                "threshold": args.threshold, "margin": args.margin}
        (out_dir / "summary.md").write_text(report_lib.render_summary(summaries, meta))
        (out_dir / "summary.json").write_text(json.dumps(summaries, indent=2))
        if args.judge:
            judge_extras(args, out_dir)
        print("\n" + (out_dir / "summary.md").read_text())


if __name__ == "__main__":
    main()
