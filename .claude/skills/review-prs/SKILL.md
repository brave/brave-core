---
name: review-prs
description:
  'Review PRs in the configured PR repository for best practices violations.
  Supports single PR (#12345), state filter (open/closed/all), and auto mode for
  cron. Triggers on: review prs, review recent prs, /review-prs, check prs for
  best practices.'
argument-hint:
  '[days|page<N>|#<PR>] [open|closed|all] [auto] [reviewer-priority]'
allowed-tools: Bash(gh pr diff:*)
---

# Review PRs for Best Practices

Scan recent open PRs in the configured PR repository for violations of
documented best practices.

- **Interactive mode** (default): drafts comments and asks for user approval
  before posting.
- **Auto mode** (`auto` argument): posts all violations automatically without
  approval. Designed for cron/headless use.

**IMPORTANT:** This skill only reviews PRs against existing best practices. It
must NEVER create, modify, or add new best practice rules or documentation
during a review run.

---

## Architecture: File-Based Pipeline

The review pipeline minimizes LLM token usage by pushing all heavy data through
files, not context:

1. **prepare-review.py** (zero LLM tokens) — fetches PRs, diffs, comments;
   writes subagent prompt files to a temp work directory; outputs a tiny JSON
   pointer to the work dir
2. **Subagents** (subagent tokens only) — each reads its prompt from a file,
   reviews the diff, validates violations by reading source code, writes results
   to a JSON file
3. **collect-results.py** (zero LLM tokens) — reads all result files, feeds to
   post-review.py which handles prioritization, dedup, posting, approval, and
   notifications

The main LLM session only orchestrates: run scripts, read a small manifest,
launch subagents with tiny prompts, run the collector. It never sees diffs, rule
text, or violation details.

---

## The Job

When invoked with
`/review-prs [days|page<N>|#<PR>] [open|closed|all] [auto] [reviewer-priority]`:

### Step 1: Prepare (zero LLM tokens)

Run the prepare script with all arguments:

```bash
SKILL_DIR="<absolute path to .claude/skills/review-prs>"
python3 $SKILL_DIR/prepare-review.py [days|page<N>|#<PR>] [open|closed|all] [--auto] [--reviewer-priority]
```

The script's stdout is a tiny JSON with `work_dir` and `manifest` paths.
Progress and cost summary go to stderr.

Parse the stdout JSON to get `work_dir`.

### Step 2: Read manifest

Read the manifest file at `{work_dir}/manifest.json`. It contains:

- **`auto_mode`**: whether to post without approval
- **`bot_username`**: the bot's GitHub username
- **`pr_repo`**: the target PR repository
- **`fetch_summary`**: stats on how many PRs were fetched/filtered/skipped
- **`progress_lines`**: pre-formatted progress messages — print these to stdout
  for cron logs
- **`prs`**: array of PRs to review, each containing:
  - `number`, `title`, `headRefOid`, `author`, `hasApproval`
  - `subagent_prompts`: array of entries with `prompt_file` and `results_file`
    paths (NOT prompt text)
- **`cached_prs`**: already-reviewed PRs (handled by prepare script — just log
  results)
- **`errors`**: per-PR errors encountered during preparation

Print the `progress_lines`. Log any errors.

For each cached PR, log:

- If `approved` is true:
  `APPROVE: [PR #N](url) (title) - all threads resolved, approved`
- If `thread_resolution.unresolved_bot_threads > 0`:
  `CACHED: [PR #N](url) (title) - N threads still unresolved`

If no PRs to review (empty `prs` array), skip to Step 4.

### Step 3: Launch subagents

For every PR in `prs`, for every entry in that PR's `subagent_prompts`, launch a
**Task subagent** (subagent_type: "general-purpose") with this prompt:

```
Read your full review instructions from: {prompt_file}
Execute them completely. The instructions contain the PR diff, best practice rules, review rules, and validation requirements.
After reviewing and validating, write your results JSON to the file path specified in the instructions.
```

**Launch ALL subagents across ALL PRs in a single message** so they run
concurrently.

**CRITICAL: Launch ALL subagents — no exceptions.** The prepare script already
filtered documents by file type. Every entry in `subagent_prompts` MUST get a
subagent. Do NOT skip any.

Wait for all subagents to return.

**CRITICAL: NEVER post reviews, comments, or approvals to GitHub yourself.** Do
NOT use `gh api`, `gh pr review`, `gh pr comment`, or any GitHub API calls to
post anything on any PR. All posting is handled exclusively by
`collect-results.py` → `post-review.py` in Step 4. If you post reviews directly,
it creates duplicates.

### Step 4: Collect and post (zero LLM tokens)

Run the collector script — it reads all subagent result files and runs
post-review.py:

```bash
python3 $SKILL_DIR/collect-results.py --work-dir "$WORK_DIR" [--auto]
```

Pass `--auto` if `auto_mode` is true.

The script handles everything: collecting violations from result files,
prioritization/capping (5 per PR), rule link validation, deduplication, posting
inline reviews, approval for clean PRs, cache updates, and the final summary
block.

For **interactive mode** (no `--auto`): instead of running collect-results.py
directly, read the result files yourself from
`{work_dir}/pr_{number}/{chunk_id}_results.json`, present each violation to the
user for approval, then write only approved violations back and run
collect-results.py.

Read the script's stderr — it contains the summary. Print it for cron logs.

### Summary of what consumes LLM tokens

| Phase                           | Token cost                  | Who does it                             |
| ------------------------------- | --------------------------- | --------------------------------------- |
| Fetch PRs, diffs, comments      | **Zero**                    | `prepare-review.py`                     |
| Build subagent prompts (files)  | **Zero**                    | `prepare-review.py`                     |
| Resolve threads, check approval | **Zero**                    | `prepare-review.py`                     |
| Read manifest, launch subagents | **~50 tokens per subagent** | Main LLM session                        |
| Rule-checking + validation      | **Subagent tokens**         | Subagents (read prompt from file)       |
| Collect results, post, approve  | **Zero**                    | `collect-results.py` + `post-review.py` |

---

## PR Link Format

When displaying PR numbers, ALWAYS use a full markdown link:
`[PR #<number>](https://github.com/$PR_REPO/pull/<number>)`. NEVER use bare
`#<number>` — the TUI auto-links them against the wrong repository.

---

## Closed/Merged PR Workflow

When reviewing closed or merged PRs and a violation is found:

1. **Present the finding** to the user (draft comment + ask for approval)
2. **If approved**, try to post inline review comments. If the API fails, fall
   back to:
   ```bash
   gh pr comment --repo $PR_REPO {number} --body "[file:line] comment text"
   ```
3. **Create a follow-up issue** in `$PR_REPO` to track the fix:
   ```bash
   gh issue create --repo $PR_REPO --title "Fix: <brief description>" --body "Found during post-merge review of PR #<NUMBER>. <description>"
   ```
4. **Reference the new issue** back in the PR comment.
