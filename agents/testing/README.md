# Skill-eval harness

A Brave port of chromium's `//agents/testing/` promptfoo harness, retargeted at
**Claude Code** (our skills use Claude Code-only frontmatter and are not runnable
under gemini-cli). It's a characterization/regression suite for skills: a fixed
fixture + fixed task → an expected *structural* outcome, so weakening a skill's
prose shows up as the agent's output degrading.

It is **not** a per-PR gate — it spins a real agent and burns tokens. Run it
path-filtered (when `agents/skills/**` or `agents/testing/**` changes), on a
schedule, or on demand.

## Layout

```
agents/
  skills/<name>/            # skills (source of truth); linked into .claude/skills/ by setup.py
  prompts/eval/<skill>/     # per-skill eval data
    <RULE>.promptfoo.yaml   # one promptfoo test (e.g. CS-003)
    <RULE>.prompt.md        # the task handed to the agent
    fixtures/               # canned PR diff(s)
  testing/
    run_evals.py            # discover + run + pass-k report
    claude_provider.py      # promptfoo python: provider driving Claude Code
    fake_gh.py              # stub `gh` (serves canned data, records mutations)
    eval_config.py          # runs_per_test / pass_k_threshold / tags
    asserts/check_changes.py# structural file/content asserts (ported verbatim)
```

## Running

From the repo root (`src/brave`):

```bash
# Run every discovered eval
python3 agents/testing/run_evals.py

# Run only the stable-tagged set
python3 agents/testing/run_evals.py --tag-filter stable

# List discovered tests without running
python3 agents/testing/run_evals.py --list

# Run one config directly via promptfoo (from its own dir)
cd agents/prompts/eval/review-prs && npx promptfoo eval -c CS-003.promptfoo.yaml
npx promptfoo view          # inspect inputs/output/asserts in the web UI
```

Requires `node`/`npx`, the Claude Code CLI on PATH (or `$CLAUDE_BIN`), and
whatever auth Claude Code needs for headless runs.

## How a test works

1. `run_evals.py` discovers `*.promptfoo.yaml`, reads its `metadata`
   (`runs_per_test`, `pass_k_threshold`, `tags`), and runs it that many times.
2. Each run, promptfoo hands the prompt to `claude_provider.py`, which:
   - ensures the requested `skills` are linked into `.claude/skills/`,
   - if `fake_gh` is set, writes a stub `gh` first on `PATH` that serves the
     canned PR (diff + metadata) and records — never performs — any mutating
     call, so real GitHub is never touched,
   - runs Claude Code headless in the repo root,
   - harvests the skill's `pr_*/*_results.json` into
     `agents/testing/.last_run/results.json` (deterministic path for asserts).
3. Asserts (`check_changes.py:check_file_content` + promptfoo `icontains`)
   check the results *structurally* — e.g. the flagged violation names the rule
   ID and the offending symbol — not the model's prose.
4. A test passes if at least `pass_k_threshold` of `runs_per_test` runs pass.

## Adding a test (one per best-practice rule)

Drop `<RULE-ID>.promptfoo.yaml` + `<RULE-ID>.prompt.md` + a fixture under
`agents/prompts/eval/review-prs/`, reusing this provider and asserts. Keep
asserts name/signature-agnostic: pin the *pattern the skill teaches*, not an
exact result.

## Open items

- **Headless auth.** How Claude Code authenticates in CI/headless is not yet
  settled; locally it uses the developer's existing Claude Code auth.
- **Results path.** `.last_run/results.json` is harvested by globbing the run's
  temp dir for `pr_*/*_results.json`; confirm against a live `review-prs` run
  and tighten if its output layout changes.
