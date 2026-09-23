---
name: create-contributor-prs
description:
  For an external contributor's open PRs to brave/brave-core, ensure the
  `sync-and-rebase-pr-from-fork.yml` workflow has been dispatched for each PR's
  latest commit SHA — so a `contributor-*` draft PR exists inside
  brave/brave-core and CI can run. Invoke when the user says something like
  "create contributor PRs for <user>", "sync contributor PRs <list>", "run the
  rebase-from-fork workflow for these PRs", or passes a GitHub username / list
  of PR URLs/numbers.
---

# create-contributor-prs

External contributors fork brave/brave-core and open PRs from their fork. Most
of Brave's CI doesn't run on fork PRs for security reasons. The
`sync-and-rebase-pr-from-fork.yml` workflow exists to create an in-org mirror
branch (`contributor-<owner>-<headRefName>`) and a draft PR in brave/brave-core
that re-triggers full CI on the rebased commit.

This skill dispatches that workflow for each open PR by a contributor (or each
PR in an explicit list), but only when a dispatch hasn't already happened for
the PR's current HEAD commit.

## Inputs

The user invokes this skill with one of:

1. **A GitHub username** — e.g. `create-contributor-prs jharris-tech`. Resolve
   to all open PRs they authored against `brave/brave-core`.
2. **A list of PRs** — any mix of:
   - Full URL: `https://github.com/brave/brave-core/pull/36402`
   - Short form: `#36402` or `36402`
   - Cross-repo form: `brave/brave-core#36402`

If the user gives both a username and PR list, treat them as additional filters
(union of both).

If no input is given, ask for one. Don't proceed without explicit scope.

## Repo

Always `brave/brave-core` unless the user explicitly names a different repo.
Pass `--repo brave/brave-core` to every `gh` call. Don't assume the cwd matters.

## Workflow facts (don't re-derive)

`.github/workflows/sync-and-rebase-pr-from-fork.yml` inputs:

- `PR_NUMBER` (optional) — number of the contributor PR
- `COMMIT_HASH` (required, 40-char hex) — the contributor PR's HEAD commit SHA

The workflow:

- Requires the PR to be cross-repo (`isCrossRepository: true`). It exits 1
  otherwise — so filter non-cross-repo PRs out client-side, don't dispatch them.
- Checks out the given `COMMIT_HASH` from the contributor's fork, rebases onto
  the PR's base branch, force-pushes to
  `refs/heads/contributor-<headRepositoryOwner>-<headRefName>` inside
  brave/brave-core.
- Creates a draft PR titled `CI run for contributor PR #<PR_NUMBER>` if one
  doesn't already exist for that contributor branch.

GitHub does NOT expose `workflow_dispatch` inputs via the runs API. To find out
which `COMMIT_HASH` a past run used, you must read the run's log.

## Procedure

### Step 1 — Resolve PRs

For a username:

```bash
gh pr list --repo brave/brave-core --author <USER> --state open \
  --json number,title,url,headRefOid,headRepositoryOwner,headRepository,isCrossRepository,headRefName \
  --limit 100
```

For each user-supplied PR number `N`:

```bash
gh pr view N --repo brave/brave-core \
  --json number,title,url,headRefOid,headRepositoryOwner,headRepository,isCrossRepository,headRefName,state,author
```

Combine into one list. Drop any PR where `isCrossRepository == false` (the
workflow rejects same-repo PRs) and report them as "skipped: not cross-repo".
Drop any PR not in `OPEN` state and report it as "skipped: not open".

### Step 2 — Check whether the workflow already ran for each PR's current HEAD

For each PR, compare its `headRefOid` against the `COMMIT_HASH` inputs of recent
workflow runs.

Pull a window of recent runs once, up front (don't refetch per-PR):

```bash
gh run list --repo brave/brave-core \
  --workflow=sync-and-rebase-pr-from-fork.yml \
  --limit 100 \
  --json databaseId,status,conclusion,createdAt,event
```

For each candidate run (filter to `event == "workflow_dispatch"`), the
`COMMIT_HASH` input appears in the job log on a line like
`  COMMIT_HASH: <40-hex>`. Fetching the full log per run is slow, so batch-fetch
only as many as needed: iterate runs newest-first and stop early once every
target PR's HEAD SHA is either matched or determined absent in the window.

Per-run log fetch + extract:

```bash
gh run view <RUN_ID> --repo brave/brave-core --log 2>/dev/null \
  | grep -oE 'COMMIT_HASH: [0-9a-f]{40}' \
  | head -1 \
  | awk '{print $2}'
```

Build a set `dispatched_shas`. Also record per-SHA whether the run is
`in_progress` / `queued` / `completed` (success or failure) — useful for
reporting.

If the run window of 100 doesn't cover far enough back (e.g. the contributor's
HEAD commit is older than the oldest run in the window), expand the window with
`--limit 200` etc., or accept that you may re-dispatch. The workflow is
idempotent (force-push + skip-if-exists draft PR), so a duplicate dispatch is
wasteful CI but not destructive. Mention this tradeoff if you bail on the
lookback early.

### Step 3 — Dispatch the workflow where needed

For each PR whose `headRefOid` is NOT in `dispatched_shas`:

```bash
gh workflow run sync-and-rebase-pr-from-fork.yml \
  --repo brave/brave-core \
  -f PR_NUMBER=<N> \
  -f COMMIT_HASH=<headRefOid>
```

`gh workflow run` returns nothing useful on success and exits non-zero on
failure. Capture stderr.

Do NOT pass `--ref` — let it default to the workflow's default branch (master).
The workflow operates on the input commit hash, not the ref.

### Step 4 — Report

Print a compact table per PR:

```
PR     SHA       Status
36402  3807075d  already dispatched (in_progress, run 26407241424)
36510  abc1234d  dispatched now
36588  9988aabb  skipped: not cross-repo
36601  77665544  skipped: closed
```

End with a one-line summary: `Dispatched N, already-running M, skipped K.`

If any dispatch failed, list the PR + the stderr from `gh workflow run`.

## Confirmation before dispatching

Each dispatch consumes CI minutes and creates/updates a draft PR in the org.
Before triggering a batch dispatch, show the list of PRs you're about to
dispatch for and ask the user to confirm — unless they explicitly said "just do
it" / "no confirmation" / passed something like `--yes`. A single-PR dispatch
can proceed without re-confirming if the user clearly named that one PR.

## Things to avoid

- Don't dispatch for non-cross-repo PRs — the workflow exits 1.
- Don't dispatch for closed/merged PRs.
- Don't use `display_title` or `head_sha` of the workflow run to infer the
  COMMIT_HASH input. `head_sha` is the SHA of `master` at dispatch time, not the
  input. Only the log has the real input.
- Don't fetch every run's log up front — stop early when all target SHAs are
  resolved.
- Don't comment on or modify the contributor's PR. This skill only triggers a
  workflow.
