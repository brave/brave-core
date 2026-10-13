"""The run manifest (plan, section 9) and the run folder.

Every run writes manifest.json. A run without it is not a result.
"""

from __future__ import annotations

import datetime
import hashlib
import json
import platform
import re
import socket
import subprocess
import sys
from pathlib import Path

from . import clients

EVAL_ROOT = Path(__file__).resolve().parents[1]
REPO = EVAL_ROOT.parents[1]  # src/brave


def new_run_dir(root: Path, tag: str) -> tuple:
    """Makes <root>/<YYYYMMDD-HHMMSS>-<tag>/ and returns (run_id, path)."""
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "-", tag or "run").strip("-") or "run"
    run_id = f"{stamp}-{safe}"
    path = root / run_id
    n = 1
    while path.exists():
        n += 1
        run_id = f"{stamp}-{safe}-{n}"
        path = root / run_id
    path.mkdir(parents=True)
    return run_id, path


def _git(*args: str) -> str:
    try:
        return subprocess.run(
            ["git", "-C", str(REPO), *args],
            capture_output=True,
            text=True,
            timeout=30,
        ).stdout
    except (OSError, subprocess.SubprocessError):
        return ""


def git_info() -> dict:
    # Porcelain lines are "XY path". Do not strip them: X can be a space.
    dirty = [
        line[3:]
        for line in _git("status", "--porcelain").splitlines()
        if line.strip()
    ]
    return {
        "sha": _git("rev-parse", "HEAD").strip(),
        "branch": _git("rev-parse", "--abbrev-ref", "HEAD").strip(),
        "dirty": dirty,
    }


def machine_info() -> dict:
    ram = None
    try:
        if sys.platform == "darwin":
            ram = int(
                subprocess.run(
                    ["sysctl", "-n", "hw.memsize"],
                    capture_output=True,
                    text=True,
                ).stdout.strip()
            )
        else:
            for line in Path("/proc/meminfo").read_text().splitlines():
                if line.startswith("MemTotal:"):
                    ram = int(line.split()[1]) * 1024
    except (OSError, ValueError):
        pass
    return {
        "host": socket.gethostname(),
        "os": platform.platform(),
        "python": platform.python_version(),
        "ram_gb": round(ram / 2**30, 1) if ram else None,
    }


def ollama_info(ollama: str, models: dict) -> dict:
    """|models| maps a role (decision, llm, ...) to a model name."""
    info = {"url": ollama, "version": None, "models": []}
    try:
        info["version"] = clients.http_json(
            f"{ollama}/api/version", timeout=10
        ).get("version")
        tags = {
            m["name"]: m
            for m in clients.http_json(f"{ollama}/api/tags", timeout=10).get(
                "models", []
            )
        }
        loaded = {
            m["name"]: m
            for m in clients.http_json(f"{ollama}/api/ps", timeout=10).get(
                "models", []
            )
        }
    except Exception as e:  # The manifest must not stop a run.
        info["error"] = str(e)
        return info
    for role, name in models.items():
        if not name:
            continue
        tag = tags.get(name, {})
        ps = loaded.get(name, {})
        info["models"].append(
            {
                "role": role,
                "name": name,
                "digest": tag.get("digest"),
                "size": tag.get("size"),
                "loaded": bool(ps),
                "size_vram": ps.get("size_vram"),
                "context_length": ps.get("context_length"),
            }
        )
    return info


def dataset_info(path: Path) -> dict:
    data = path.read_bytes()
    if path.suffix == ".jsonl":
        count = sum(1 for line in data.decode().splitlines() if line.strip())
    else:
        try:
            parsed = json.loads(data)
            count = (
                len(parsed.get("chats", []))
                if isinstance(parsed, dict)
                else len(parsed)
            )
        except json.JSONDecodeError:
            count = None
    try:
        rel = str(path.resolve().relative_to(EVAL_ROOT))
    except ValueError:
        rel = str(path)
    return {
        "path": rel,
        "version": dataset_version(),
        "sha256": hashlib.sha256(data).hexdigest(),
        "cases": count,
    }


def dataset_version() -> str | None:
    """The version line of datasets/DATASETS.md: "Version: <v>"."""
    card = EVAL_ROOT / "datasets" / "DATASETS.md"
    if not card.exists():
        return None
    m = re.search(r"^Version:\s*(\S+)", card.read_text(), re.M)
    return m.group(1) if m else None


def build_info(brave: str | None) -> dict | None:
    if not brave:
        return None
    p = Path(brave)
    return {
        "path": str(p),
        "mtime": datetime.datetime.fromtimestamp(p.stat().st_mtime).isoformat(
            timespec="seconds"
        )
        if p.exists()
        else None,
    }


def write_manifest(
    run_dir: Path,
    *,
    run_id: str,
    tag: str,
    ollama: str,
    models: dict,
    params: dict,
    variants: dict,
    datasets: list,
    repeats: int,
    cache: bool,
    brave: str | None = None,
    extra: dict | None = None,
) -> dict:
    manifest = {
        "run_id": run_id,
        "tag": tag,
        "started": datetime.datetime.now().isoformat(timespec="seconds"),
        "command": " ".join(sys.argv),
        "git": git_info(),
        "build": build_info(brave),
        "ollama": ollama_info(ollama, models),
        "params": params,
        "variants": variants,
        "datasets": [dataset_info(Path(d)) for d in datasets],
        "repeats": repeats,
        "cache": cache,
        "machine": machine_info(),
    }
    manifest.update(extra or {})
    (run_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False)
    )
    return manifest
