# Skill-eval harness

A Brave port of chromium's `//agents/testing/` promptfoo harness, retargeted at
**Claude Code** (our skills use Claude Code-only frontmatter and are not
runnable under gemini-cli). It's a characterization/regression suite for skills:
a fixed fixture + fixed task → an expected _structural_ outcome, so weakening a
skill's prose shows up as the agent's output degrading.

It is **not** a per-PR gate — it spins a real agent and burns tokens. Run it
path-filtered (when `agents/skills/**` or `agents/testing/**` changes), on a
schedule, or on demand.

## Layout

```
agents/
  skills/<name>/            # Brave skills (source of truth); linked into .claude/skills/ by setup.py
                            # setup.py also links upstream Chromium skills from ../agents/skills/
  prompts/eval/<skill>/     # per-skill eval data
    *.promptfoo.yaml        # one promptfoo test (e.g. CS-003.promptfoo.yaml, eval.promptfoo.yaml)
    *.prompt.md             # the task handed to the agent
    fixtures/               # canned PR diff / setup patch
  testing/
    run_evals.py            # discover + run + pass-k report
    claude_provider.py      # promptfoo python: provider driving Claude Code
    fake_gh.py              # stub `gh` (serves canned data, records mutations)
    eval_config.py          # runs_per_test / pass_k_threshold / tags
    asserts/check_changes.py# structural file/content asserts (ported verbatim)
    .last_run/              # generated: harvested results.json + sandbox/ mirror
```

## Two eval flavors

- **Read-only** (e.g. `review-prs/CS-003`): the skill only inspects a canned PR
  via `fake_gh` and writes findings; the provider runs it in the real
  `src/brave` tree and asserts on the harvested `results.json`.
- **Sandbox / mutating** (e.g. `browser-window-feature-refactor`): the skill
  edits code. The provider (`config.sandbox: true`) applies the setup patch and
  runs the agent in a throwaway per-run git repo — never the real tree — then
  mirrors the result to `.last_run/sandbox/`, which the asserts read at a stable
  relative path. This is what lets a refactor eval re-run `runs_per_test` times
  without polluting the checkout or failing to re-apply its fixture.

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
   - runs Claude Code headless — in the repo root, or (when `sandbox: true`) in
     a throwaway per-run git repo with the skill linked in,
   - for a read-only eval, harvests the skill's `pr_*/*_results.json` into
     `.last_run/results.json`; for a sandbox eval, mirrors the edited tree to
     `.last_run/sandbox/` (both are deterministic paths for asserts).
3. Asserts (`check_changes.py` + promptfoo `icontains`) check the result
   _structurally_ — e.g. the flagged violation names the rule ID and the
   offending symbol, or the refactor created the controller files and moved the
   method — not the model's prose.
4. A test passes if at least `pass_k_threshold` of `runs_per_test` runs pass.

## Adding a test

Drop a `*.promptfoo.yaml` + `*.prompt.md` + fixture under
`agents/prompts/eval/<skill>/`, reusing this provider and asserts. For a
review/inspection skill, follow `review-prs/CS-003` (one file per best-practice
rule, `fake_gh`). For a code-mutating skill, follow
`browser-window-feature-refactor` (`sandbox: true`, assert on
`.last_run/sandbox/`). Keep asserts name/signature-agnostic: pin the _pattern
the skill teaches_, not an exact result.

## Open items

- **Headless auth.** How Claude Code authenticates in CI/headless is not yet
  settled; locally it uses the developer's existing Claude Code auth.
- **Results path.** `.last_run/results.json` is harvested by globbing the run's
  temp dir for `pr_*/*_results.json`; confirm against a live `review-prs` run
  and tighten if its output layout changes.
