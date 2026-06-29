# GN dependency-frequency repo map

This tool generates a compact Brave/Chrome repo map from GN direct dependency
frequency. It is intended for agents that need a fast-to-read orientation
artifact before searching or opening source files.

It is deliberately **not** a semantic repo map. It does not use PageRank, symbol
extraction, git history, ownership data, semantic scoring, or task-specific
ranking. The only score is the number of inbound direct GN dependency references
from reachable non-testonly targets.

## Generate

From Chromium `src`:

```sh
python3 brave/tools/repo_map/generate_dependency_map.py \
  --out-dir out/current_link \
  --root //brave:brave \
  --top-areas 100 \
  --output brave/.repo-map
```

From `src/brave`:

```sh
python3 tools/repo_map/generate_dependency_map.py \
  --out-dir ../out/current_link \
  --root //brave:brave \
  --top-areas 100 \
  --output .repo-map
```

If `--out-dir` is omitted, the script defaults to `out/current_link` (or
`../out/current_link` from `src/brave`) and falls back to `out/Component_arm64`
when that directory exists locally.

## Output

The generated `.repo-map/` directory is ignored by git. Commit the generator and
README, not generated snapshots, unless a task explicitly asks for a snapshot.

- `.repo-map/default.md` — small entry point for agents. Includes generation
  metadata, root targets, testonly exclusion, top depended-on areas, and links to
  per-area files. Areas with zero inbound dependency references are omitted.
- `.repo-map/areas/<area>.md` — compact area details: score, built target count,
  top contributing GN targets, and common source/header directories when cheap
  to get from GN metadata.
- `.repo-map/data/dependency_frequency.json` and `.repo-map/data/targets.json` —
  optional machine-readable data for later tooling.

## Agent workflow

1. Generate or update `.repo-map/`.
2. Read `.repo-map/default.md` first.
3. Read only the relevant `.repo-map/areas/*.md` files.
4. Search or open actual source files as needed.

## Counting model

For every reachable GN target from the root target(s):

1. Skip test-only targets entirely.
2. Inspect the direct deps of each non-testonly target.
3. Skip deps that are test-only.
4. Increment the dep target's count by one.
5. Roll target labels up into coarse area keys such as `base`, `components/prefs`,
   `brave/components/brave_wallet`, `chrome/browser`, and `content/browser`.

By default, most `//third_party` and `//brave/third_party` targets are excluded,
including Blink. Use `--include-third-party` for non-Blink third-party deps and
`--include-blink` for Blink deps when that noise is desired.
