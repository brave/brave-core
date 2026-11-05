---
name: test-quality
description:
  'Audit Brave tests two ways: for ones that would still pass if the code they
  test broke (effectiveness), and for ones that break the documented test best
  practices (best-practices). Independently verifies every finding and drafts
  one GitHub issue per test file (plus production-bug issues found along the
  way). Takes the test type: unittest, browsertest, interactive_uitest or webui,
  and optionally one lane. Triggers on: test quality audit, weak tests,
  false-positive tests, tests that do not test anything, test best practices
  audit, /test-quality.'
argument-hint:
  '<unittest|browsertest|interactive_uitest|webui>
  [effectiveness|best-practices|all] [path ...]'
disable-model-invocation: true
---

# Test Quality

Audit tests in two **lanes**, verify each finding against the whole repo, and
turn the confirmed ones into GitHub issues:

- `effectiveness` — tests where the answer to **"if the base functionality
  broke, would this test fail?"** is no.
- `best-practices` — tests that break a documented test rule in
  `docs/best-practices` (flaky waits, unrestored globals, unchecked feature
  defaults, misplaced tests, and so on).

The judging is done by subagents with the briefs in this directory. `tq.py` does
the bookkeeping: enumerate and shard the tests, track progress, build the next
phase's work, merge results into issue drafts, and post them rate limited. Each
lane has its own work dir and runs the same phases.

---

## Arguments

- `$1` — test type: `unittest` (`*_unittest.{cc,mm}`), `browsertest`
  (`*_browsertest.{cc,mm}`), `interactive_uitest`
  (`*_interactive_ui_test(s).{cc,mm}`) or `webui` (`*.test.{ts,tsx,js}`).
- `$2` — optional lane: `effectiveness`, `best-practices` or `all` (default). If
  `$2` is anything else, it is the first path and the lane is `all`.
- `[path ...]` — optional path prefixes to limit the sweep, e.g.
  `components/brave_wallet browser/brave_wallet`.

`/test-quality unittest` runs both lanes;
`/test-quality unittest best-practices components/brave_wallet` runs just the
rules lane on one component.

`TQ=agents/skills/test-quality/tq.py` below; run it from `src/brave`.

---

## The best-practices lane

Agents check each test file against the rules in `DIR/PRACTICES.md`, which
`init` builds from `docs/best-practices` for the test type. Only rules a reader
can check from a test file are used, so most of those docs are left out on
purpose. The source of truth is the tables at the top of `tq.py`
(`PRACTICE_RULES`, `PRACTICE_REFS`, `PRACTICE_SKIP`); each audited rule carries
an audit note that narrows what counts.

- **Audited:** rules about reliability (sleeps, `RunUntilIdle`, nested run
  loops, JS and mojo races), isolation (unrestored globals, unpinned feature
  defaults, `//chrome` from components tests), safety (`ASSERT` before use,
  `WillOnce`, silent mock handlers) and placement (component tests in
  `browser/`, `PlatformBrowserTest`).
- **Also judged by `effectiveness`:** rules about assertions that wouldn't fail
  (unchecked `EvalJs`, unverified order or negative cases, and so on). With
  `all` they are left to that lane so one problem doesn't become two issues; run
  `best-practices` alone to include them.
- **Left out:** style and maintainability (test names, `TEST` vs `TEST_F`,
  helper extraction), PR and coverage process, and production-side rules. Filing
  hundreds of public issues for them isn't worth the noise.
- **Rules are found by id**, in whatever doc they sit, so moving a rule between
  docs is safe. `init` stops if a listed id has gone, and warns about any rule
  in the `testing-*` docs that `tq.py` doesn't classify (it is skipped until
  someone adds it to a table). Report that warning to the user.

Best-practices issues are labelled `tests/best-practices`. Many older tests
predate these rules, so a full sweep may produce a lot of findings; Phase 4 lets
you cut them by severity.

---

## Phase 0: Scope and cost

```sh
python3 $TQ init --type <type> [--lane <lane>] [--path <prefix> ...] [--out <dir>]
```

Makes one work dir per lane, `<out>/effectiveness` and `<out>/best-practices`,
and prints each one (`DIR` below) with the file/line/test/shard counts. The
default `<out>` is under `/tmp`, which macOS can clear; pass `--out` for a long
run. `status`, `verify-work`, `bug-work` and `aggregate` accept `<out>` to cover
every lane; `post` needs a lane dir.

**Before launching anything, tell the user the counts and the cost, and confirm
the scope.** A full `effectiveness` sweep of `unittest` (1,441 files, ~10.4k
tests) took 44 audit agents and 43 verify agents, roughly 25M tokens. The
`best-practices` lane has not been measured. It is designed to read less
production code, but every lane re-reads the same test files, so `all` costs at
least as much as `effectiveness` alone. Offer to pilot one or two components
first, and pass on any "unclassified rule" warning from `init`.

---

## Phase 1: Audit

One `general-purpose` agent per `DIR/audit/shards/shard-NN.txt`, for each lane.
Run at most ~15 at once across all lanes and start the next shard as each
finishes.

Agent prompt, `effectiveness` lane:

> You are auditing shard NN of Brave's <type name> for weak tests: for each
> test, "if the base functionality this test covers broke, would the test fail?"
> Read-only audit — don't modify repo files, build, run tests, or post anywhere.
> Read DIR/AUDIT_BRIEF.md first and follow it exactly. NN = <NN>: file list
> DIR/audit/shards/shard-NN.txt, results file DIR/audit/results/shard-NN.jsonl.
> cwd is the brave-core repo root. Review every file in your list, reading the
> production code under test, not just the test.

Agent prompt, `best-practices` lane:

> You are auditing shard NN of Brave's <type name> against the project's test
> best practices: for each test file, does it break any rule in
> DIR/PRACTICES.md? Read-only audit — don't modify repo files, build, run tests,
> or post anywhere. Read DIR/AUDIT_BRIEF.md first and follow it exactly. NN =
> <NN>: file list DIR/audit/shards/shard-NN.txt, results file
> DIR/audit/results/shard-NN.jsonl. cwd is the brave-core repo root. Review
> every file in your list, including its fixtures and helpers.

Check progress from the results files, not the agents' summaries:

```sh
python3 $TQ status <out>   # every lane; or a lane's DIR
```

To report progress on a schedule, run a `Monitor` whose command loops
`sleep 120; python3 $TQ status <out>`.

---

## Phase 2: Verify every finding

```sh
python3 $TQ verify-work <out>
```

One agent per `DIR/verify/work/shard-NN.json` in each lane, **using the
strongest available model** (`model: opus`). Every finding is verified, whatever
its first-pass confidence; in the reference run the `effectiveness` audit's
"medium" findings held up better than its "high" ones.

Agent prompt, `effectiveness` lane:

> You are independently verifying "weak test" findings in Brave's <type name>
> before they become public GitHub issues (shard NN). For each finding decide
> CONFIRMED, NARROWED, REFUTED or UNCERTAIN. Assume the audit made mistakes.
> Read-only: don't modify repo files, build, run tests, or post anywhere. Read
> DIR/VERIFY_BRIEF.md first and follow it exactly. NN = <NN>: work file
> DIR/verify/work/shard-NN.json, results file DIR/verify/results/shard-NN.jsonl.
> cwd is the brave-core repo root. Verify every finding id in the work file.

Agent prompt, `best-practices` lane:

> You are independently verifying "breaks a test best practice" findings in
> Brave's <type name> before they become public GitHub issues (shard NN). For
> each finding decide CONFIRMED, NARROWED, REFUTED or UNCERTAIN. Assume the
> audit made mistakes. Read-only: don't modify repo files, build, run tests, or
> post anywhere. Read DIR/VERIFY_BRIEF.md first and follow it exactly. NN =
> <NN>: work file DIR/verify/work/shard-NN.json, results file
> DIR/verify/results/shard-NN.jsonl. cwd is the brave-core repo root. Verify
> every finding id in the work file.

In the `effectiveness` reference run ~17% of findings were refuted here, mostly
because a test in another file (often a browser test) already caught the
mutation, or the code was unreachable. The `best-practices` lane has no
reference run; its likely refutations are a rule's own exception, a base class
that already handles it, or a documented alternative that can't be used.

---

## Phase 3: Production bugs

Verifiers note real bugs in `production_bug`. Collect them and verify them with
one more `model: opus` agent per lane that has any:

```sh
python3 $TQ bug-work <out>
```

> Verify the suspected production bugs listed in DIR/bugs/candidates.json. Read
> DIR/BUG_BRIEF.md first and follow it exactly. cwd is the brave-core repo root.

Read the code yourself for every `high`-severity `REAL` bug before it goes in a
report.

---

## Phase 4: Aggregate and report

```sh
python3 $TQ aggregate <out> [--min-severity low|medium|high]
```

Writes, per lane, `DIR/final/issues.json` (one issue per test file with at least
one CONFIRMED/NARROWED finding at or above `--min-severity`), Markdown drafts in
`DIR/final/drafts/`, and `DIR/final/bug_issues.{json,md}` from `REAL`/`LATENT`
bugs. It is cheap to re-run with a different cutoff.

Report to the user, per lane: files and tests reviewed, findings, verdict split,
issues by top severity, the most serious findings, and the production bugs.
Include the finding count by severity so they can choose a cutoff, and for the
`best-practices` lane by rule. Production bugs from the two lanes can describe
the same defect in different words, so skim both lists for duplicates. Issue
bodies pin line links to the current brave-core commit.

---

## Phase 5: Post (only with explicit approval)

Never post without the user's explicit go-ahead for this run. Issues go to
`brave/brave-browser` (brave-core has issues disabled).

1. Labels: test issues get `tests`, `tests/test-quality`, `QA/No`,
   `release-notes/exclude`, then for `effectiveness` `tests/false-positive` /
   `tests/weak-assertions` / `tests/missing-coverage` by finding category, or
   for `best-practices` `tests/best-practices`, and a feature label from the
   path. Bug issues get `bug`, `crash` where it applies, and a feature label.
   Creating labels needs write access: `python3 $TQ labels` (default gh
   account).
2. Ask which account to post as. Posting reads the token with
   `gh auth token --user <account>` and passes it as `GH_TOKEN`, so the user's
   active gh account isn't switched. A triage-only account can apply labels but
   not create them.
3. Post one sample from the lane and show it to the user:
   `python3 $TQ post DIR --account <account> --one <path>`
4. On approval, post the rest of that lane detached so it survives the session
   (then the next lane, with its own approval):

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
