---
name: make-ci-green
description:
  'Re-run failed CI jobs for a brave/brave-core PR. Detects failure stage and
  uses WIPE_WORKSPACE for build/infra failures. Triggers on: make ci green,
  retry ci, rerun ci, fix ci, re-run failed jobs, retrigger ci.'
argument-hint: '<pr-number> [--dry-run]'
disable-model-invocation: true
---

# Make CI Green

Re-run failed Jenkins CI jobs for a brave/brave-core PR. Automatically detects
the failure stage and decides whether to use WIPE_WORKSPACE (for build/infra
failures) or a normal re-run (for test/storybook failures).

---

## When to Use

- **Flaky test failures** on CI that just need a retry
- **Build/infra failures** that need a workspace wipe to recover
- **After pushing new commits** to retrigger only the failing checks
- **Batch retriggering** when multiple platform checks are failing

---

## Environment Requirements

The following environment variables must be set (e.g., in your `.envrc`):

```bash
export JENKINS_BASE_URL=https://$JENKINS_BASE_URL
export JENKINS_USER=<your-jenkins-username>
export JENKINS_TOKEN=<your-api-token>
```

Get your API token from `$JENKINS_BASE_URL/me/configure`.

---

## The Job

### Step 1: Parse Arguments

Extract the PR number from the user's input. The PR number is required. Check
for `--dry-run` flag.

| User says                            | PR number       |
| ------------------------------------ | --------------- |
| `/make-ci-green 33936`               | 33936           |
| "retry ci for 33936"                 | 33936           |
| "make ci green on PR 33936"          | 33936           |
| "re-run failed jobs 33936 --dry-run" | 33936 (dry run) |

### Step 2: Analyze Failures

Run the script in dry-run mode to analyze without triggering:

```bash
python3 .claude/skills/make-ci-green/retrigger-ci.py <pr-number> --dry-run --format json
```

### Step 3: Present Findings

Show the user what was found:

- **Failing Jenkins checks**: check name, failed stage, recommended action
  (normal vs WIPE_WORKSPACE), reason
- **Test failure analysis** (for test stage failures): per-test details
  including location, PR correlation, upstream flake verdict, existing issues,
  and issue filing suggestions
- **Non-Jenkins failures** (SonarCloud, Socket Security, etc.): listed but not
  actionable by this tool
- **Pending checks**: still running, listed for awareness
- **No failures**: report that CI is already green

**Example output (build/infra failure):**

```
PR 33936: 1 failing Jenkins check(s)

  [DRY-RUN] continuous-integration/linux-x64/pr-head
       Stage: build
       Action: WIPE_WORKSPACE
       Reason: Pre-test stage failure: "build" -> WIPE_WORKSPACE
       URL: https://$JENKINS_BASE_URL/job/brave-core-build-pr-linux-x64/job/PR-33936/2/
```

**Example output (test failure with analysis):**

```
PR 33936: 1 failing Jenkins check(s)

  [DRY-RUN] continuous-integration/linux-x64/pr-head
       Stage: test_brave_unit_tests
       Action: normal
       Reason: Test/post-build stage failure: "test_brave_unit_tests" -> normal re-run
       URL: https://$JENKINS_BASE_URL/job/brave-core-build-pr-linux-x64/job/PR-33936/2/
       Test Failures (2):

         - BraveWalletServiceTest.GetBalance
           Location: brave
           PR correlation: likely_from_pr (PR modifies files in same directory as test: browser/brave_wallet/)

         - WebUIURLLoaderFactoryTest.RangeRequest
           Location: chromium
           Upstream flake: known_upstream_flake
           PR correlation: likely_unrelated (Chromium test with no related chromium_src overrides in PR)
           >> SUGGEST FILING ISSUE: "Test failure: WebUIURLLoaderFactoryTest.RangeRequest"
           Stack trace (last 20 lines):
             ...
```

**Key decisions shown per test failure:**

- **likely_from_pr**: The PR changes overlap with the test's source — developer
  should fix their PR, no issue suggested
- **likely_unrelated**: The failure appears unrelated to the PR — suggests
  filing an issue if none exists
- **Existing issue found**: Shows link to the existing issue instead of
  suggesting a new one
- **Upstream flake** (Chromium tests only): Shows LUCI Analysis verdict for
  upstream flakiness

### Step 4: Confirm and Trigger

If the user wants to proceed (and not `--dry-run`), run the script to trigger
rebuilds:

```bash
python3 .claude/skills/make-ci-green/retrigger-ci.py <pr-number> --format json
```

Report the results: which checks were retriggered, the action taken, and any
errors.

---

## WIPE_WORKSPACE Decision Logic

The script examines which pipeline stage failed:

| Failed Stage                                                                                                                           | Action             | Rationale                                                |
| -------------------------------------------------------------------------------------------------------------------------------------- | ------------------ | -------------------------------------------------------- |
| init, checkout, install, config, build, compile, setup, sync, gclient, source, deps, fetch, configure, bootstrap, prepare, environment | **WIPE_WORKSPACE** | Infrastructure/build failure; workspace may be corrupted |
| storybook, test(s), audit, lint, upload, publish, or anything else                                                                     | **Normal re-run**  | Test failure likely flaky; no workspace issue            |
| Unknown (API error)                                                                                                                    | **Normal re-run**  | Safe default                                             |

Stage matching is case-insensitive substring matching (e.g., "Build (Debug)"
matches "build").

---

## Test Failure Analysis

When the failed stage is a **test stage** (not build/infra), the script
automatically:

1. **Extracts test failures** from Jenkins console output (GTest `[ FAILED ]`
   patterns)
2. **Classifies each test** as Brave (`src/brave/`) or Chromium (`src/`
   excluding brave) using `git grep`
3. **Checks upstream flakiness** via LUCI Analysis (Chromium tests only —
   Brave-specific tests are not in LUCI)
4. **Correlates with PR changes** to determine if the failure is likely caused
   by the PR or unrelated
5. **Searches for existing issues** in `brave/brave-browser` matching the test
   name
6. **Suggests filing an issue** only if the failure is likely unrelated to the
   PR AND no existing issue exists

### PR Correlation Logic

| Scenario                                              | Assessment           | Action                                              |
| ----------------------------------------------------- | -------------------- | --------------------------------------------------- |
| PR modifies the test file itself                      | **likely_from_pr**   | No issue suggestion — developer should fix their PR |
| PR modifies files in the same directory/module        | **likely_from_pr**   | No issue suggestion                                 |
| PR has `chromium_src/` overrides in related paths     | **likely_from_pr**   | No issue suggestion                                 |
| Chromium test with no related `chromium_src/` changes | **likely_unrelated** | Suggest filing issue (if none exists)               |
| Brave test with no overlapping PR changes             | **likely_unrelated** | Suggest filing issue (if none exists)               |
| Cannot determine (e.g., test source not found)        | **unknown**          | No suggestion                                       |

### Issue Filing

Issues are **never auto-filed**. The script only suggests filing and provides:

- Suggested title: `Test failure: <TestSuite.TestMethod>`
- Body with platform, upstream flake verdict (if applicable), and stack trace
- Suggested label: `QA/intermittent`
- Target repo: `brave/brave-browser`

The user must confirm before any issue is created.

---

## Usage Examples

```bash
# Dry run: analyze without triggering
python3 .claude/skills/make-ci-green/retrigger-ci.py 33936 --dry-run

# Trigger rebuilds for all failing checks
python3 .claude/skills/make-ci-green/retrigger-ci.py 33936

# JSON output for programmatic use
python3 .claude/skills/make-ci-green/retrigger-ci.py 33936 --format json

# Dry run with JSON output
python3 .claude/skills/make-ci-green/retrigger-ci.py 33936 --dry-run --format json
```

---

## Exit Codes

- `0`: Success (checks found and processed)
- `1`: Config error (missing JENKINS_TOKEN)
- `2`: API error (GitHub or Jenkins unreachable)
- `3`: No failing Jenkins checks found

---

## Limitations

- Only handles Jenkins CI checks (identified by `$JENKINS_BASE_URL` in the URL)
- Cannot retrigger GitHub Actions or other CI systems (SonarCloud, Socket
  Security)
- Requires Jenkins API access with a valid token
- Does not handle PENDING checks (still running)
- WIPE_WORKSPACE detection relies on stage name keyword matching
- Test failure extraction only supports GTest output format (`[ FAILED ]`
  markers)
- Upstream flake check only works for Chromium tests (Brave-specific tests are
  not in LUCI)
- PR correlation uses directory-level heuristics; indirect dependencies may not
  be detected
- Requires a local chromium `src/` checkout for `git grep` test classification
