---
name: force-push-downstream
description:
  'Force-push a branch and all its downstream branches to origin. Auto-detects
  the downstream tree and skips branches already up-to-date. Triggers on: force
  push downstream, push chain, push all branches.'
argument-hint: '[starting-branch]'
disable-model-invocation: true
---

# Force Push Downstream

Force-push a branch and all its downstream branches to their respective remotes.
Skips branches whose local and remote tips already match.

## Arguments

- **`starting-branch`** (optional) — The branch to start from. Defaults to the
  current branch if not specified.

Examples:

- `/force-push-downstream` — push current branch + all downstream
- `/force-push-downstream candle-service-webui` — push from that branch down

## Current State

- Branch: !`git branch --show-current`
- Status: !`git status --short`

## Steps

### 1. Determine starting branch

If an argument was provided, use it as the starting branch. Otherwise use the
current branch.

```bash
git branch --show-current
```

### 2. Detect downstream chain

Run the detect-chain script (shared with rebase-downstream) to find all
downstream branches:

```bash
bash .claude/skills/rebase-downstream/detect-chain.sh <starting-branch>
```

### 3. Build push list

The push list is: the starting branch itself, followed by every downstream
branch in tree order (from detect-chain output).

### 4. Show plan and confirm

Display the branches that will be pushed:

```
Force push plan:
  candle-service-background-webcontents
    → background-webcontents-timer
      → candle-service-webui
        → candle-service-model-loading
          → brave-history-embeddings
          → local-ai-internals
```

For each branch, check if it is already up-to-date with origin:

```bash
# Get local and remote SHAs
local_sha=$(git rev-parse <branch>)
remote_sha=$(git rev-parse origin/<branch> 2>/dev/null || echo "none")
```

If `local_sha == remote_sha`, mark as `(up-to-date, will skip)`. If the remote
branch doesn't exist, mark as `(new, will push)`. Otherwise mark as
`(diverged, will force-push)`.

Ask the user to confirm before pushing.

### 5. Push each branch

For each branch in the push list:

1. **Skip if up-to-date**: If local SHA matches origin SHA, print skip message
   and move on.

2. **Force push**:

   ```bash
   git push --force-with-lease origin <branch>
   ```

   Use `--force-with-lease` for safety — it fails if someone else pushed to the
   remote since our last fetch, preventing accidental overwrites of others'
   work.

3. **If the remote branch doesn't exist** (first push), use:

   ```bash
   git push -u origin <branch>
   ```

4. **Report result**: Print success/failure for each branch.

### 6. Report summary

After all pushes complete, show a summary:

```
Force push results:
  ✓ candle-service-background-webcontents (pushed)
  - background-webcontents-timer (skipped, up-to-date)
  ✓ candle-service-webui (pushed)
  ✓ candle-service-model-loading (pushed)
  ✓ brave-history-embeddings (pushed, new)
  ✗ local-ai-internals (failed: ...)
```

## Important

- **Always use `--force-with-lease`** instead of `--force` for safety
- **Do NOT push** without user confirmation
- **Skip branches** that are already up-to-date (local == remote SHA)
- If any push fails, report it but continue with remaining branches
- Return to the original branch at the end
