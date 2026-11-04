---
name: test-quality
description:
  'Audit Brave tests for ones that would still pass if the code they test broke,
  independently verify every finding, and draft one GitHub issue per test file
  (plus production-bug issues found along the way). Takes the test type:
  unittest, browsertest, interactive_uitest or webui. Triggers on: test quality
  audit, weak tests, false-positive tests, tests that do not test anything,
  /test-quality.'
argument-hint: '<unittest|browsertest|interactive_uitest|webui> [path ...]'
disable-model-invocation: true
---

# Test Quality

Find tests where the answer to **"if the base functionality broke, would this
test fail?"** is no, verify each finding against the whole repo, and turn the
confirmed ones into GitHub issues.

The judging is done by subagents with the briefs in this directory. `tq.py` does
the bookkeeping: enumerate and shard the tests, track progress, build the next
phase's work, merge results into issue drafts, and post them rate limited.

---

## Arguments

- `$1` — test type: `unittest` (`*_unittest.{cc,mm}`), `browsertest`
  (`*_browsertest.{cc,mm}`), `interactive_uitest`
  (`*_interactive_ui_test(s).{cc,mm}`) or `webui` (`*.test.{ts,tsx,js}`).
- `[path ...]` — optional path prefixes to limit the sweep, e.g.
  `components/brave_wallet browser/brave_wallet`.

`TQ=agents/skills/test-quality/tq.py` below; run it from `src/brave`.

---

## Phase 0: Scope and cost

```sh
python3 $TQ init --type <type> [--path <prefix> ...] [--out <dir>]
```

Prints the work dir (`DIR`), and the file/line/test/shard counts. The default
`DIR` is under `/tmp`, which macOS can clear; pass `--out` for a long run.

**Before launching anything, tell the user the counts and the cost, and confirm
the scope.** A full `unittest` sweep (1,441 files, ~10.4k tests) took 44 audit
agents and 43 verify agents, roughly 25M tokens. Offer to pilot one or two
components first.

---

## Phase 1: Audit

One `general-purpose` agent per `DIR/audit/shards/shard-NN.txt`. Run at most ~15
at once and start the next shard as each finishes.

Agent prompt:

> You are auditing shard NN of Brave's <type name> for weak tests: for each
> test, "if the base functionality this test covers broke, would the test fail?"
> Read-only audit — don't modify repo files, build, run tests, or post anywhere.
> Read DIR/AUDIT_BRIEF.md first and follow it exactly. NN = <NN>: file list
> DIR/audit/shards/shard-NN.txt, results file DIR/audit/results/shard-NN.jsonl.
> cwd is the brave-core repo root. Review every file in your list, reading the
> production code under test, not just the test.

Check progress from the results files, not the agents' summaries:

```sh
python3 $TQ status DIR
```

To report progress on a schedule, run a `Monitor` whose command loops
`sleep 120; python3 $TQ status DIR`.

---

## Phase 2: Verify every finding

```sh
python3 $TQ verify-work DIR
```

One agent per `DIR/verify/work/shard-NN.json`, **using the strongest available
model** (`model: opus`). Every finding is verified, whatever its first-pass
confidence; in the reference run the audit's "medium" findings held up better
than its "high" ones.

Agent prompt:

> You are independently verifying "weak test" findings in Brave's <type name>
> before they become public GitHub issues (shard NN). For each finding decide
> CONFIRMED, NARROWED, REFUTED or UNCERTAIN. Assume the audit made mistakes.
> Read-only: don't modify repo files, build, run tests, or post anywhere. Read
> DIR/VERIFY_BRIEF.md first and follow it exactly. NN = <NN>: work file
> DIR/verify/work/shard-NN.json, results file DIR/verify/results/shard-NN.jsonl.
> cwd is the brave-core repo root. Verify every finding id in the work file.

In the reference run ~17% of findings were refuted here, mostly because a test
in another file (often a browser test) already caught the mutation, or the code
was unreachable.

---

## Phase 3: Production bugs

Verifiers note real bugs in `production_bug`. Collect them and verify them with
one more `model: opus` agent:

```sh
python3 $TQ bug-work DIR
```

> Verify the suspected production bugs listed in DIR/bugs/candidates.json. Read
> DIR/BUG_BRIEF.md first and follow it exactly. cwd is the brave-core repo root.

Read the code yourself for every `high`-severity `REAL` bug before it goes in a
report.

---

## Phase 4: Aggregate and report

```sh
python3 $TQ aggregate DIR
```

Writes `DIR/final/issues.json` (one issue per test file with at least one
CONFIRMED/NARROWED finding), Markdown drafts in `DIR/final/drafts/`, and
`DIR/final/bug_issues.{json,md}` from `REAL`/`LATENT` bugs.

Report to the user: files and tests reviewed, findings, verdict split, issues by
top severity, the most serious findings, and the production bugs. Issue bodies
pin line links to the current brave-core commit.

---

## Phase 5: Post (only with explicit approval)

Never post without the user's explicit go-ahead for this run. Issues go to
`brave/brave-browser` (brave-core has issues disabled).

1. Labels: test issues get `tests`, `tests/test-quality`, `QA/No`,
   `release-notes/exclude`, then `tests/false-positive` /
   `tests/weak-assertions` / `tests/missing-coverage` by finding category and a
   feature label from the path. Bug issues get `bug`, `crash` where it applies,
   and a feature label. Creating labels needs write access: `python3 $TQ labels`
   (default gh account).
2. Ask which account to post as. Posting reads the token with
   `gh auth token --user <account>` and passes it as `GH_TOKEN`, so the user's
   active gh account isn't switched. A triage-only account can apply labels but
   not create them.
3. Post one sample and show it to the user:
   `python3 $TQ post DIR --account <account> --one <path>`
4. On approval, post the rest detached so it survives the session:

   ```sh
   nohup python3 $TQ post DIR --account <account> --batch > DIR/post/out.txt 2>&1 &
   ```

   Then the bugs the same way with `--bugs`, once the user approves them.

Posting notes:

- `--gap 15,35` (default) is ~140 issues/hour. GitHub's documented ceiling for
  content creation is 500/hour; stay well under it. Faster gaps work but draw
  more transient errors.
- GitHub returns a generic "GraphQL: Something went wrong" a few times per
  hundred. The poster waits 2–5 min and checks GitHub for the title before
  retrying, so a create that actually succeeded isn't duplicated. Rate-limit
  errors wait 30–60 min; 3 consecutive failures stop the run.
- Runs are resumable: already-posted titles are read from GitHub on start.
  `touch DIR/post/STOP` stops after the current issue.
- To restart a running poster, find it by its exact process
  (`ps -axo pid,args | grep "[P]ython.*tq.py post"`); `pgrep -f` also matches
  the shell running the check.
