#!/usr/bin/env python3
"""Loads the two Ollama models of learned memory into memory, and keeps them there.

The models must be installed already (`ollama pull`). Ollama unloads a model
after 5 minutes without a request, and then the next Dreaming run or chat turn
waits many seconds while the model loads. This script sends one small request to
each model with a long `keep_alive`, so that both stay loaded. Ollama keeps the
`keep_alive` of the first request for the requests that have none, so the
requests of the browser do not shorten it.

  ./ollama_setup.py                 # load both models, keep them for 60 minutes
  ./ollama_setup.py --status        # show what is loaded, load nothing
  ./ollama_setup.py --keep-alive 3h
  ./ollama_setup.py --unload        # unload both models

The harness scripts (run_eval.py, make_demo_profile.py, decision_eval.py,
relevance_eval.py) call this module before they use the models.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
import urllib.error
import urllib.request

DEFAULT_OLLAMA = "http://localhost:11434"
DEFAULT_DECISION = "clef-flash:9b"
DEFAULT_LLM = "qwen3.5:9b"
DEFAULT_KEEP_ALIVE = "60m"


class OllamaSetupError(Exception):
    pass


def _request(url: str, body: dict | None = None, timeout: int = 900):
    data = json.dumps(body).encode() if body is not None else None
    request = urllib.request.Request(
        url, data, {"Content-Type": "application/json"} if data else {})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        raise OllamaSetupError(
            f"{url}: HTTP {error.code} {error.read()[:200].decode(errors='replace')}")
    except (urllib.error.URLError, OSError) as error:
        raise OllamaSetupError(f"{url}: {error}")


def installed_models(ollama: str) -> set:
    try:
        tags = _request(f"{ollama}/api/tags", timeout=10)
    except OllamaSetupError as error:
        raise OllamaSetupError(
            f"Ollama is not running at {ollama}. Start the Ollama app. ({error})")
    return {m["name"] for m in tags.get("models", [])}


def loaded_models(ollama: str) -> list:
    """The models in memory, with the size and the time when Ollama unloads them."""
    return _request(f"{ollama}/api/ps", timeout=10).get("models", [])


def load_decision_model(ollama: str, model: str, keep_alive: str) -> None:
    # The decision model supports only the System One API, not /api/generate.
    _request(f"{ollama}/v1/systemone", {
        "model": model, "state": "warm up", "keep_alive": keep_alive,
        "questions": {"q": {"type": "noul", "instructions": "Is this a test?"}}})


def load_llm(ollama: str, model: str, keep_alive: str) -> None:
    # An empty prompt loads the model and generates nothing.
    _request(f"{ollama}/api/generate",
             {"model": model, "prompt": "", "keep_alive": keep_alive})


def unload(ollama: str, model: str) -> None:
    try:
        _request(f"{ollama}/api/generate", {"model": model, "keep_alive": 0})
    except OllamaSetupError:
        # The decision model does not support /api/generate.
        _request(f"{ollama}/v1/systemone", {
            "model": model, "state": "x", "keep_alive": 0,
            "questions": {"q": {"type": "noul", "instructions": "Is this a test?"}}})


def format_status(ollama: str, models: list) -> str:
    running = {m["name"]: m for m in loaded_models(ollama)}
    lines = []
    for name in models:
        info = running.get(name)
        if not info:
            lines.append(f"  {name:20} not loaded")
            continue
        size = info.get("size", 0) / 1e9
        vram = info.get("size_vram", 0)
        gpu = f"{100 * vram // max(info.get('size', 1), 1)}% on GPU"
        lines.append(f"  {name:20} loaded, {size:.1f} GB, {gpu}, "
                     f"unloads at {info.get('expires_at', '?')[11:19]}")
    return "\n".join(lines)


def check_installed(ollama: str, models: list) -> None:
    """Raises OllamaSetupError when Ollama does not run or a model is missing."""
    installed = installed_models(ollama)
    missing = [m for m in models if m and m not in installed]
    if missing:
        raise OllamaSetupError(
            f"Not installed in Ollama: {', '.join(missing)}. Check `ollama list`.")


def load_models(ollama: str = DEFAULT_OLLAMA, decision: str | None = DEFAULT_DECISION,
                llm: str | None = DEFAULT_LLM, keep_alive: str = DEFAULT_KEEP_ALIVE,
                log=lambda message: print(message, flush=True)) -> None:
    """Loads |decision| and |llm| (each can be None) and keeps them loaded.

    Raises OllamaSetupError when Ollama does not run or a model is not
    installed.
    """
    wanted = [m for m in (decision, llm) if m]
    check_installed(ollama, wanted)
    for model, load in ((decision, load_decision_model), (llm, load_llm)):
        if not model:
            continue
        start = time.time()
        load(ollama, model, keep_alive)
        log(f"Loaded {model} in {time.time() - start:.1f} s")
    log(f"Ollama models (kept for {keep_alive}):\n{format_status(ollama, wanted)}")


def add_arguments(parser: argparse.ArgumentParser) -> None:
    """The options that the harness scripts share."""
    parser.add_argument("--ollama", default=DEFAULT_OLLAMA)
    parser.add_argument("--decision", default=DEFAULT_DECISION,
                        help="decision model name")
    parser.add_argument("--llm", default=DEFAULT_LLM, help="local LLM name")
    parser.add_argument("--keep-alive", default=DEFAULT_KEEP_ALIVE,
                        help="how long Ollama keeps the loaded models (default 60m)")
    parser.add_argument("--no-warmup", action="store_true",
                        help="do not load the Ollama models before the run")


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    add_arguments(parser)
    parser.add_argument("--status", action="store_true",
                        help="show what is loaded and load nothing")
    parser.add_argument("--unload", action="store_true", help="unload both models")
    args = parser.parse_args()
    models = [args.decision, args.llm]
    try:
        check_installed(args.ollama, models)
        if args.status:
            print(format_status(args.ollama, models))
        elif args.unload:
            for model in models:
                unload(args.ollama, model)
            print(format_status(args.ollama, models))
        else:
            load_models(args.ollama, args.decision, args.llm, args.keep_alive)
    except OllamaSetupError as error:
        sys.exit(str(error))


if __name__ == "__main__":
    main()
