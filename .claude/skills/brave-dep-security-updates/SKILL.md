---
name: brave-dep-security-updates
description: Use when CI audit findings or security alerts require dependency updates in brave-core, web-discovery-project, leo, or Rust packages - handles GHSA and RUSTSEC advisories using CI workflows (socket-fix.yml, update-dep.yml) and the update-crate script
---

# Brave Dependency Security Updates

## Overview

Manages GHSA and RUSTSEC security advisories across the brave-core ecosystem. **Prefer CI workflows over manual fixes.** All package.json changes use pinned exact versions only. Never manually edit `package-lock.json`, `DEPS`, or `Cargo.lock`.

## Repo Path Setup

Before starting, check memory for known paths. If missing, ask the user:

| Variable | Repo |
|---|---|
| `BRAVE_CORE` | brave-core (default: git root of cwd) |
| `WDP_DIR` | web-discovery-project |
| `LEO_DIR` | leo |

**Save paths to memory once provided.**

---

## Step 1: Read the Issue

The user will provide a GitHub issue link. Read it:

```bash
gh issue view <issue-url>
```

Extract from the issue body:
- GHSA ID(s) (`GHSA-xxxx-xxxx-xxxx`) or RUSTSEC ID(s) (`RUSTSEC-YYYY-NNNN`)
- Any context about affected packages

---

## Step 2: Identify Affected Package

Run the identification script with the advisory ID:

```bash
export BRAVE_CORE="/path/to/brave-browser/src/brave"
export WDP_DIR="/path/to/web-discovery-project"   # if known
export LEO_DIR="/path/to/leo"                      # if known

python3 ~/.claude/skills/brave-dep-security-updates/identify-affected.py GHSA-xxxx-xxxx-xxxx
```

The script checks `npm audit --json` in each repo for the GHSA ID, and for RUSTSEC runs `cargo audit --json` in `tools/crates`. It reports which packages and repos are affected.

If a repo path is unknown, ask the user and save to memory.

---

## Step 3: Build Fix Plan

Create a `TodoWrite` task list based on what was found. Each affected repo is a separate task. Common combinations:

| Location | Fix |
|---|---|
| web-discovery-project | `socket-fix.yml` in WDP → `update-dep.yml` in brave-core |
| @brave/leo | `socket-fix.yml` in leo → update commit hash in brave-core `package.json` |
| brave-core JS | `socket-fix.yml` in brave-core |
| Rust vendored crate | `npm run update_brave_tools_crates -- --update-crate=<name>@<version>` locally |
| Multiple | Ordered: fix deps first, then brave-core |

---

## Step 4: Execute Fixes via CI

### Fix A — web-discovery-project (GHSA)

The workflow opens a PR automatically. Pass the issue link so the body is set correctly:

```bash
gh workflow run socket-fix.yml \
  --repo brave/web-discovery-project \
  -f ghsa_ids="GHSA-xxxx-xxxx-xxxx" \
  -f issue_link="<issue-url>"
```

If the WDP workflow doesn't support `issue_link`, add a comment after the PR is created:
```bash
gh pr comment <PR_NUMBER> --repo brave/web-discovery-project --body "related to <issue-url>"
```

Monitor the PR:
```bash
gh pr list --repo brave/web-discovery-project --head "socket-fix/*"
```

Once the WDP PR is open (no need to wait for merge), capture its HEAD commit and trigger the DEPS update:

```bash
WDP_COMMIT=$(gh pr view <PR_NUMBER> --repo brave/web-discovery-project --json headRefOid --jq '.headRefOid')

gh workflow run update-dep.yml \
  --repo brave/brave-core \
  --ref master \
  -f dep="vendor/web-discovery-project" \
  -f ref="$WDP_COMMIT"
```

Monitor the brave-core DEPS PR and edit its body to add `closes <issue-url>`:
```bash
gh pr list --repo brave/brave-core --head "update-dep/*"
gh pr edit <PR_NUMBER> --repo brave/brave-core --body "$(gh pr view <PR_NUMBER> --repo brave/brave-core --json body --jq '.body')"$'\n\n'"closes <issue-url>"
```

### Fix B — @brave/leo (GHSA)

Trigger socket-fix in leo. Pass the issue link so the body is set correctly:

```bash
gh workflow run socket-fix.yml \
  --repo brave/leo \
  -f ghsa_ids="GHSA-xxxx-xxxx-xxxx" \
  -f issue_link="<issue-url>"
```

If the Leo workflow doesn't support `issue_link`, add a comment after:
```bash
gh pr comment <PR_NUMBER> --repo brave/leo --body "related to <issue-url>"
```

After the leo PR merges, update the commit hash in brave-core `package.json` (~line 205):

```bash
LEO_COMMIT=$(gh pr view <PR_NUMBER> --repo brave/leo --json mergeCommit --jq '.mergeCommit.oid')
# Edit brave-core package.json:
# "@brave/leo": "github:brave/leo#<LEO_COMMIT>",
```

Then open a brave-core PR with that change with body `closes <issue-url>`.

### Fix C — brave-core JS (GHSA)

The workflow opens a PR with `CI/skip` and `security` labels and sets the body to `closes <issue-url>` automatically:

```bash
gh workflow run socket-fix.yml \
  --repo brave/brave-core \
  -f ghsa_ids="GHSA-xxxx-xxxx-xxxx" \
  -f issue_link="<issue-url>"
```

Monitor:
```bash
gh pr list --repo brave/brave-core --head "socket-fix/*"
```

### Fix D — Rust vendored crate (RUSTSEC)

Find the crate name and safe version from the advisory, then run locally in brave-core:

```bash
cd "$BRAVE_CORE"
npm run update_brave_tools_crates -- --update-crate=<crate-name>@<version>
```

This patches only that crate in the vendor directory, updates `Cargo.lock`, and regenerates `.cargo-checksum.json` — no collateral version bumps from a full re-vendor. Then open a PR:

```bash
git checkout -b "security/rustsec-<id>"
git add tools/crates/
git commit -m "fix: update <crate> to <version> (RUSTSEC-YYYY-NNNN)"
# Ask user permission before pushing
git push origin HEAD
gh pr create \
  --title "fix: update <crate> to <version> (RUSTSEC-YYYY-NNNN)" \
  --body "closes <issue-url>" \
  --base master \
  --label "CI/skip"
```

---

## Step 5: PR Body Conventions

| Repo | PR body | How |
|---|---|---|
| brave/web-discovery-project | `related to <issue-url>` | `-f issue_link=` if supported, else `gh pr comment` |
| brave/leo | `related to <issue-url>` | `-f issue_link=` if supported, else `gh pr comment` |
| brave/brave-core | `closes <issue-url>` | `-f issue_link=` (workflow sets it automatically) |

**Always pass `issue_link` when triggering socket-fix workflows.** If the workflow supports it, the body is set at PR creation time — no follow-up needed. Only fall back to `gh pr comment` for repos whose workflow doesn't have the `issue_link` input yet.

---

## Step 6: Uplift

Once all brave-core PRs are merged, run `/uplift` to create uplifts to beta and release branches.

---

## Step 7: Print PR Summary

Print a final list:

```
## PRs for <advisory ID>

| Repo | PR | Status |
|---|---|---|
| brave/web-discovery-project | #NNNN <url> | open/merged |
| brave/brave-core (DEPS/fix) | #NNNN <url> | open/merged |
| brave/brave-core (uplift beta) | #NNNN <url> | open |
| brave/brave-core (uplift release) | #NNNN <url> | open |
```

---

## General Rules

- **Always use pinned exact versions** — never ranges (`>=`, `^`, `~`)
- **Always use PRs** — never push directly to `main`/`master`
- **Never push or open a PR without user permission** — prepare, then ask
- `update-dep.yml` can run against a WDP commit still in PR review — always ask permission first
- Never manually edit `package-lock.json`, `DEPS`, or `Cargo.lock`
- Verify `npm audit` resolves the advisory before committing manual changes

## Quick Reference

| Advisory | Location | Primary CI | brave-core follow-up |
|---|---|---|---|
| GHSA | web-discovery-project | `socket-fix.yml` in WDP | `update-dep.yml` |
| GHSA | @brave/leo | `socket-fix.yml` in leo | manual hash bump in package.json |
| GHSA | brave-core JS | `socket-fix.yml` in brave-core | none |
| RUSTSEC | vendored crate | local `--update-crate` flag | PR with crate changes |
| Any | no fix / breaking upgrade | `create_pull_request.yml` in audit-config | none |

## Audit-Config (No Fix Available)

When a vulnerability has no fix or requires a major update that would cause a breaking change, use the `create_pull_request.yml` workflow in `brave/audit-config` — it handles the config.json edit and PR creation automatically:

```bash
gh workflow run create_pull_request.yml \
  --repo brave/audit-config \
  -f advisory="GHSA-xxxx-xxxx-xxxx" \
  -f issue="<issue-url>" \
  -f comment="<optional reason, e.g. no fix available / breaking upgrade>"
```

The workflow accepts GHSA IDs, RUSTSEC IDs, or full advisory URLs. It opens a PR in `brave/audit-config` titled "Ignore `<advisory-id>`".

## Common Mistakes

| Mistake | Fix |
|---|---|
| Using version ranges | Pin exactly: `"3.4.2"` not `">=3.4.2"` |
| Waiting for WDP/Leo PR to merge before `update-dep.yml` | Trigger immediately using the PR's HEAD commit |
| Forgetting `CI/skip` on brave-core manual PRs | `socket-fix.yml` adds it automatically; add manually for Rust PRs |
| Running full `update_brave_tools_crates` for a single crate | Use `--update-crate=<name>@<version>` to avoid collateral bumps |
| Pushing without permission | Always ask before pushing or creating PRs |
