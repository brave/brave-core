---
name: uplift
description: "Create an uplift PR that cherry-picks intermittent test fixes and crash fixes from closed PRs into a target branch (beta or release). Triggers on: /uplift, create uplift, uplift PRs."
argument-hint: [github-username] [beta|release] [all|<num>d|PR1,PR2,PR3]
disable-model-invocation: true
allowed-tools: Bash, Read, WebFetch, Grep, Glob
---

# Uplift PR Creator

Create an uplift pull request that cherry-picks intermittent test fixes and
crash fixes from a contributor's recently closed/merged PRs into a target
channel branch.

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

Parse the arguments by splitting `$ARGUMENTS` on whitespace. Examples:

- `/uplift netzenbot` → username=`netzenbot`, channel=`beta`, filter=recent 50
- `/uplift netzenbot beta` → username=`netzenbot`, channel=`beta`, filter=recent
  50
- `/uplift netzenbot release` → username=`netzenbot`, channel=`release`,
  filter=recent 50
- `/uplift netzenbot beta all` → username=`netzenbot`, channel=`beta`,
  filter=all PRs (past 30 days)
- `/uplift netzenbot beta 10d` → username=`netzenbot`, channel=`beta`,
  filter=all PRs (past 10 days)
- `/uplift netzenbot release 60d` → username=`netzenbot`, channel=`release`,
  filter=all PRs (past 60 days)
- `/uplift netzenbot release 33534,33547,33580` → username=`netzenbot`,
  channel=`release`, filter=only those 3 PRs

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
either **include** or **exclude** for the uplift:

**INCLUDE** if the PR is any of:

- Intermittent/flaky test fix (titles often contain "Fix flaky", "Fix test:",
  "Fix intermittent", "Disable flaky")
- Crash fix (titles mention "crash", "null dereference",
  "EXCEPTION_ACCESS_VIOLATION", etc.)
- Test filter updates (disabling broken upstream tests, updating stale filter
  entries)

**EXCLUDE** if the PR is:

- A feature addition (not a fix)
- A refactor unrelated to test stability or crashes
- Already has the uplift label for the target channel (check the `labels` array
  in the PR JSON for `uplift/beta` or `uplift/release` depending on the target
  channel)
- Not merged (`mergedAt` is null)

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
  updates (no crash fixes): `Uplift intermittent test fixes to <target-branch>`
- If the uplift contains **only** crash fixes (no test fixes):
  `Uplift crash fixes to <target-branch>`
- If the uplift contains **both** test fixes and crash fixes:
  `Uplift intermittent test fixes and crash fixes to <target-branch>`

### Body Format

Only list the PRs being uplifted. Do NOT mention excluded PRs in the PR body.

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

## Step 7: Summary

After creating the PR, output a clear summary to the user:

### Uplifted:

List each included PR with its number, title, and merge commit SHA.

### Not Uplifted:

List each excluded PR with its number, title, and the reason it was excluded
(e.g., "not merged", "already uplifted (has uplift label)", "already in target
branch", "not a test fix or crash fix", "cherry-pick conflict").

### PR Link:

Provide the URL of the newly created uplift PR.
