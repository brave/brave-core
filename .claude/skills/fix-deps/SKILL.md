---
name: fix-deps
description: Use when handling GHSA security advisories, fixing vulnerable npm dependencies, or when user provides GHSA IDs, GitHub issue URLs with security alerts, or says "fix deps"
---

# Fix Dependency Security Advisories

Orchestrate supply chain security fixes across brave-core, brave/leo, and
brave/web-discovery-project using CI pipelines. The LLM automates tedious parts
(parsing input, formatting GHSA IDs, running scripts, interpreting output). All
fixes are performed by CI workflows — the local script only identifies affected
repos and triggers CI.

## Prerequisites

Set environment variables. Check memory for known paths, or ask the user:

```
export WDP_DIR="/path/to/web-discovery-project"
export LEO_DIR="/path/to/leo"
```

## Phase 1: Identify and Fix Upstream

**Step 1 — Parse input and identify affected repos:**

```bash
npm run fix_deps -- --dry-run <input>
```

Input formats: GHSA ID(s), GitHub issue URL, advisory URL. The script runs
`npm audit` across all 3 repos, traces dependency chains, and saves state to
`.fix_deps_state.json`.

**Step 2 — Review the identification output.** Verify the dependency chain
tracing is correct (which packages are direct vs transitive via Leo/WDP).

**Step 3 — Trigger upstream CI.** Run without `--dry-run`:

```bash
npm run fix_deps -- <input>
```

The script triggers `socket-fix.yml` in affected upstream repos (leo, WDP) and
polls for PRs. It also triggers `socket-fix.yml` in brave-core for any direct
(non-transitive) vulnerabilities.

**Step 4 — Report PR URLs to user.** Tell them to review and merge the upstream
PRs before continuing.

**STOP HERE.** Wait for user to confirm upstream PRs are merged.

## Phase 2: Bump Brave-Core

**Step 5 — After upstream PRs are merged, run:**

```bash
npm run fix_deps -- --complete
```

This checks upstream PR merge status, collects merge commit hashes, and triggers
`socket-fix.yml` in brave-core with `wdp_ref`/`leo_ref` inputs. The CI pipeline
updates DEPS, package.json, runs `socket fix` for remaining transitive deps, and
opens a single brave-core PR that closes the tracking issue.

**Step 6 — Report the brave-core PR URL.** The state file is cleaned up
automatically.

## Error Handling

| Situation | Action |
|---|---|
| `--complete` exits with code 5 | Upstream PR not merged yet — ask user to wait |
| `--complete` missing issue URL | Re-run Phase 1 with a GitHub issue URL as input |
| Workflow fails | `gh run list --repo <repo> --workflow socket-fix.yml` then `gh run view <id> --log-failed` |
| PR polling timeout (5 min) | Check if workflow is queued/running in GitHub Actions |
| No changes produced by socket fix | Advisory may already be resolved — verify with `npm audit` |

## CI Pipelines

| Repo | Workflow | Inputs |
|---|---|---|
| brave/leo | `socket-fix.yml` | `ghsa_ids` |
| brave/web-discovery-project | `socket-fix.yml` | `ghsa_ids` |
| brave/brave-core | `socket-fix.yml` | `ghsa_ids`, `issue_link` (required), `wdp_ref`, `leo_ref` |

## Quick Reference

```
npm run fix_deps -- GHSA-xxxx-xxxx-xxxx                    # Single advisory
npm run fix_deps -- GHSA-xxx,GHSA-yyy                      # Multiple (batched)
npm run fix_deps -- https://github.com/brave/brave-browser/issues/54048
npm run fix_deps -- --dry-run GHSA-xxxx                    # Identify only
npm run fix_deps -- --complete                             # Post-merge: trigger brave-core CI
```
