---
name: top-crashers
description:
  "Get top crashers from Brave's Backtrace crash reporting. Shows crash
  signatures, stacks, platforms, versions, channel breakdown, code origin, and
  regression detection. Triggers on: top crashers, crash report, what's
  crashing, top crashes, crash analysis, regression crashers, new crashes."
disable-model-invocation: true
argument-hint:
  '[--days 7] [--platform Windows] [--compare 2] [--new-only] [--crashes-only]
  [--channels nightly,beta] [--brave-only]'
---

# Top Crashers

Query Brave's Backtrace crash reporting instance for top crashers. Returns
developer-actionable data including crash signatures, stack traces,
platform/version breakdowns, channel mapping, code origin classification, and
triage URLs.

**PII-safe**: Only aggregate data is output. Use the triage URLs for full crash
details.

---

## When to Use

- **Investigating stability regressions** after a release
- **Prioritizing crash fixes** — which crashes affect the most users
- **Detecting new crashers** introduced by recent changes
- **Filing crash bugs** — the output includes suggested issue titles and labels
- **During development** — checking if your area of code has top crashers

---

## Environment Requirements

The `BACKTRACE_API_KEY` environment variable must be set with a token that has
`query:post` capability. The `BACKTRACE_PROJECT` environment variable should
also be set (or pass `--project` as an argument).

If not set, inform the user:

```
BACKTRACE_API_KEY must be set. Create a token with query:post capability in Backtrace project settings.
BACKTRACE_PROJECT should be set to the Backtrace project name (or pass --project).
```

---

## The Job

When invoked, run the top-crashers script and present the results to the user.

### Step 1: Parse Arguments

Map the user's request to script arguments:

| User says                                    | Script flags                        |
| -------------------------------------------- | ----------------------------------- |
| "top crashers" (no args)                     | `--days 7 --limit 25`               |
| "top 10 crashers"                            | `--limit 10`                        |
| "crashers on Windows"                        | `--platform Windows`                |
| "crashers on Mac"                            | `--platform Darwin`                 |
| "crashers on Linux"                          | `--platform Linux`                  |
| "crashers on Android"                        | `--platform Android`                |
| "crashers in version 1.73"                   | `--version 1.73.`                   |
| "new crashers" or "crash regressions"        | `--new-only`                        |
| "compare crashers" or "what's regressing"    | `--compare 2`                       |
| "crashers last 30 days"                      | `--days 30`                         |
| "crashers since 2025-02-01"                  | `--since 2025-02-01`                |
| "worst crashers"                             | `--order count` (default)           |
| "most recent crashers"                       | `--order last-seen`                 |
| "actual crashes" or "real crashes"           | `--crashes-only`                    |
| "crashes on nightly" or "nightly crashers"   | `--channels nightly`                |
| "crashes on nightly and beta"                | `--channels nightly,beta`           |
| "release crashes"                            | `--channels release`                |
| "brave code crashes" or "our crashes"        | `--brave-only`                      |
| "what should we fix" or "actionable crashes" | `--crashes-only --channels nightly` |

If the user provides explicit flags (e.g.,
`/top-crashers --compare 3 --platform Windows`), pass them through directly.

### Step 2: Run the Script

```bash
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --format json [parsed flags]
```

Always use `--format json` when running programmatically so you can parse and
present the results. For a quick human-readable dump, use `--format markdown`
instead.

### Step 3: Present Results

After running the script, present the results to the user in a clear format:

1. **Summary header**: "Found N crash groups in the last M days"
2. **For each crasher** (in order):
   - Rank, suggested title, and badges:
     - **[CRASH]** or **[DUMP_WITHOUT_CRASHING]** — whether this is an actual
       crash or DumpWithoutCrashing. DumpWithoutCrashing is a non-fatal
       diagnostic that does NOT affect users.
     - **[BRAVE]**, **[CHROMIUM]**, or **[MIXED]** — whether the crashing code
       is in Brave's codebase (`src/brave`) or upstream Chromium (`src/`). Only
       Brave code crashes are directly fixable by the Brave team.
     - **[NEW]**, **[RISING]** — regression indicators
   - Crash count and rate (crashes/day)
   - **Channel breakdown** — which channels are affected (Nightly, Beta,
     Release, Older). Highlight whether the crash affects **current Nightly**
     since that represents the latest master code.
   - Top platform with % share
   - Top version with % share (displayed as "Brave X.Y.Z (Cr N)")
   - Recency (when last seen)
   - Callstack (top frames)
   - Triage URL for full details
3. **If compare mode**: Highlight NEW and RISING crashers prominently

**Presentation priorities:**

- Always clearly distinguish actual crashes from DumpWithoutCrashing
- Highlight crashes affecting Nightly (current master) — these are the most
  actionable
- Note code origin — crashes in Brave code are directly fixable
- When prioritizing what to fix, recommend focusing on: actual crashes (not
  dumps) that affect Nightly (current code) in Brave code

### GitHub Formatting Rules

When mentioning a crasher's rank in GitHub issues, PR descriptions, or comments,
**never** use `#N` syntax (e.g., `#8`). GitHub automatically converts `#N` into
a link to issue or PR number N, which is misleading.

- **Bad:** "This is the #8 crasher this week" — GitHub links to issue/PR 8
- **Good:** "This is the No. 8 crasher this week", "This is crasher rank 8",
  "the 8th top crasher"

This applies to issue titles, bodies, PR descriptions, and comments — anywhere
GitHub renders markdown.

### Step 4: Offer Follow-Up Actions

After presenting results, suggest relevant follow-up actions:

- "Want me to look into any of these crashes in more detail?"
- "I can filter by a specific platform or version"
- "I can check if any of these are regressions with `--compare`"
- "I can show only actual crashes with `--crashes-only`"
- "I can filter by channel with `--channels nightly`, `--channels nightly,beta`,
  etc."
- "I can show only Brave code crashes with `--brave-only`"
- "I can draft GitHub issues for the top crashers"

---

## Usage Examples

```bash
# Default: top 25 crashers in last 7 days
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT"

# Top 10, JSON output for parsing
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --format json --limit 10

# Real crashes only (exclude DumpWithoutCrashing)
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --crashes-only

# Crashes affecting current Nightly
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --channels nightly

# Crashes on Nightly and Beta
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --channels nightly,beta

# Brave-specific code crashes only
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --brave-only

# Actionable crashes: real crashes on Nightly in Brave code
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --crashes-only --channels nightly --brave-only

# Windows-only crashers
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --platform Windows

# Regression detection: compare last 2 days vs prior 2 days
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --compare 2

# Only new crashers (first seen within lookback window)
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --new-only

# Specific version, sorted by recency
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --version 1.73. --order last-seen

# Last 30 days, CSV for spreadsheet
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --format csv --days 30

# Dry run to see the query without executing
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --dry-run

# NDJSON for piping to jq
python3 ./.claude/skills/top-crashers/top-crashers.py --project "$BACKTRACE_PROJECT" --format ndjson --limit 5 | jq .fingerprint
```

---

## Script Details

**Location**: `./.claude/skills/top-crashers/top-crashers.py`

**Output formats**: `markdown`, `json`, `ndjson`, `csv`

**Exit codes**:

- `0`: Success
- `1`: Auth/config error (missing API key or project)
- `2`: Network/API error
- `3`: No results matching criteria

**Full flag reference**:

| Flag             | Description                                                          | Default                 |
| ---------------- | -------------------------------------------------------------------- | ----------------------- |
| `--project`      | Backtrace project name                                               | `BACKTRACE_PROJECT` env |
| `--days`         | Lookback window in days                                              | 7                       |
| `--since`        | ISO date start (alternative to --days)                               | —                       |
| `--limit`        | Max crash groups to return                                           | 25                      |
| `--min-count`    | Minimum crash count threshold                                        | 10                      |
| `--format`       | Output format                                                        | markdown                |
| `--platform`     | Filter by platform                                                   | —                       |
| `--version`      | Filter by version prefix                                             | —                       |
| `--channel`      | Filter by channel                                                    | —                       |
| `--order`        | Sort: count, last-seen, rate                                         | count                   |
| `--frames`       | Stack frames to show                                                 | 8                       |
| `--new-only`     | Only first-seen-in-window crashers                                   | false                   |
| `--crashes-only` | Exclude DumpWithoutCrashing entries                                  | false                   |
| `--channels`     | Filter by channels (comma-separated: nightly,beta,release, or 'all') | all                     |
| `--brave-only`   | Only crashes in Brave code                                           | false                   |
| `--brave-src`    | Path to src/brave for code origin grep                               | auto-discovered         |
| `--compare`      | Regression: compare N days vs prior N                                | —                       |
| `--dry-run`      | Print query without executing                                        | false                   |
| `--verbose`      | Print timing/debug info                                              | false                   |

---

## Output Fields Per Crash Group

Each crash group includes these **allowlisted fields only** (PII-safe):

- **fingerprint** — SHA256 group ID (safe to reference in issues)
- **count** / **crashes_per_day** — Occurrence counts
- **classifier** — Crash signal (e.g. dump, invalid-access, breakpoint, abort)
- **crash_type** — `"crash"` (actual user-impacting) or `"dump"`
  (DumpWithoutCrashing diagnostic)
- **code_origin** — `"brave"` (Brave-specific code), `"chromium"` (upstream), or
  `"mixed"`
- **top_frame** — First meaningful crashing function
- **signature** — Combined `TopFrame (Classifier) on Platform Version`
- **callstack** — Top N sanitized stack frames
- **top_platform** / **platform_pct** — Most affected platform + % share
- **top_version** / **version_pct** — Most affected version + % share
- **channel_breakdown** — `{"nightly": N, "beta": N, "release": N, "older": N}`
- **top_channel** — Channel with most crashes
- **affects_nightly** — Whether this crash appears on current Nightly
- **first_seen** / **last_seen** / **recency** — Timestamps
- **is_new** — Whether first seen within the lookback window
- **triage_url** — Link to Backtrace UI for full details
- **suggested_title** — Ready-to-use issue title
- **labels** — Suggested issue labels

In compare mode, also:

- **regression_badge** — NEW, RISING, FALLING, or STABLE
- **change_factor** — Ratio of recent vs baseline count
- **baseline_count** — Count from the baseline period

---

## Version Format

Brave versions follow the format
`CHROMIUM_MAJOR.BRAVE_MAJOR.BRAVE_MINOR.BRAVE_PATCH`:

- `145.1.87.42` = Brave 1.87.42 on Chromium 145
- Channel mapping is fetched from the Brave Release Schedule wiki at runtime

---

## Limitations

- Requires `BACKTRACE_API_KEY` with `query:post` capability
- Stack frames are sanitized (paths stripped, truncated at 200 chars)
- Histograms show top 5 values; use triage URL for full breakdown
- Compare mode uses a simple 2x threshold for RISING detection
- Code origin detection greps for crashing symbols in `src/brave` when the
  directory is found, with namespace heuristic fallback
- Channel versions are fetched from the wiki with hardcoded fallback defaults
