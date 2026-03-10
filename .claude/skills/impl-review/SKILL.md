---
name: impl-review
description:
  'Implement review feedback on a PR. Checks out the branch, applies
  reviewer-requested changes, runs preflight, commits, and pushes. Triggers on:
  impl-review, implement review, implement review feedback, address review
  comments.'
argument-hint: '<pr-number>'
---

# Implement Review Feedback

Checkout a PR's branch, implement the reviewer's feedback, run preflight checks,
commit, and push.

## Arguments

- **`<pr-number>`** (required) — The PR number in brave/brave-core to implement
  review feedback for.

## Confirmation

Always ask for user confirmation before implementing changes, before
committing/pushing, and before posting comments on GitHub.

## Steps

### 1. Parse Arguments

Extract the PR number from the arguments. It must be a numeric value. If no PR
number is provided, ask the user for one.

### 2. Fetch PR Details

```bash
gh pr view $PR_NUMBER --repo brave/brave-core --json headRefName,title,body,state,files,baseRefName
```

Extract:

- **Branch name** (`headRefName`) — the branch to checkout
- **Title** — for context
- **State** — must be `OPEN` (abort if closed/merged)
- **Base branch** — for understanding the diff context
- **Changed files** — to understand what the PR modifies

If the PR is not open, inform the user and stop.

### 3. Fetch Review Comments

Fetch review data from the PR:

```bash
gh api repos/brave/brave-core/pulls/$PR_NUMBER/reviews --paginate
gh api repos/brave/brave-core/pulls/$PR_NUMBER/comments --paginate
gh api repos/brave/brave-core/issues/$PR_NUMBER/comments --paginate
```

This provides:

- Reviews (approve/request changes/comment)
- Inline code review comments (with file paths and line numbers)
- Issue-level discussion comments

### 4. Analyze Review Feedback

Parse the review comments to understand what changes are requested:

1. **Identify actionable feedback** — Focus on explicit change requests from
   reviewers. Ignore:

   - Approvals without change requests
   - Informational comments that don't require action
   - Comments that have already been addressed (check commit timestamps vs
     comment timestamps)

2. **Categorize each requested change:**

   - Which file(s) need modification
   - What specific change is requested
   - Whether it affects production code, test code, or both

3. **Present the plan:**
   - How many review comments were found
   - Which are actionable vs already addressed
   - What changes you plan to make for each
   - Ask for user confirmation before proceeding

### 5. Checkout the Branch

```bash
git fetch origin
git checkout <headRefName>
git pull origin <headRefName>
```

Ensure you're on the correct branch and up to date with the remote.

### 6. Implement Changes

**CRITICAL: Only make changes the reviewer explicitly asks for.**

- Do NOT make any additional changes — no "while I'm here" cleanups, no extra
  refactoring, no renaming things the reviewer didn't mention
- If the reviewer asks you to fix one thing, fix exactly that one thing and
  nothing else
- Every extra change risks introducing issues and makes the reviewer's job
  harder
- Read the full source files for context before making changes (don't just look
  at the diff)
- Apply the same coding standards as the rest of the codebase

### 7. Run Preflight Checks

Invoke the preflight skill to validate all changes:

```
/preflight
```

This runs: best practices check, format, gn_check, presubmit, build, and
affected tests.

If preflight fails:

- Fix the issues it identifies
- Re-run preflight until it passes
- All fixes should still be scoped to the review feedback (don't fix unrelated
  issues)

### 8. Commit and Push

Before committing, show the user a summary of all changes (files modified, diff
stats) and ask for confirmation. If the user rejects, stop without committing.

**ALWAYS create a NEW separate commit** — never amend existing commits.

```bash
git add <changed-files>
git commit -m "$(cat <<'EOF'
Address review: <brief description of changes>
EOF
)"
git push
```

Commit message guidelines:

- Start with "Address review:" prefix
- Briefly describe what feedback was addressed
- Keep under 72 characters for subject line
- **DO NOT** include any `Co-Authored-By` line
- **DO NOT** use `--no-verify` or `--no-gpg-sign` flags

If multiple logical units of review feedback were addressed, consider separate
commits for each.

### 9. Post PR Comment

Show the draft comment to the user and ask for confirmation before posting. If
the user rejects, skip posting.

Post a summary comment on the PR:

```bash
gh pr comment $PR_NUMBER --repo brave/brave-core --body "$(cat <<'EOF'
Fixed: <description of what was changed>

<For each review point addressed, briefly note what was done>
EOF
)"
```

### 10. Report Summary

Output a summary to the user:

- PR number and title
- Number of review comments addressed
- What changes were made
- Preflight result (pass/fail)
- Commit SHA
- Link to the PR

---

## Important Rules

1. **Minimal changes** — Only implement what reviewers explicitly ask for
2. **New commits only** — Never amend; separate commits let reviewers track what
   changed
3. **No attribution** — No `Co-Authored-By` lines in commits
4. **User confirmation** — Present the plan before implementing, confirm before
   committing, confirm before posting comments
5. **Preflight required** — All changes must pass preflight before pushing

---

## Example Usage

```
/impl-review 12345
```

Asks for confirmation before implementing, committing, and posting.
