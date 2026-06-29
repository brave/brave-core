#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generate a compact GN dependency-frequency repo map.

This tool uses one inspectable GN query per reached target. It walks reachable
direct GN dependencies from one or more product roots, excludes test-only
targets, counts inbound direct-dependency references, rolls labels up to coarse
repo areas, and writes compact Markdown for agents.
"""

import argparse
import collections
import datetime
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
from typing import Dict, Iterable, List, NamedTuple, Sequence, Set, Tuple

DEFAULT_TOP_AREAS = 100
DEFAULT_TOP_TARGETS_PER_AREA = 12
DEFAULT_EXCLUDED_PREFIXES = (
    "//out/",
    "//node_modules/",
    "//vendor/",
    "//brave/vendor/",
)
THIRD_PARTY_PREFIXES = ("//third_party", "//brave/third_party")
BLINK_PREFIX = "//third_party/blink"


class TargetInfo(NamedTuple):
    label: str
    deps: List[str]
    testonly: bool
    sources: List[str]


class AreaInfo(NamedTuple):
    area: str
    count: int
    target_count: int
    top_targets: List[Tuple[str, int]]
    common_paths: List[str]
    path: str


def src_root_from_cwd(cwd: pathlib.Path) -> pathlib.Path:
    """Return Chromium src root when cwd is src or src/brave."""
    cwd = cwd.resolve()
    if (cwd / "brave" / "BUILD.gn").exists() and (cwd / "build").exists():
        return cwd
    if cwd.name == "brave" and (cwd / "BUILD.gn").exists():
        return cwd.parent
    if (cwd / "BUILD.gn").exists() and (cwd.parent / "build").exists():
        return cwd.parent
    return cwd


def brave_root_from_src(src_root: pathlib.Path) -> pathlib.Path:
    brave_root = src_root / "brave"
    if brave_root.exists():
        return brave_root
    return src_root


def default_out_dir(cwd: pathlib.Path, src_root: pathlib.Path) -> pathlib.Path:
    candidates = []
    if cwd.name == "brave":
        candidates.extend([
            cwd.parent / "out" / "current_link",
            cwd.parent / "out" / "Component_arm64",
        ])
    candidates.extend([
        src_root / "out" / "current_link",
        src_root / "out" / "Component_arm64",
    ])
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return src_root / "out" / "current_link"


def default_output_dir(cwd: pathlib.Path,
                       src_root: pathlib.Path) -> pathlib.Path:
    if cwd.name == "brave":
        return cwd / ".repo-map"
    return brave_root_from_src(src_root) / ".repo-map"


def default_gn_binary(src_root: pathlib.Path) -> str:
    candidates = [
        src_root / "buildtools" / "mac" / "gn",
        src_root / "buildtools" / "linux64" / "gn",
        src_root / "buildtools" / "win" / "gn.exe",
        src_root / "third_party" / "depot_tools" / "gn",
        src_root / "brave" / "vendor" / "depot_tools" / "gn",
    ]
    for candidate in candidates:
        if candidate.exists() and os.access(candidate, os.X_OK):
            return str(candidate)
    found = shutil.which("gn")
    if found:
        return found
    return "gn"


def normalize_out_dir(path: str, cwd: pathlib.Path,
                      src_root: pathlib.Path) -> str:
    out_path = pathlib.Path(path).expanduser()
    if not out_path.is_absolute():
        out_path = (cwd / out_path).resolve()
    try:
        return str(out_path.relative_to(src_root))
    except ValueError:
        return str(out_path)


def normalize_label(label: str) -> str:
    label = label.strip()
    if not label:
        raise ValueError("empty GN label")
    if label.startswith(":"):
        # Keep cwd-relative labels unsupported/explicit for reproducibility.
        raise ValueError(f"root label must be absolute, got {label!r}")
    if not label.startswith("//"):
        label = f"//{label}"
    return label


def strip_toolchain(label: str) -> str:
    if "(" in label:
        return label.split("(", 1)[0]
    return label


def label_dir(label: str) -> str:
    label = strip_toolchain(label)
    if not label.startswith("//"):
        return ""
    body = label[2:]
    if ":" in body:
        return body.split(":", 1)[0]
    return body


def sanitize_area_filename(area: str) -> str:
    name = area.strip("/").replace("/", "__") or "root"
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name) + ".md"


def parse_rollup_rules(values: Sequence[str]) -> List[Tuple[str, int]]:
    default_rules = [
        ("brave/components", 3),
        ("brave", 2),
        ("components", 2),
        ("chrome", 2),
        ("content", 2),
        ("ui", 2),
        ("services", 2),
        ("net", 1),
        ("base", 1),
        ("url", 1),
    ]
    custom_rules = []
    for value in values:
        if "=" not in value:
            raise ValueError(f"--rollup must be PREFIX=DEPTH, got {value!r}")
        prefix, depth = value.split("=", 1)
        prefix = prefix.strip().strip("/")
        if prefix.startswith("//"):
            prefix = prefix[2:]
        custom_rules.append((prefix, int(depth)))
    return custom_rules + default_rules


def rollup_area(label: str, rules: Sequence[Tuple[str, int]]) -> str:
    directory = label_dir(label)
    if not directory:
        return "root"
    parts = directory.split("/")
    for prefix, depth in rules:
        prefix_parts = prefix.split("/") if prefix else []
        if parts[:len(prefix_parts)] == prefix_parts:
            return "/".join(parts[:max(1, min(depth, len(parts)))])
    return parts[0]


def is_excluded_label(label: str, include_third_party: bool,
                      include_blink: bool) -> bool:
    label = strip_toolchain(label)
    if any(label.startswith(prefix) for prefix in DEFAULT_EXCLUDED_PREFIXES):
        return True
    if label == BLINK_PREFIX or label.startswith(
            f"{BLINK_PREFIX}/") or label.startswith(f"{BLINK_PREFIX}:"):
        return not include_blink
    if any(label == prefix or label.startswith(f"{prefix}/")
           or label.startswith(f"{prefix}:")
           for prefix in THIRD_PARTY_PREFIXES):
        return not include_third_party
    return False


def run_gn_desc(gn: str, src_root: pathlib.Path, out_dir: str,
                label: str) -> str:
    cmd = [gn, "desc", out_dir, label, "--format=json"]
    proc = subprocess.run(
        cmd,
        cwd=str(src_root),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError("GN query failed:\n"
                           f"  command: {' '.join(cmd)}\n"
                           f"  cwd: {src_root}\n"
                           f"  stdout: {proc.stdout.strip()}\n"
                           f"  stderr: {proc.stderr.strip()}")
    return proc.stdout


def parse_json_value(raw: str, fallback):
    raw = raw.strip()
    if not raw:
        return fallback
    return json.loads(raw)


def parse_target_description(raw: str, query_label: str) -> Dict[str, object]:
    """Return the target metadata object from `gn desc --format=json` output."""
    value = parse_json_value(raw, {})
    if not isinstance(value, dict):
        raise RuntimeError(
            f"Unexpected GN JSON for {query_label}: expected object, got "
            f"{type(value).__name__}")

    # GN returns either the target object directly or an object keyed by label,
    # depending on the GN version/pattern used. Accept both shapes.
    if any(key in value for key in ("deps", "testonly", "sources", "type")):
        return value

    canonical_label = strip_toolchain(query_label)
    for key in (query_label, canonical_label):
        target_value = value.get(key)
        if isinstance(target_value, dict):
            return target_value
    if len(value) == 1:
        only_value = next(iter(value.values()))
        if isinstance(only_value, dict):
            return only_value
    raise RuntimeError(f"Unexpected GN JSON shape for {query_label}")


def normalize_dep_label(label: str) -> str:
    # Keep explicit toolchains distinct while traversing/counting. Area rollups
    # and Markdown target summaries strip toolchains later for compactness.
    return normalize_label(label)


def unique_labels(labels: Iterable[str]) -> List[str]:
    return list(dict.fromkeys(labels))


def desc_target(
    gn: str,
    src_root: pathlib.Path,
    out_dir: str,
    query_label: str,
    canonical_label: str,
) -> Tuple[TargetInfo, List[str]]:
    desc = parse_target_description(
        run_gn_desc(gn, src_root, out_dir, query_label), query_label)
    raw_deps = [str(dep) for dep in desc.get("deps", [])]
    sources = [str(source) for source in desc.get("sources", [])]
    return (
        TargetInfo(
            label=canonical_label,
            deps=unique_labels(normalize_dep_label(dep) for dep in raw_deps),
            testonly=bool(desc.get("testonly", False)),
            sources=sources,
        ),
        raw_deps,
    )


def walk_targets(
    gn: str,
    src_root: pathlib.Path,
    out_dir: str,
    roots: Sequence[str],
    include_third_party: bool,
    include_blink: bool,
) -> Dict[str, TargetInfo]:
    targets: Dict[str, TargetInfo] = {}
    queue = collections.deque(roots)
    queued: Set[str] = {normalize_dep_label(root) for root in roots}

    while queue:
        query_label = queue.popleft()
        canonical_label = normalize_dep_label(query_label)
        if canonical_label in targets:
            continue
        if is_excluded_label(canonical_label, include_third_party,
                             include_blink):
            continue
        info, raw_deps = desc_target(gn, src_root, out_dir, query_label,
                                     canonical_label)
        targets[canonical_label] = info
        if info.testonly:
            continue
        for dep in raw_deps:
            canonical_dep = normalize_dep_label(dep)
            if (canonical_dep not in queued and not is_excluded_label(
                    canonical_dep, include_third_party, include_blink)):
                queued.add(canonical_dep)
                queue.append(dep)
    return targets


def count_inbound_references(
        targets: Dict[str, TargetInfo]) -> collections.Counter:
    counts = collections.Counter()
    for info in targets.values():
        if info.testonly:
            continue
        for dep in info.deps:
            dep_info = targets.get(dep)
            if dep_info and not dep_info.testonly:
                counts[dep] += 1
    return counts


def common_source_dirs(targets: Iterable[TargetInfo],
                       limit: int = 6) -> List[str]:
    counts = collections.Counter()
    for info in targets:
        for source in info.sources:
            if not source.startswith("//"):
                continue
            directory = source[2:].rsplit(
                "/", 1)[0] if "/" in source[2:] else source[2:]
            if directory:
                counts[directory] += 1
    return [path for path, _ in counts.most_common(limit)]


def build_area_infos(
    targets: Dict[str, TargetInfo],
    inbound_counts: collections.Counter,
    rollup_rules: Sequence[Tuple[str, int]],
    top_areas: int,
    top_targets_per_area: int,
) -> List[AreaInfo]:
    area_targets: Dict[str, List[TargetInfo]] = collections.defaultdict(list)
    area_counts = collections.Counter()
    area_target_counts: Dict[str,
                             collections.Counter] = collections.defaultdict(
                                 collections.Counter)

    for label, info in targets.items():
        if info.testonly:
            continue
        area = rollup_area(label, rollup_rules)
        area_targets[area].append(info)
        count = inbound_counts.get(label, 0)
        area_counts[area] += count
        if count:
            area_target_counts[area][strip_toolchain(label)] += count

    infos = []
    for area, count in area_counts.most_common():
        if count <= 0:
            continue
        infos.append(
            AreaInfo(
                area=area,
                count=count,
                target_count=len(area_targets[area]),
                top_targets=area_target_counts[area].most_common(
                    top_targets_per_area),
                common_paths=common_source_dirs(area_targets[area]),
                path=f"areas/{sanitize_area_filename(area)}",
            ))
        if len(infos) >= top_areas:
            break
    return infos


def prepare_output_dir(output: pathlib.Path) -> None:
    """Remove previously generated files that could otherwise go stale."""
    for child in (output / "areas", output / "data"):
        if child.exists():
            shutil.rmtree(child)
    default_md = output / "default.md"
    if default_md.exists():
        default_md.unlink()


def write_file(path: pathlib.Path, contents: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents, encoding="utf-8")


def write_markdown(
    output: pathlib.Path,
    area_infos: Sequence[AreaInfo],
    targets: Dict[str, TargetInfo],
    inbound_counts: collections.Counter,
    metadata: Dict[str, object],
) -> None:
    areas_dir = output / "areas"
    areas_dir.mkdir(parents=True, exist_ok=True)

    generated_at = metadata["generated_at"]
    root_targets = ", ".join(f"`{root}`" for root in metadata["roots"])
    lines = [
        "# Brave GN dependency frequency map",
        "",
        "Compact agent entry point: areas ranked by inbound direct GN deps.",
        "Not semantic ranking, ownership, PageRank, symbols, or git history.",
        "",
        "## Metadata",
        "",
        f"- Generated: `{generated_at}`",
        f"- GN output dir: `{metadata['out_dir']}`",
        f"- Roots: {root_targets}",
        f"- Non-testonly targets: `{metadata['non_testonly_targets']}`",
        f"- Testonly targets skipped: `{metadata['testonly_targets']}`",
        f"- Third-party included: `{metadata['include_third_party']}`; Blink included: `{metadata['include_blink']}`",
        "- Count: each direct non-testonly dep from a reachable non-testonly target adds 1.",
        "",
        "## Top depended-on areas",
        "",
        "| Rank | Area | Refs | Targets | Map |",
        "| ---: | --- | ---: | ---: | --- |",
    ]
    for index, info in enumerate(area_infos, start=1):
        lines.append(
            f"| {index} | `{info.area}` | {info.count} | {info.target_count} | [{info.path}]({info.path}) |"
        )

    lines.extend([
        "",
        "Read only the relevant per-area files, then inspect source as needed.",
        "",
    ])
    write_file(output / "default.md", "\n".join(lines))

    for info in area_infos:
        lines = [
            f"# {info.area}",
            "",
            f"- Direct dep refs: `{info.count}`",
            f"- Built non-testonly targets: `{info.target_count}`",
            "",
            "## Top GN targets contributing to this area",
            "",
        ]
        if info.top_targets:
            lines.extend(["| Target | Direct dep refs |", "| --- | ---: |"])
            for label, count in info.top_targets:
                lines.append(f"| `{label}` | {count} |")
        else:
            lines.append(
                "No inbound direct-dependency references were counted for individual targets in this area."
            )
        if info.common_paths:
            lines.extend(["", "## Common source/header directories", ""])
            for source_path in info.common_paths:
                lines.append(f"- `//{source_path}`")
        lines.append("")
        write_file(output / info.path, "\n".join(lines))


def write_json_data(
    output: pathlib.Path,
    area_infos: Sequence[AreaInfo],
    targets: Dict[str, TargetInfo],
    inbound_counts: collections.Counter,
    metadata: Dict[str, object],
) -> None:
    data_dir = output / "data"
    data_dir.mkdir(parents=True, exist_ok=True)
    dependency_frequency = {
        "metadata": {
            k: v
            for k, v in metadata.items() if k != "rollup_rules"
        },
        "areas": [info._asdict() for info in area_infos],
        "target_counts": dict(inbound_counts.most_common()),
    }
    target_data = {
        label: {
            "deps": [
                dep for dep in info.deps
                if dep in targets and not targets[dep].testonly
            ],
            "sources": info.sources,
            "inbound_count": inbound_counts.get(label, 0),
        }
        for label, info in sorted(targets.items()) if not info.testonly
    }
    write_file(
        data_dir / "dependency_frequency.json",
        json.dumps(dependency_frequency, indent=2, sort_keys=True) + "\n")
    write_file(data_dir / "targets.json",
               json.dumps(target_data, indent=2, sort_keys=True) + "\n")


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--out-dir",
        help="GN output dir. Defaults to out/current_link, falling back to "
        "out/Component_arm64 when present.")
    parser.add_argument(
        "--root",
        action="append",
        dest="roots",
        help="Root GN label to traverse. Repeatable. Defaults to //brave:brave."
    )
    parser.add_argument(
        "--output",
        help="Output directory. Defaults to brave/.repo-map from src or "
        ".repo-map from src/brave.")
    parser.add_argument("--top-areas",
                        type=int,
                        default=DEFAULT_TOP_AREAS,
                        help="Number of top areas to include in Markdown.")
    parser.add_argument("--top-targets-per-area",
                        type=int,
                        default=DEFAULT_TOP_TARGETS_PER_AREA,
                        help="Number of top targets listed in each area file.")
    parser.add_argument("--format",
                        choices=("markdown", "json", "both"),
                        default="both",
                        help="Output format to write.")
    parser.add_argument(
        "--include-third-party",
        action="store_true",
        help="Include //third_party targets. Excluded by default.")
    parser.add_argument(
        "--include-blink",
        action="store_true",
        help=
        "Include //third_party/blink targets. Excluded by default unless set.")
    parser.add_argument(
        "--rollup",
        action="append",
        default=[],
        metavar="PREFIX=DEPTH",
        help="Override/add area rollup rule, e.g. --rollup brave/components=3. "
        "Earlier rules win.")
    parser.add_argument(
        "--gn",
        help="Path to gn binary. Defaults to buildtools/depot_tools lookup.")
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    cwd = pathlib.Path.cwd()
    src_root = src_root_from_cwd(cwd)
    if args.out_dir:
        out_dir = normalize_out_dir(args.out_dir, cwd, src_root)
    else:
        out_dir = normalize_out_dir(str(default_out_dir(cwd, src_root)), cwd,
                                    src_root)
    roots = [
        normalize_label(root) for root in (args.roots or ["//brave:brave"])
    ]
    for root in roots:
        if is_excluded_label(root, args.include_third_party,
                             args.include_blink):
            print(
                f"Warning: root target {root} is excluded by the current filters",
                file=sys.stderr)
    output = (pathlib.Path(args.output).expanduser()
              if args.output else default_output_dir(cwd, src_root))
    if not output.is_absolute():
        output = (cwd / output).resolve()
    rollup_rules = parse_rollup_rules(args.rollup)
    gn = args.gn or default_gn_binary(src_root)

    targets = walk_targets(
        gn=gn,
        src_root=src_root,
        out_dir=out_dir,
        roots=roots,
        include_third_party=args.include_third_party,
        include_blink=args.include_blink,
    )
    inbound_counts = count_inbound_references(targets)
    area_infos = build_area_infos(
        targets=targets,
        inbound_counts=inbound_counts,
        rollup_rules=rollup_rules,
        top_areas=args.top_areas,
        top_targets_per_area=args.top_targets_per_area,
    )
    prepare_output_dir(output)
    metadata = {
        "generated_at": datetime.datetime.now(
            datetime.timezone.utc).isoformat(timespec="seconds"),
        "src_root": str(src_root),
        "out_dir": out_dir,
        "roots": roots,
        "include_third_party": args.include_third_party,
        "include_blink": args.include_blink,
        "rollup_rules": rollup_rules,
        "total_targets": len(targets),
        "non_testonly_targets": sum(1 for info in targets.values()
                                    if not info.testonly),
        "testonly_targets": sum(1 for info in targets.values()
                                if info.testonly),
    }

    if args.format in ("markdown", "both"):
        write_markdown(output, area_infos, targets, inbound_counts, metadata)
    if args.format in ("json", "both"):
        write_json_data(output, area_infos, targets, inbound_counts, metadata)

    print(f"Wrote repo map to {output}")
    print(
        f"Reachable targets: {metadata['total_targets']} ({metadata['non_testonly_targets']} non-testonly)"
    )
    print(f"Areas written: {len(area_infos)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
