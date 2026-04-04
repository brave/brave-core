---
name: add-best-practice
description:
  'Add a new best practice to the appropriate doc. Checks for duplicates,
  assigns stable IDs, creates new category docs if needed. Triggers on: add best
  practice, new best practice, add bp, new bp.'
argument-hint: '<best practice description>'
disable-model-invocation: true
allowed-tools:
  Bash(python3:*), Bash(git:*), Bash(gh:*), Bash(ls:*), Read, Grep, Glob, Edit,
  Write, Agent, WebFetch
---

# Add Best Practice

Add a new best practice rule to the correct best-practices document. Ensures no
duplicates, assigns stable IDs, and follows the exact format of existing
entries.

---

## Step 1: Verify Working Directory

This skill runs from the brave-core (`src/brave`) directory. Best practices docs
are at `./docs/best-practices/` and the index is at `./docs/best_practices.md`.

Set `DOCS_DIR="./docs/best-practices"`.

---

## Step 2: Understand the Best Practice

Parse the argument to understand:

1. **What** the best practice is about (the rule, guideline, or pattern)
2. **Why** it matters (correctness, performance, readability, security, etc.)
3. **Category** it falls under (C++ coding, testing, architecture, build system,
   frontend, etc.)

**If the argument is a GitHub PR comment URL**, gather full context before
proceeding:

1. Fetch the review comment text via `gh api`
2. Fetch the PR diff around the commented file/line to understand the actual
   code being discussed:

   ```bash
   # Get the comment details
   gh api repos/<owner>/<repo>/pulls/comments/<comment_id> \
     --jq '{body, path, line, diff_hunk}'

   # Get surrounding context from the actual source file
   # Read the file at the path mentioned in the comment
   ```

3. Read the **actual source file** at the commented line to understand the full
   context — the `diff_hunk` alone is often insufficient
4. If the comment is part of a thread, fetch the entire thread to understand the
   full discussion

**CRITICAL: Do NOT rely solely on the comment text.** Review comments are often
brief and assume the reader can see the code. You MUST read the actual code
being reviewed to understand what the reviewer is flagging and why.
Misunderstanding the code context leads to incorrect best practices.

If the description is vague even after gathering context, ask the user to
clarify before proceeding.

---

## Step 3: Check for Duplicates

Search ALL existing best-practice documents for rules that already cover this
topic:

1. Read the index file `docs/best_practices.md` to understand document
   categories
2. Use Grep to search across all docs in `$DOCS_DIR/*.md` for keywords related
   to the new practice
3. Read any potentially matching sections in full to determine if they cover the
   same ground

**If a duplicate or near-duplicate is found:**

- Tell the user: "Not added — this is already covered by **[Rule Title]** in
  `<document>.md` (ID: `<ID>`)."
- If the existing rule is close but could be improved/updated with the new
  information, offer to update it instead
- Stop here unless the user wants to proceed

---

## Step 4: Choose the Target Document

Discover the available documents and their categories dynamically:

```bash
python3 ./.claude/skills/review/discover-bp-docs.py ./docs/best-practices/ --list-categories
```

This outputs JSON listing each document and its category (e.g., `cpp`, `test`,
`build`, `frontend`, `always`). Use the category and document name to determine
the best match for the new practice. Read the top of candidate documents if
needed to confirm the fit.

**If it doesn't fit any existing document AND the topic seems like it would
attract multiple related practices**, create a new category document:

1. Choose a descriptive filename (e.g., `performance.md`,
   `security-practices.md`)
2. Add a prefix mapping to `script/manage-bp-ids.py` in the `DOC_PREFIXES` dict
3. Add an entry in `docs/best_practices.md` index under the appropriate section
4. Create the new doc with a `# Title` header and cross-reference comment

**If it doesn't fit but is a one-off**, find the closest existing document and
add it there.

---

## Step 5: Draft the Best Practice Entry

Write the entry following the exact format used in existing docs. Read a few
entries from the target document first to match its style.

**Required format:**

````markdown
<a id="PLACEHOLDER"></a>

## ✅ Rule Title Here

**Bold principle statement explaining the rule.**

```language
// ❌ WRONG - brief explanation
<bad code example>

// ✅ CORRECT - brief explanation
<good code example>
```
````

Additional context, edge cases, or explanation if needed.

---

````

**Key formatting rules:**
- Use `✅` for positive practices ("Do this"), `❌` for anti-patterns ("Don't do this")
- Start with a bold principle statement
- Include WRONG and CORRECT code examples when applicable
- Use `---` horizontal rule after each entry
- Use `##` for top-level practices, `###` for sub-practices within a group
- Write `PLACEHOLDER` for the ID — the script will assign the real ID
- Keep the entry concise and actionable

**Show the drafted entry to the user and ask for approval before writing it.**

---

## Step 5.5: Review Subagent Validation

**CRITICAL: Before showing the draft to the user, launch a review subagent to
validate the entry.** This catches misunderstandings of the source material that
led to incorrect best practices in the past.

Launch an Agent with the following prompt structure:

```
You are reviewing a drafted best practice entry for accuracy. Your job is to
verify the draft correctly captures the issue from the source material.

SOURCE CONTEXT:
- Review comment: <the original review comment text>
- File: <path to the file being reviewed>
- Code context: <the diff_hunk and/or actual source code around the comment>
- Thread context: <any additional thread replies>

DRAFTED BEST PRACTICE:
<the full drafted entry>

VALIDATION CHECKLIST:
1. Does the draft accurately describe the problem shown in the code?
2. Are the code examples (WRONG/CORRECT) consistent with what the reviewer
   flagged?
3. Is the "why" explanation technically correct?
4. Could the draft be misinterpreted or applied incorrectly?
5. Does the draft introduce any claims not supported by the source material?

Return one of:
- PASS: <brief confirmation of accuracy>
- FAIL: <specific explanation of what's wrong and what the correct
  interpretation should be>
```

**If the subagent returns FAIL:**
- Revise the draft based on the subagent's feedback
- Re-run the subagent on the revised draft
- Repeat until PASS or ask the user for clarification

**If the subagent returns PASS:**
- Proceed to show the draft to the user

---

## Step 6: Add the Entry to the Document

1. Read the target document to find the right location for the new entry
2. **Entries must be ordered by ID** — since the new entry gets the next
   available ID (highest + 1), always append it at the end of the document
3. Make sure there's a `---` separator between entries

---

## Step 7: Assign the Stable ID

Determine the next available ID manually by finding the highest existing ID for the target document's prefix and incrementing by 1. Replace the `PLACEHOLDER` anchor with the new ID (e.g., `CS-042`).

- Never reuse old IDs or fill gaps — always increment from the highest existing number
- Do NOT use `manage-bp-ids.py --assign` for this step (it doesn't handle `PLACEHOLDER` anchors correctly)

---

## Step 8: Validate

Run validation to confirm the ID was assigned correctly and there are no duplicates:

```bash
python3 ./script/manage-bp-ids.py --validate
````

If validation fails, fix the issue before proceeding.

---

## Step 9: Commit and Offer to Branch + PR

After successfully adding the best practice, commit all changes as a single
atomic commit on the current branch. Use `--amend` to fold the ID assignment and
any related edits into one commit (do NOT create separate commits for the entry
and the ID assignment — they are one logical unit).

```bash
git add $DOCS_DIR/<document>.md
# Also add manage-bp-ids.py and docs/best_practices.md if they were modified
git commit -m "Add best practice: <short rule title>"
```

Then ask the user:

> "The best practice has been added to `<document>.md` with ID `<ID>` and
> committed. Would you like me to create a branch and open a PR?"

**If the user wants a PR:**

```bash
git stash  # stash the commit temporarily
git checkout -b best-practices-update-$(date +%Y%m%d-%H%M%S) HEAD~1
git stash pop
git add -A
git commit -m "Add best practice: <short rule title>"
git push -u origin HEAD
gh pr create --title "Add best practice: <short rule title>" --body "$(cat <<'EOF'
## Summary
- Adds new best practice to `<document>.md`: **<Rule Title>**
- ID: `<ID>`

EOF
)"
```

Report the PR URL if created.

---

## Important Guidelines

- **No Claude Code attribution** — do NOT include `Co-Authored-By`,
  `Generated with Claude Code`, or any other attribution in commits or PRs

- **Never reuse old IDs** — the `manage-bp-ids.py --assign` script handles this
  automatically by incrementing from the highest existing number
- **Never manually pick IDs** — always let the script assign them
- **Never fill gaps** in ID sequences — gaps are intentional (deleted practices)
- **Always check for duplicates first** — adding redundant rules wastes
  everyone's time
- **Match the style** of the target document exactly — read existing entries
  before writing
- **Keep entries concise** — best practices should be scannable, not essays
- **Include code examples** whenever possible — show don't tell
