---
name: uplift
description: "Create an uplift PR that cherry-picks merged PRs from a contributor into a target branch (beta or release). Defaults to broad eligibility; the scope can be narrowed via a free-form description (e.g. 'only automated test and crash fixes'). Triggers on: /uplift, create uplift, uplift PRs."
argument-hint: [github-username] [beta|release] [all|<num>d|PR1,PR2,PR3] [<scope description in English>]
disable-model-invocation: true
allowed-tools: Bash, Read, WebFetch, Grep, Glob
---

# Uplift PR Creator

Create an uplift pull request that cherry-picks merged PRs from a contributor
into a target channel branch.

By default, eligibility is broad: any merged PR that is a reasonable uplift
candidate is included (bug fixes, correctness fixes, crash fixes, intermittent
test fixes, test filter updates, minor polish). The caller can narrow the scope
by appending a free-form English description as the final argument — for example
`only automated test fixes and crash fixes`, which restores the previous
restrictive behavior.

## Inputs

- **Arguments**: `$ARGUMENTS` — space-separated values:
  - First argument: **GitHub username** (the author whose closed PRs to review)
  - Second argument (optional): **Target channel** — either `beta` or `release`.
    Defaults to `beta` if not specified.
  - Third argument (optional): **PR filter** — either:
    - `all` — evaluate all closed/merged PRs from this author in the past 30
      days. Use
      `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url --jq 'sort_by(.mergedAt)'`
      where `YYYY-MM-DD` is 30 days ago.
    - A **duration** like `<N>d` (e.g., `10d`, `60d`, `7d`) — evaluate all
      closed/merged PRs from this author in the past N days. Use
      `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url --jq 'sort_by(.mergedAt)'`
      where `YYYY-MM-DD` is N days ago from today. Parse the number by stripping
      the trailing `d`.
    - A comma-separated list of PR numbers with no spaces (e.g.,
      `33534,33547,33580`) — only evaluate these specific PRs. Fetch each one
      individually with
      `gh pr view <number> --repo brave/brave-core --json number,title,mergedAt,mergeCommit,labels,body,url`.
    - If omitted, defaults to the recent 50 closed PRs (current behavior).
  - Remaining arguments (optional): **Scope description** — any remaining text
    after the filter is treated as a free-form English description of what to
    include or exclude. Apply your judgment when classifying PRs in Step 2. If
    absent, use the default broad eligibility.

Parse the arguments by splitting `$ARGUMENTS` on whitespace for the first three
positional arguments. Everything after the third token (if any) is the free-form
scope description and should be kept intact as a single string. The first token
is always the username. The second token, if it equals `beta` or `release`, is
the channel; otherwise it is treated as the filter and the channel defaults to
`beta`. The third token, if present, is the filter; otherwise filter defaults to
recent 50.

Examples:

- `/uplift netzenbot` → username=`netzenbot`, channel=`beta`, filter=recent 50,
  scope=default broad
- `/uplift netzenbot beta` → username=`netzenbot`, channel=`beta`, filter=recent
  50, scope=default broad
- `/uplift netzenbot release` → username=`netzenbot`, channel=`release`,
  filter=recent 50, scope=default broad
- `/uplift netzenbot beta all` → username=`netzenbot`, channel=`beta`,
  filter=all PRs (past 30 days), scope=default broad
- `/uplift netzenbot beta 10d` → username=`netzenbot`, channel=`beta`,
  filter=all PRs (past 10 days), scope=default broad
- `/uplift netzenbot release 60d` → username=`netzenbot`, channel=`release`,
  filter=all PRs (past 60 days), scope=default broad
- `/uplift netzenbot release 33534,33547,33580` → username=`netzenbot`,
  channel=`release`, filter=only those 3 PRs, scope=default broad
- `/uplift netzenbot beta 30d only automated test fixes and crash fixes` →
  username=`netzenbot`, channel=`beta`, filter=30d, scope=restrict to
  intermittent test fixes, test filter updates, and crash fixes only
- `/uplift netzenbot beta all exclude UI changes, include all bug fixes` →
  username=`netzenbot`, channel=`beta`, filter=all, scope=as described

---

## Step 1: Gather Information

Run these in parallel:

1. **Fetch closed PRs** (method depends on the third argument):

   - **Default (no third arg)**: Use
     `gh pr list --repo brave/brave-core --author <username> --state closed --limit 50 --json number,title,mergedAt,mergeCommit,labels,body,url --jq 'sort_by(.mergedAt)'`
   - **`all`**: Use
     `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url --jq 'sort_by(.mergedAt)'`
     where `YYYY-MM-DD` is 30 days ago from today.
   - **`<num>d` duration** (e.g., `10d`, `60d`): Use
     `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url --jq 'sort_by(.mergedAt)'`
     where `YYYY-MM-DD` is `<num>` days ago from today.
   - **Comma-separated PR list**: For each PR number, use
     `gh pr view <number> --repo brave/brave-core --json number,title,mergedAt,mergeCommit,labels,body,url`.
     Collect results into a list sorted by `mergedAt`.

   The `mergedAt` field is a GitHub API property — if it is `null`, the PR was
   closed without being merged and should be skipped.

2. **Determine the target branch**: Fetch the content at
   `https://github.com/brave/brave-browser/wiki/Brave-Release-Schedule` and find
   the "Current channel information" table. Look for the row matching the target
   channel:

   - If channel is `beta`: find the **Beta** row to get its branch name (e.g.,
     `1.88.x`)
   - If channel is `release`: find the **Release** row to get its branch name
     (e.g., `1.87.x`)

   This branch is:

   - The base branch to create the uplift PR against
   - The branch to cherry-pick commits into

---

## Step 2: Classify PRs

Review each **merged** PR (skip any where `mergedAt` is null) and classify it as
either **include** or **exclude** for the uplift.

**Always EXCLUDE** (regardless of scope):

- Not merged (`mergedAt` is null)
- Already has the uplift label for the target channel (check the `labels` array
  in the PR JSON for `uplift/beta` or `uplift/release` depending on the target
  channel)

**If a scope description was provided** in the arguments, follow it. Use the PR
title and body to judge whether each PR matches the described scope. When in
doubt, prefer to exclude and note the reason in the summary so the caller can
override.

A common scope description is restricting to automated test and crash fixes only
— in that case use this classification:

- **INCLUDE**:
  - Intermittent/flaky test fix (titles often contain "Fix flaky", "Fix test:",
    "Fix intermittent", "Disable flaky")
  - Crash fix (titles mention "crash", "null dereference",
    "EXCEPTION_ACCESS_VIOLATION", etc.)
  - Test filter updates (disabling broken upstream tests, updating stale filter
    entries)
- **EXCLUDE**: feature additions, refactors, anything else unrelated to test
  stability or crashes.

**If no scope description was provided**, use broad default eligibility:

- **INCLUDE** any merged PR that is a reasonable uplift candidate:
  - Bug fixes and correctness fixes
  - Crash fixes
  - Intermittent/flaky test fixes and test filter updates
  - Small UX polish or visible defect fixes
  - Localization / string fixes
  - Build / packaging fixes that affect the channel
- **EXCLUDE**:
  - New feature additions (not fixes)
  - Large refactors with no behavior change
  - Risky changes that the author or reviewers would likely not want
    auto-uplifted (e.g., security-sensitive rewrites, schema migrations); when
    uncertain, exclude and note the reason

When in doubt about a PR, exclude it and explain the reasoning in the summary;
the caller can re-run the skill with an explicit scope or PR list to include it.

---

## Step 3: Cherry-Pick in Chronological Order

1. **Detect the remote**: Run `git remote -v` to determine whether `upstream` or
   `origin` points to `brave/brave-core`. Use whichever remote is correct
   (typically `origin` if there's no `upstream`).
2. Fetch the target branch: `git fetch <remote> <target-branch>`
3. **Choose a unique branch name**: Use `uplift_<username>_<target-branch>` as
   the base name. If that branch already exists (locally or on the remote),
   append a numeric suffix (e.g., `uplift_<username>_<target-branch>_2`, `_3`,
   etc.) until you find an unused name. Create the branch:
   `git checkout -b <branch-name> <remote>/<target-branch>`
4. **Filter out commits already in the target branch**: Before cherry-picking,
   check which included commits are already present in the target branch. For
   each commit, search the target branch log for the PR number from the commit
   message (e.g., `git log <remote>/<target-branch> --oneline --grep="#XXXXX"`).
   If a match is found, that PR is already in the target branch — mark it as
   excluded with reason "Already in target branch" and skip it. This avoids
   empty cherry-picks and unnecessary merge conflicts.
5. Cherry-pick the remaining commits sorted by `mergedAt` timestamp (earliest
   first):
   ```bash
   git cherry-pick <merge_commit_sha>
   ```
6. If a cherry-pick has conflicts, try to resolve them. If unresolvable, skip
   that PR and note it in the summary.

---

## Step 4: Pre-submission Checks

After all cherry-picks are complete but before pushing, run these checks:

1. **Run format**: `npm run format`
2. **Run gn_check**: `npm run gn_check`

If either command indicates changes are needed (e.g., formatting fixes, GN file
updates), make the necessary fixes and amend the last commit:

```bash
git add -A && git commit --amend --no-edit
```

3. **Run presubmit**:
   `npm run presubmit -- --base=<remote>/<target-branch> --fix` (e.g.,
   `npm run presubmit -- --base=origin/1.87.x --fix`)

If the presubmit check fails for reasons unrelated to the cherry-picked changes
(e.g., pre-existing issues in the target branch), note the failures in the
summary but proceed with the uplift.

---

## Step 5: Create the Uplift PR

### Title Format

Generate the title dynamically based on the categories of PRs actually included:

- If the uplift contains **only** intermittent/flaky test fixes and test filter
  updates (no crash fixes, no other PRs):
  `Uplift intermittent test fixes to <target-branch>`
- If the uplift contains **only** crash fixes (no test fixes, no other PRs):
  `Uplift crash fixes to <target-branch>`
- If the uplift contains **only** test fixes and crash fixes (no other PRs):
  `Uplift intermittent test fixes and crash fixes to <target-branch>`
- If the uplift contains a **single** PR outside the test/crash categories,
  mirror that PR's title with an `(uplift to <target-branch>)` suffix — e.g.,
  `Fix Foo on Linux (uplift to 1.88.x)`.
- Otherwise (a mix of categories or multiple non-test/non-crash PRs), use a
  short summary title appropriate for the contents, ending with
  `to <target-branch>` — e.g., `Uplift fixes to <target-branch>`.

### Body Format

Only list the PRs being uplifted. Do NOT mention excluded PRs in the PR body.

The body created here intentionally omits `Resolves` directives for the
underlying tracking issues — those are inserted in Step 7 (immediately after the
last `Uplift of #XXXX` line) once each included PR's linked or freshly created
issue is known.

Use a HEREDOC for correct formatting:

```bash
gh pr create --repo brave/brave-core --base <target-branch> --title "<title>" --body "$(cat <<'EOF'
Uplift of #XXXX
Uplift of #YYYY
Uplift of #ZZZZ

## Included PRs
- #XXXX - <PR title>
- #YYYY - <PR title>
...

Pre-approval checklist:
- [ ] You have tested your change on Nightly.
- [ ] This contains text which needs to be translated.
    - [ ] There are more than 7 days before the release.
    - [ ] I've notified folks in #l10n on Slack that translations are needed.
- [ ] The PR milestones match the branch they are landing to.


Pre-merge checklist:
- [ ] You have checked CI and the builds, lint, and tests all pass or are not related to your PR.

Post-merge checklist:
- [ ] The associated issue milestone is set to the smallest version that the changes is landed on.
EOF
)"
```

### Labels

- If **all** included PRs are test filter-only changes (i.e., only modifying
  files in `test/filters/`), add the `CI/skip` label to the uplift PR.
- Do NOT add `CI/skip` if any included PR contains code changes beyond filter
  files.

### Push and Create

```bash
git push -u origin <branch-name>
```

---

## Step 6: Label the Base PRs

After the uplift PR is created, add the appropriate uplift label to **each base
PR** that was included in the uplift:

- For beta: `uplift/beta`
- For release: `uplift/release`

```bash
gh pr edit <PR_NUMBER> --repo brave/brave-core --add-label "uplift/<channel>"
```

Do this for every PR that was successfully cherry-picked and included.

---

## Step 7: Ensure each included PR has a linked issue (and link them from the uplift PR)

Brave's post-merge checklist requires the associated issue milestone be set to
the smallest version the change landed on. PRs uplifted without a linked issue
make that step impossible, so create and close a tracking issue for any included
PR that does not already have one. While doing that, collect every resulting
issue reference so the uplift PR body can `Resolves` them.

Maintain a running list `RESOLVED_ISSUES` of issue references in
`owner/repo#NNN` form (e.g. `brave/brave-browser#52310`). For each issue
surfaced or created below, append it to this list — deduped, preserving
discovery order.

For **each PR included in the uplift**:

1. **Check for an existing linked issue**:

   ```bash
   gh pr view <PR_NUMBER> --repo brave/brave-core --json closingIssuesReferences
   ```

   Also inspect the PR body for closing keywords (`Fixes`, `Resolves`, `Closes`
   followed by `#NNN` or `<owner>/<repo>#NNN`). Normalize any matches to the
   `owner/repo#NNN` form — bare `#NNN` references default to
   `brave/brave-browser` — and append each to `RESOLVED_ISSUES`. If at least one
   issue is found this way, no new issue is needed — skip to the next PR.

2. **Create a new tracking issue** in `brave/brave-browser` (Brave's convention
   is that issues live in `brave-browser` while code lives in `brave-core`).
   Mirror the original PR's title and reference both PRs in the body so the
   relationship is discoverable from either direction.

   ```bash
   gh issue create --repo brave/brave-browser \
     --title "<original PR title>" \
     --body "$(cat <<'EOF'
   Tracking issue created retroactively for an uplift that lacked a linked
   issue.

   - Original PR: brave/brave-core#<PR_NUMBER>
   - Uplift PR: brave/brave-core#<UPLIFT_PR_NUMBER> (to <target-branch>)
   EOF
   )"
   ```

   Capture the new issue number from the URL returned by `gh issue create`, and
   append `brave/brave-browser#<ISSUE_NUMBER>` to `RESOLVED_ISSUES`.

3. **Close the new issue** (the original PR has already merged, so the work the
   issue describes is complete):

   ```bash
   gh issue close <ISSUE_NUMBER> --repo brave/brave-browser \
     --comment "Closing — work landed in brave/brave-core#<PR_NUMBER>. Uplifted in brave/brave-core#<UPLIFT_PR_NUMBER>."
   ```

4. **Cross-link from the PRs**: post one short comment on the original PR and
   one on the uplift PR pointing at the new issue, so future readers of either
   PR can find the tracking issue.
   ```bash
   gh pr comment <PR_NUMBER> --repo brave/brave-core \
     --body "Tracking issue: brave/brave-browser#<ISSUE_NUMBER> (created for uplift to <target-branch>)."
   gh pr comment <UPLIFT_PR_NUMBER> --repo brave/brave-core \
     --body "Tracking issue for #<PR_NUMBER>: brave/brave-browser#<ISSUE_NUMBER>."
   ```

The uplift PR itself remains open — only the newly created tracking issue is
closed.

If issue creation fails for any reason (e.g., permission issues or rate limits),
record the failure in the summary and continue rather than aborting the rest of
the run.

### Add `Resolves` directives to the uplift PR body

After every included PR has been processed, insert one `Resolves <ref>` line
into the uplift PR body for each entry in `RESOLVED_ISSUES`. Place the new lines
**immediately after the last `Uplift of #XXXX` line** (i.e. underneath the list
of uplifts, before the blank line preceding `## Included PRs`). The opening of
the body should end up looking like:

```
Uplift of #XXXX
Uplift of #YYYY
Resolves brave/brave-browser#AAA
Resolves brave/brave-browser#BBB

## Included PRs
...
```

Fetch the current body, splice the lines in at the correct position, then update
the PR. One way to do the splice in bash:

```bash
CURRENT_BODY=$(gh pr view <UPLIFT_PR_NUMBER> --repo brave/brave-core --json body --jq .body)
NEW_BODY=$(printf '%s' "$CURRENT_BODY" | python3 -c '
import sys
body = sys.stdin.read()
resolves = ["Resolves brave/brave-browser#AAA", "Resolves brave/brave-browser#BBB"]
lines = body.splitlines()
idx = max((i for i, l in enumerate(lines) if l.startswith("Uplift of #")), default=-1)
out = lines[:idx+1] + resolves + lines[idx+1:] if idx >= 0 else resolves + [""] + lines
print("\n".join(out))
')
gh pr edit <UPLIFT_PR_NUMBER> --repo brave/brave-core --body "$NEW_BODY"
```

Replace the example `AAA`/`BBB` references with one line per entry in
`RESOLVED_ISSUES`, in discovery order. If `RESOLVED_ISSUES` ended up empty
(e.g., every lookup and issue creation failed), leave the body untouched.

---

## Step 8: Summary

After creating the PR, output a clear summary to the user:

### Uplifted:

List each included PR with its number, title, and merge commit SHA.

### Not Uplifted:

List each excluded PR with its number, title, and the reason it was excluded
(e.g., "not merged", "already uplifted (has uplift label)", "already in target
branch", "outside requested scope", "cherry-pick conflict").

### Tracking Issues Created:

For each included PR that lacked a linked issue, list the original PR number and
the new tracking issue number (e.g., `#34501 → brave-browser#52310, closed`). If
no new issues were needed, say so explicitly. If any issue creation failed, list
the PR and the reason.

### PR Link:

Provide the URL of the newly created uplift PR.
