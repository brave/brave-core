---
name: check-milestones
description: "Check and fix milestones on PRs and their associated issues. Sets PR milestones based on the branch they merged into, and issue milestones based on the smallest version with a merged uplift. Triggers on: check milestones, fix milestones, /check-milestones."
argument-hint: [github-username] [all|PR1,PR2,PR3]
disable-model-invocation: true
allowed-tools: Bash, Read, WebFetch, Grep, Glob
---

# Check Milestones

Check and fix milestones on a contributor's merged PRs and their associated
GitHub issues.

## Inputs

- **Arguments**: `$ARGUMENTS` — space-separated values:
  - First argument: **GitHub username** (the author whose merged PRs to check)
  - Second argument (optional): **PR filter** — either:
    - `all` — evaluate all closed/merged PRs from this author in the past 30
      days. Use
      `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url,baseRefName,milestone --jq 'sort_by(.mergedAt)'`
      where `YYYY-MM-DD` is 30 days ago.
    - A comma-separated list of PR numbers with no spaces (e.g.,
      `33534,33547,33580`) — only evaluate these specific PRs. Fetch each one
      individually with
      `gh pr view <number> --repo brave/brave-core --json number,title,mergedAt,mergeCommit,labels,body,url,baseRefName,milestone`.
    - If omitted, defaults to the recent 50 closed PRs.

Parse the arguments by splitting `$ARGUMENTS` on whitespace. Examples:

- `/check-milestones netzenbot` → username=`netzenbot`, filter=recent 50
- `/check-milestones netzenbot all` → username=`netzenbot`, filter=all PRs (past
  30 days)
- `/check-milestones netzenbot 33534,33547,33580` → username=`netzenbot`,
  filter=only those 3 PRs

---

## Step 1: Gather Information

Run these in parallel:

1. **Fetch PRs** (method depends on the second argument):

   - **Default (no second arg)**: Use
     `gh pr list --repo brave/brave-core --author <username> --state closed --limit 50 --json number,title,mergedAt,mergeCommit,labels,body,url,baseRefName,milestone --jq 'sort_by(.mergedAt)'`
   - **`all`**: Use
     `gh pr list --repo brave/brave-core --author <username> --state closed --limit 200 --search "closed:>YYYY-MM-DD" --json number,title,mergedAt,mergeCommit,labels,body,url,baseRefName,milestone --jq 'sort_by(.mergedAt)'`
     where `YYYY-MM-DD` is 30 days ago from today.
   - **Comma-separated PR list**: For each PR number, use
     `gh pr view <number> --repo brave/brave-core --json number,title,mergedAt,mergeCommit,labels,body,url,baseRefName,milestone`.
     Collect results into a list sorted by `mergedAt`.

   Skip any PR where `mergedAt` is null (not merged).

2. **Fetch the release schedule**: Fetch the content at
   `https://github.com/brave/brave-browser/wiki/Brave-Release-Schedule` and find
   the "Current channel information" table. Extract the version numbers for each
   channel:

   - **Nightly** row → version (e.g., `1.89.x`)
   - **Beta** row → version (e.g., `1.88.x`)
   - **Release** row → version (e.g., `1.87.x`)

   These map to milestone names:

   - `<nightly-version> - Nightly` (e.g., `1.89.x - Nightly`)
   - `<beta-version> - Beta` (e.g., `1.88.x - Beta`)
   - `<release-version> - Release` (e.g., `1.87.x - Release`)

---

## Step 2: Determine Correct PR Milestones

For each merged PR, determine the correct milestone based on its `baseRefName`
(the branch it was merged into):

- If `baseRefName` is `master` or `main` → milestone is
  `<nightly-version> - Nightly`
- If `baseRefName` matches the beta branch (e.g., `1.88.x`) → milestone is
  `<beta-version> - Beta`
- If `baseRefName` matches the release branch (e.g., `1.87.x`) → milestone is
  `<release-version> - Release`
- If `baseRefName` is some other branch → skip (cannot determine milestone)

Compare the PR's current milestone (from the `milestone` field) against the
correct one. If they differ (or the milestone is not set), add the PR to the
list of PRs that need updating.

---

## Step 3: Determine Correct Issue Milestones

For each merged PR, extract the associated issue number(s) from the PR body.
Look for patterns like:

- `Resolves #XXXXX` or `Resolves brave/brave-browser#XXXXX`
- `Fixes #XXXXX` or `Fixes brave/brave-browser#XXXXX`
- `Fixes https://github.com/brave/brave-browser/issues/XXXXX`
- `Resolves https://github.com/brave/brave-browser/issues/XXXXX`

For each associated issue:

1. **Find all PRs that reference this issue** — check if the issue has uplift
   PRs by searching for other merged PRs that reference the same issue across
   different branches. You can use:

   ```bash
   gh api search/issues --method GET \
     -f q="repo:brave/brave-core is:pr is:merged $ISSUE_NUMBER" \
     --jq '.items[] | {number, title, html_url, pull_request}'
   ```

   Or check the PR's labels for `uplift/beta` and `uplift/release` to identify
   if uplift PRs exist.

2. **Determine which branches this fix has been merged into** (considering
   uplifts):

   - The original PR's base branch (e.g., `master`)
   - If there is a **merged** uplift PR to beta → the beta branch
   - If there is a **merged** uplift PR to release → the release branch
   - Uplifts that are NOT merged do not count

3. **The correct issue milestone is the smallest version** where the fix is
   merged:

   - If merged into release branch → milestone is `<release-version> - Release`
   - Else if merged into beta branch → milestone is `<beta-version> - Beta`
   - Else if merged into master → milestone is `<nightly-version> - Nightly`

4. **Check the issue's current milestone** and compare:
   ```bash
   gh issue view <ISSUE_NUMBER> --repo brave/brave-browser --json milestone --jq '.milestone.title'
   ```

---

## Step 4: Apply Milestone Updates

### Update PR Milestones

For each PR that needs a milestone update:

```bash
gh pr edit <PR_NUMBER> --repo brave/brave-core --milestone "<milestone-name>"
```

### Update Issue Milestones

For each issue that needs a milestone update:

```bash
gh issue edit <ISSUE_NUMBER> --repo brave/brave-browser --milestone "<milestone-name>"
```

**Important**: Only update an issue's milestone to a **smaller** version number
than what is currently set. For example:

- Milestone is unset → set it to the computed milestone
- Milestone is `1.89.x - Nightly` but fix was uplifted to beta → update to
  `1.88.x - Beta`
- Milestone is `1.88.x - Beta` but fix was uplifted to release → update to
  `1.87.x - Release`

Do NOT update if the current milestone already has a smaller version number than
the computed one (e.g., do not change `1.87.x - Release` to `1.88.x - Beta`).

---

## Step 5: Summary

Output a clear summary of all actions taken:

### PR Milestones Updated:

| PR     | Title    | Base Branch | Old Milestone | New Milestone    |
| ------ | -------- | ----------- | ------------- | ---------------- |
| #XXXXX | PR title | master      | (none)        | 1.89.x - Nightly |

### Issue Milestones Updated:

| Issue                     | Old Milestone | New Milestone | Reason                        |
| ------------------------- | ------------- | ------------- | ----------------------------- |
| brave/brave-browser#XXXXX | (none)        | 1.88.x - Beta | Merged uplift to beta (#YYYY) |

### Already Correct:

List PRs and issues that already had the correct milestone (brief count or
list).

### Skipped:

List any PRs or issues that were skipped with the reason (e.g., "no associated
issue", "could not determine branch", "unmerged uplift").
