---
name: pr
description: Create a pull request for the current branch using `gh`.
---

# Create Pull Request

Create a pull request for the current branch using `gh`.

## Current State

- Branch: !`git branch --show-current`
- Commits on branch: !`git log master..HEAD --oneline`
- Changed files: !`git diff master...HEAD --stat`

## Steps

### 1. Gather context

- Review the commits and changed files above.
- Check if the branch is associated with a GitHub issue (look at branch name,
  commit messages, or recent context for an issue number).

### 2. Push the branch

Run `git push -u origin HEAD`.

### 3. Create the PR

Use `gh pr create` with:

- **Title:** Short, direct summary of the change (under 70 chars).
- **Body:** A brief, factual summary of what changed and why. No filler. If
  there is an associated issue, include
  `Fix https://github.com/brave/brave-browser/issues/<number>` on its own line
  at the top of the body.

Keep the description concise. Just state what was done.

### 4. Done

Print the PR URL for the user.
