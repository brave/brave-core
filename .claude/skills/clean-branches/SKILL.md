---
name: clean-branches
description:
  'Delete local branches whose PRs have been merged upstream. Checks GitHub PR
  status for each branch. Triggers on: clean branches, delete merged branches,
  prune branches, branch cleanup.'
---

# Clean Branches - Delete Merged Local Branches

Clean up local branches by checking their GitHub PR status and deleting those
that have been merged.

## Steps

1. **Fetch and list branches** — Fetch the latest remote state, then get all
   local branches except `master`:

```bash
git fetch origin master
git branch --format='%(refname:short)' | grep -v '^master$'
```

Show the list to the user. If no branches exist besides `master`, report that
and stop.

2. **Check merge status for each branch** — Use multiple methods to detect
   merges (including squash merges):

**Method 1: Direct ancestry** — Catches regular merges and fast-forwards:

```bash
git branch --merged origin/master --format='%(refname:short)'
```

**Method 2: Squash merge detection via `git cherry`** — For branches not caught
by method 1, check if all commits have equivalent patches in `origin/master`. A
`-` prefix means the patch already exists upstream:

```bash
git cherry -v origin/master <branch>
```

If ALL lines start with `-` (or output is empty), the branch has been
squash-merged.

**Method 3: GitHub PR status** (if `gh` is authenticated) — For remaining
branches:

```bash
# Check for merged PRs
gh pr list --head <branch> --state merged --json number,title,url

# Check for closed (not merged) PRs
gh pr list --head <branch> --state closed --json number,title,url,mergedAt
```

A PR is "closed without merge" when it appears in the closed list but has an
empty/null `mergedAt` field. If `gh` is not authenticated, skip this method.

**Important:** Always compare against `origin/master` (not local `master`) to
detect merges even when local master is stale.

3. **Categorize branches** into these groups:

   - **Merged**: Detected as merged by any method above → safe to delete
   - **Closed (not merged)**: PR was closed without merging → flag for user
     decision
   - **No PR / Open PR**: No PR found (and not detected as merged), or PR is
     still open → skip

4. **Display categorized list** — Show the user a summary table with:

   - Branch name
   - Category (Merged / Closed / No PR / Open)
   - PR number and title (if available)
   - Detection method (ancestry / squash / PR status)

5. **Confirm with user** — Ask the user which branches to delete:

   - Pre-select "Merged" branches for deletion
   - Ask about "Closed (not merged)" branches individually
   - Never auto-delete "No PR" or "Open PR" branches

6. **Delete confirmed branches** — If currently on a branch being deleted,
   switch to `master` first and fast-forward it:

```bash
git checkout master && git merge --ff-only origin/master
git branch -D <branch1> <branch2> ...
```

After deleting, prune stale remote tracking references:

```bash
git remote prune origin
```

7. **Report results** — Summarize:
   - How many branches were deleted
   - Which branches were kept and why
