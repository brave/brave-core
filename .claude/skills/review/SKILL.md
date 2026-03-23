---
name: review
description:
  'Review code for quality, root cause analysis, and fix confidence. Supports PR
  review and local review of uncommitted/branch changes. Default mode is local
  (reviews current branch changes). Triggers on: review pr, review this pr,
  /review <pr_url>, /review local, /review, check bot pr quality.'
allowed-tools:
  Bash(gh issue list:*), Bash(gh issue view:*), Bash(gh pr list:*), Bash(gh pr
  view:*), Bash(gh pr diff:*), Bash(git diff:*), Bash(git log:*), Bash(git
  status:*), Bash(git merge-base:*), Bash(git branch:*), Bash(git rev-parse:*),
  Read, Edit, Grep, Glob
---

# Code Review Skill

Perform a comprehensive review of code changes. Supports two modes:

- **Local mode** (default): Reviews uncommitted changes + current branch's diff
  from master
- **PR mode**: Reviews a specific pull request by URL or number

---

## The Job

### Determine Review Mode

Parse the arguments to determine the mode:

- `/review` or `/review local` → **Local mode**
- `/review <pr_url>` or `/review <pr_number>` → **PR mode**

**If no argument is provided or the argument is `local`, use local mode.**

---

## Paths

This skill runs from the `src/brave` (brave-core) directory.

- Chromium source (`src/`) is at `../` relative to this repo

---

## Local Mode

When reviewing local changes, gather the diff from two sources:

### Step L1: Determine the Base Branch

The base branch is what this branch's changes should be compared against. Do NOT
assume `master` — the branch may depend on another feature branch.

Detect the base branch in this order:

1. **Check for an existing PR**: If a PR exists for this branch, use its base
   branch:

   ```bash
   CURRENT_BRANCH=$(gitbranch --show-current)
   PR_BASE=$(gh pr view "$CURRENT_BRANCH" --repo brave/brave-core \
     --json baseRefName --jq '.baseRefName' 2>/dev/null) || true
   ```

2. **Check the upstream tracking branch**: If no PR exists, check what the
   branch tracks:

   ```bash
   TRACKING=$(gitrev-parse --abbrev-ref \
     "$CURRENT_BRANCH@{upstream}" 2>/dev/null) || true
   # Strip the remote prefix (e.g., "origin/branch-A" -> "branch-A")
   ```

3. **Fall back to `master`**: If neither method yields a result, use `master`.

### Step L2: Gather Local Changes

```bash
# 1. Get the merge base with the detected base branch
MERGE_BASE=$(gitmerge-base HEAD $BASE_BRANCH)

# 2. Get all committed changes on this branch since diverging from base
gitdiff $MERGE_BASE..HEAD

# 3. Get uncommitted changes (staged + unstaged)
gitdiff HEAD

# 4. Get list of changed files for context
gitdiff --name-only $MERGE_BASE..HEAD
gitdiff --name-only HEAD
```

Combine these diffs to form the complete set of changes to review. The combined
diff represents what would be in a PR against `$BASE_BRANCH` if one were created
right now.

**Report the base branch** at the start of the review so it's clear what the
changes are compared against (e.g., "Reviewing against base branch:
`branch-A`").

### Step L3: Gather Context

1. **Check the branch name** for hints about what the change does:

   ```bash
   gitbranch --show-current
   ```

2. **Check recent commit messages** on this branch for context:

   ```bash
   gitlog $MERGE_BASE..HEAD --oneline
   ```

3. **Read the modified files** in full to understand the surrounding code
   context

Then proceed to the **Common Analysis Steps** below (Step 3 onward), using the
gathered diff instead of a PR diff.

**Note**: For local reviews, skip Steps 1-2 (PR-specific steps) and the
filtering scripts step (Step 4), since there is no PR or GitHub data to filter.

---

## PR Mode

When reviewing a PR, follow Steps 1-3 below, then continue with the Common
Analysis Steps.

### Step 1: Parse PR URL and Gather Context

Extract PR information from the provided URL:

```bash
# Example: https://github.com/brave/brave-core/pull/12345
PR_REPO="brave/brave-core"  # or extract from URL
PR_NUMBER="12345"  # extract from URL
```

Get PR details:

```bash
gh pr view $PR_NUMBER --repo $PR_REPO --json title,body,state,headRefName,author,files
```

---

## Step 2: Research Previous Fix Attempts and Prove Differentiation (PR Mode Only)

**CRITICAL**: Before evaluating the current fix, understand what has been tried
before. If previous attempts exist, the current fix **MUST prove it is
materially different** or the review is an **AUTOMATIC FAIL**.

**Where to search:** Previous fix attempts live as pull requests in the target
repository (typically `brave/brave-core`). Search by issue number AND by test
name/keywords, since not all PRs reference the issue directly:

```bash
# Extract issue number from PR body
ISSUE_NUMBER="<extracted from PR body>"

# Search PRs in the target repo by issue number and test name
gh api search/issues --method GET \
  -f q="repo:brave/brave-core is:pr $ISSUE_NUMBER OR <test-name>" \
  --jq '.items[] | {number, title, state, html_url, user: .user.login}'
```

For each previous attempt found:

```bash
# Get the diff to understand what was tried
gh pr diff <pr-number> --repo brave/brave-core

# Get review comments to understand why it failed/was rejected
gh pr view <pr-number> --repo brave/brave-core --json reviews,comments
```

**Document findings:**

- What approaches were tried before?
- Why did they fail or get rejected?
- Are there patterns in the failures?

### Differentiation Requirement (AUTOMATIC FAIL if not met)

When previous fix attempts exist, you MUST compare the current PR's diff against
each previous attempt's diff and answer:

1. **Is the approach materially different?** Compare the actual code changes,
   not just the PR description. Look at:

   - Are the same files being modified?
   - Are the same lines/functions being changed?
   - Is the same strategy being applied (e.g., both add a wait, both add a null
     check, both reorder operations)?

2. **If the approach IS different, explain HOW:**

   - "Previous PR #1234 added a `RunUntilIdle()` call. This PR instead uses
     `TestFuture` to synchronize on the specific callback."
   - "Previous PR #1234 disabled the test. This PR fixes the underlying race
     condition by adding an observer."

3. **If the approach is the same or substantially similar → AUTOMATIC FAIL:**
   - Same files modified with same type of change
   - Same strategy (e.g., both add timing delays, both add the same kind of
     guard)
   - Same root cause explanation with no new evidence
   - Cosmetically different but functionally identical (e.g., different wait
     duration, different variable name for the same fix)

**The burden of proof is on the current fix.** If you cannot clearly articulate
why this fix is different from previous failed attempts, the review MUST FAIL
with the reason: "Fix is not materially different from previous attempt(s)
#XXXX."

---

## Common Analysis Steps

The following steps apply to both local and PR mode reviews.

---

## Step 3: Fetch Diff and Classify Changed Files

**For PR mode**, fetch the full diff once and save it for subagent use:

```bash
PR_DIFF=$(gh pr diff $PR_NUMBER --repo $PR_REPO)
```

**For local mode**, the diff was already gathered in Step L2. Combine the
committed + uncommitted diffs into `PR_DIFF`.

Extract the file list from the diff:

```bash
echo "$PR_DIFF" | grep '^diff --git' | sed 's|.*b/||'
```

File classification is handled automatically by the discovery script in Step 6.1
— no manual classification needed.

---

## Step 4: Fetch GitHub Data (PR Mode Only)

**For the associated issue (if any):**

```bash
gh issue view $ISSUE_NUMBER --repo brave/brave-browser --json title,body,comments
```

**For PR reviews and comments:**

```bash
gh api repos/$PR_REPO/pulls/$PR_NUMBER/reviews --paginate
gh api repos/$PR_REPO/pulls/$PR_NUMBER/comments --paginate
gh api repos/$PR_REPO/issues/$PR_NUMBER/comments --paginate
```

---

## Step 5: Analyze the Proposed Changes

**Analyze the code in context:**

1. **Read the modified files** from the repo:

   ```bash
   # For each changed file, read the full file to understand context
   # Example: If the diff modifies browser/ai_chat/ai_chat_tab_helper.cc
   # Read: src/brave/browser/ai_chat/ai_chat_tab_helper.cc
   ```

2. **Read related files** to understand the module:

   - Header files (.h) for the modified implementation files
   - Other files in the same directory
   - Test files that exercise the modified code

3. **For chromium_src overrides**, also read the upstream file:
   ```bash
   # If modifying src/brave/chromium_src/chrome/browser/foo.cc
   # Also read ../chrome/browser/foo.cc to understand what's being overridden
   ```

**Questions to answer:**

1. What files are changed?
2. What is the nature of the change?
   - Is it a code fix, test fix, or both?
   - Is it adding a filter/disable (potential workaround)?
3. Does the change match the problem description?
4. Is the change minimal and focused?
5. Does the fix make sense given the surrounding code?

---

## Step 6: Check Against Best Practices (Chunked Subagent Review)

**IMPORTANT:** The main context does NOT load best practices docs directly. Each
review is performed by subagents — one per chunk of ~3 rules — running in
parallel. Large best-practice documents are split into evenly-sized chunks by a
preprocessing script, so each subagent handles a focused set of rules. This
ensures every rule is systematically checked rather than relying on a single
pass to hold many rules in mind.

**ZERO-TOLERANCE RULE: You MUST launch a subagent for EVERY chunk from EVERY
discovered document. No exceptions. No filtering. No "focusing on key areas." No
commentary about the number of chunks. Just launch them all.**

**CRITICAL — NO SHORTCUTS FOR LARGE DIFFS:** Regardless of diff size (even
100KB+), you MUST pass the **complete, untruncated diff** to every subagent and
review ALL changed files. Do NOT skip files, truncate the diff, selectively
review "key chunks", or take any other shortcut based on diff size. The chunked
subagent architecture is specifically designed to handle large diffs — each
subagent only checks ~3 rules, so the diff size is not a constraint. If the diff
is large, that means MORE subagents are needed, not fewer.

**CRITICAL — LAUNCH ALL DISCOVERED DOCS:** You MUST launch subagents for ALL
documents returned by the discovery script — not just the ones you consider
"most relevant." The discovery script already filters out inapplicable
categories (e.g., iOS docs when no iOS files changed). If a document appears in
the discovery output, it MUST get a subagent. Do NOT second-guess the discovery
script or selectively skip documents. The only filtering happens in the script;
you do not apply additional filtering.

**CRITICAL — NO COMMENTARY ABOUT CHUNK COUNT:** Do NOT output any text
commenting on the number of chunks, expressing concern about the volume of work,
or announcing that you will "focus on key areas" or "launch focused subagents
for the most relevant areas." Just launch ALL subagents silently. The number of
chunks is irrelevant — launch them all in parallel without editorializing. Any
message like "Given the large number of chunks (N total), I'll focus on..." is
WRONG and violates this skill's requirements. You must launch every single chunk
subagent regardless of how many there are.

### Step 6.1: Discover Applicable Docs, Chunk, and Launch Subagents

For each applicable best-practice document, run the chunking script to split it
into groups of ~3 rules, then launch one subagent per chunk. **Use multiple
Agent tool calls in a single message** so they run in parallel. Pass the
`PR_DIFF` content (fetched in Step 3) directly in each subagent's prompt so they
don't need to fetch it again.

**Discovery step:** First, determine which best-practice docs apply to this
change. The discovery script auto-detects applicable documents based on changed
file types — no hardcoded document list needed:

```bash
# Extract changed file paths from the diff
CHANGED_FILES=$(echo "$PR_DIFF" | grep '^diff --git' | sed 's|.*b/||')

# Discover applicable docs (pass changed files via stdin)
echo "$CHANGED_FILES" | python3 ./.claude/skills/review/discover-bp-docs.py \
  ./docs/best-practices/ --changed-files-stdin
```

The script outputs JSON with each applicable doc's path and category. Documents
are matched to file types by naming convention (e.g., `testing-*.md` applies
when test files are changed, `android.md` when `.java`/`.kt` files are changed).
Documents that don't match any specific category (like `architecture.md`,
`documentation.md`) are always included.

**Chunking step:** For each discovered document, run:

```bash
python3 ./.claude/skills/review/chunk-best-practices.py <doc_path>
```

This outputs JSON with one or more chunks per document. Each chunk contains:

- `doc`: the source document filename
- `chunk_index` / `total_chunks`: position within the document
- `rule_count`: number of `##` rules in this chunk
- `headings`: list of rule heading texts (for the audit trail)
- `content`: the full text to pass to the subagent (includes the doc header +
  the chunk's rules)

Small documents (<=5 rules) produce 1 chunk. Large documents are split evenly
(e.g., 25 rules → 5 chunks of 5). Launch one subagent per chunk.

### Step 6.2: Subagent Prompt

Each subagent prompt MUST include:

1. **The chunk content** — embed the `content` field from the chunking script
   output directly in the prompt. The subagent does NOT read any files — all
   rules are provided inline. Include it like:
   ````
   Here are the best practice rules to check:
   ```markdown
   <chunk content>
   ```
   ````
2. **The diff content** — include the **complete, untruncated** diff text
   directly in the prompt. Never omit, summarize, or truncate any portion of the
   diff regardless of its size. The subagent MUST NOT call `gh pr diff` or
   `git diff` — the diff is already provided. Embed it in the prompt like:
   ````
   Here is the diff to review:
   ```diff
   <PR_DIFF content>
   ```
   ````
3. **The review rules** (copied into the subagent prompt):
   - Only flag violations in ADDED lines (+ lines), not existing code
   - If you notice a violation in surrounding context lines (lines without
     `+` prefix) or in unchanged code visible in the diff, do NOT comment
     on it or suggest fixing it -- unless the changes directly affect or
     break that surrounding code
   - Also flag bugs introduced by the change (e.g., missing string separators,
     duplicate DEPS entries, code inside wrong `#if` guard)
   - **Check surrounding context before making claims.** When a violation
     involves dependencies, includes, or patterns, read the full file context
     (e.g., the BUILD.gn deps list, existing includes in the file) to verify
     your claim is accurate. Do NOT claim a PR "adds a dependency" or
     "introduces a pattern" if it already existed before the PR.
   - **Only comment on things the author introduced.** If a dependency, pattern,
     or architectural issue already existed before this PR, do not flag it —
     even if it violates a best practice. The author is not responsible for
     pre-existing issues. Focus exclusively on what the changes add or modify.
   - **Do not suggest renaming imported symbols defined outside the change.**
     When a `+` line imports or calls a function/class/variable from another
     module, and that symbol's definition is NOT in a file changed by the diff,
     do not comment on the symbol's naming. Only flag naming issues on symbols
     that are defined or renamed within the changed files.
   - Security-sensitive areas (wallet, crypto, sync, credentials) deserve extra
     scrutiny — type mismatches, truncation, and correctness issues should use
     stronger language
   - Do NOT flag: existing code not being changed, template functions defined in
     headers, simple inline getters in headers, style preferences not in the
     documented best practices, **include/import ordering** (this is handled by
     formatting tools and linters)
   - **Every claim must be verified in the best practices source document.** Do
     NOT make claims based on general knowledge or assumptions about what
     "should" be a best practice. If the best practices docs do not contain a
     rule about something, do NOT flag it as a violation — even if you believe
     it to be true. Hallucinated rules erode trust and waste developer time.
     When in doubt, do not comment.
   - Comment style: short (1-3 sentences), targeted, acknowledge context. Use
     "nit:" for genuinely minor/stylistic issues. Substantive issues (test
     reliability, correctness, banned APIs) should be direct without "nit:"
     prefix
4. **Best practice link requirement** — each rule in the best practices docs has
   a stable ID anchor (e.g., `<a id="CS-001"></a>`) on the line before the
   heading. For each violation, the subagent MUST include a direct link using
   that ID. The link format is:
   ```
   https://github.com/brave/brave-core/tree/master/docs/best-practices/<doc>.md#<ID>
   ```
   **CRITICAL: The `rule_link` fragment MUST be an exact `<a id="...">` value
   from the rules provided in the chunk.** Do NOT invent IDs, guess ID numbers,
   or construct anchors from heading text. If no `<a id>` tag exists for the
   rule, omit the `rule_link` field entirely.
5. **The systematic audit requirement** (Step 6.3 below)
6. **Required output format** (Step 6.4 below)

### Step 6.3: Systematic Audit Requirement

**CRITICAL — this is what prevents the subagent from stopping after finding a
few violations.**

The subagent MUST work through its chunk **heading by heading**, checking every
`##` rule against the diff. It must output an audit trail listing EVERY `##`
heading in the chunk with a verdict:

```
AUDIT:
PASS: Always Include What You Use (IWYU)
PASS: Use Positive Form for Booleans and Methods
N/A: Consistent Naming Across Layers
FAIL: Don't Use rapidjson
PASS: Use CHECK for Impossible Conditions
... (one entry per ## heading in the chunk)
```

Verdicts:

- **PASS**: Checked the diff — no violation found
- **N/A**: Rule doesn't apply to the types of changes in this diff
- **FAIL**: Violation found — must have a corresponding entry in VIOLATIONS

This forces the model to explicitly consider every rule rather than satisficing
after a few findings.

### Step 6.4: Required Subagent Output Format

Each subagent MUST return this structured format:

```
DOCUMENT: <document name> (chunk <chunk_index+1>/<total_chunks>)

AUDIT:
PASS: <rule heading>
N/A: <rule heading>
FAIL: <rule heading>
... (one line per ## heading in the chunk)

VIOLATIONS:
- file: <path>, line: <line_number>, severity: <"high"|"medium"|"low">, rule: "<rule heading>", rule_link: <full GitHub URL to the rule heading>, issue: <brief description>, draft_comment: <1-3 sentence comment>
- ...
NO_VIOLATIONS (if none found)

Severity guide:
- **high**: Correctness bugs, use-after-free, security issues, banned APIs, test reliability problems (e.g., RunUntilIdle)
- **medium**: Substantive best practice violations (wrong container type, missing error handling, architectural issues)
- **low**: Nits, style preferences, missing docs, naming suggestions, minor cleanup
```

### Step 6.5: Aggregate and Validate Results

After ALL chunk subagents return:

1. **Aggregate violations** from all chunk subagents into a single list. Sort by
   severity: high → medium → low.

2. **Validate rule links** — for each violation with a `rule_link`, extract the
   fragment ID and validate it exists in the target doc:

   ```bash
   python3 ./script/manage-bp-ids.py --check-link <ID> --doc <doc>.md
   ```

   If the ID is invalid, strip the link from the comment text. Violations
   missing `rule_link` that are not genuine bug/correctness/security findings
   should be dropped.

3. **Deep-dive validation** — before including in the report, validate every
   remaining violation by reading the actual source code:

   - **Read the actual source file** at and around the flagged line using the
     Read tool (not the diff) to see the full file context
   - **Verify the claim is true.** If the violation says "this should use X
     instead of Y", confirm X is actually available, appropriate, and consistent
     with the rest of the file/module
   - **Deprecation claims require header verification.** Read the actual header
     file to confirm deprecation. Do NOT rely on training data
   - **Check surrounding context for justification.** Look for comments, TODOs,
     or patterns that explain the code
   - **Drop false positives.** If reading the source reveals the violation is
     incorrect, drop it

4. **Present violations one at a time and offer to fix each.** After validation,
   walk through the remaining violations sequentially. For each violation,
   present it to the user and ask whether they want it fixed:

   ```
   **Violation 1/N** (severity: high)
   **File**: path/to/file.cc:42
   **Rule**: <rule heading>
   **Issue**: <description>
   **Suggested fix**: <what the fix would look like>

   Fix this violation? (yes/no/skip)
   ```

   - **yes**: Apply the fix immediately using the Edit tool. Read the file first
     if not already loaded, make the targeted change, then confirm what was
     changed before moving to the next violation.
   - **no** or **skip**: Leave the code as-is and move to the next violation.
   - If the user says "fix all" or "yes to all", apply all remaining violations
     without further prompting.
   - If the user says "stop" or "no to all", skip all remaining violations and
     proceed to the report.

   **Fix guidelines:**

   - Fixes must be minimal and targeted — only change what the violation
     requires
   - Do NOT refactor surrounding code or make "while you're here" improvements
   - If a fix is ambiguous or could be done multiple ways, explain the options
     and ask the user which approach they prefer before editing
   - If a violation cannot be auto-fixed (e.g., requires architectural redesign
     or new tests), say so and move on

5. **Include all violations** (fixed and unfixed) in the review report under the
   Best Practices section. Mark each as **(fixed)** or **(unfixed)** so the user
   knows what remains.

---

## Step 7: Validate Root Cause Analysis

**Read the PR body and any issue analysis carefully.**

Check for **RED FLAGS** indicating insufficient root cause analysis:

### Vague/Uncertain Language (FAIL if unexplained)

- "should" - e.g., "This should fix the issue"
- "might" - e.g., "This might be causing the problem"
- "possibly" - e.g., "This is possibly a race condition"
- "probably" - e.g., "The test probably fails because..."
- "seems" - e.g., "It seems like the timing is off"
- "appears" - e.g., "The issue appears to be..."
- "could be" - e.g., "This could be the root cause"
- "may" - e.g., "The callback may not be completing"

**These words are acceptable ONLY if followed by concrete investigation:**

- BAD: "This should fix the race condition"
- GOOD: "The race condition occurs because X happens before Y. Adding a wait for
  signal Z ensures proper ordering."

### Questions to Ask

1. **Can you explain WHY the test fails?** (Not just symptoms, but cause)
2. **Can you explain HOW the fix addresses the root cause?** (Mechanism, not
   hope)
3. **Is there a clear causal chain?** (A causes B, fix C breaks the chain)
4. **Why does this fail in Brave specifically?** (What Brave-specific factors
   contribute - e.g., different UI elements, additional features, different
   timing characteristics, etc.)

### AI Slop Detection

Watch for generic explanations that could apply to any bug:

- "Improved error handling"
- "Fixed timing issues"
- "Better synchronization"
- "Enhanced stability"

**Demand specifics:**

- WHAT timing issue? Between which operations?
- WHAT synchronization was missing? What signal is now used?
- WHERE was the race condition? What two things were racing?

### Brave-Specific Context Required

If a test fails in Brave but passes in Chrome (or is flaky in Brave but stable
in Chrome), the root cause analysis MUST explain what Brave-specific factors
contribute:

- Different UI elements (Brave Shields, sidebar, wallet button, etc.)
- Additional toolbar items or browser chrome that affects layout/sizing
- Brave-specific features that change timing or execution order
- Different default settings or feature flags
- Additional observers or hooks that Brave adds

Without this explanation, the analysis is incomplete even if the general
mechanism is understood.

---

## Step 8: Additional Best Practices Checks (Non-Chunked)

These checks are performed directly by the main context (not subagents) because
they require PR-level reasoning rather than per-rule checking:

### Timing-Based "Fixes" (AUTOMATIC FAIL)

If the fix works by altering execution timing rather than adding proper
synchronization:

**BANNED patterns:**

- Adding sleep/delay calls
- Adding logging that changes timing
- Reordering code without synchronization explanation
- Adding `RunUntilIdle()` (explicitly forbidden by Chromium)
- Adding arbitrary waits without condition checks

**ACCEPTABLE patterns:**

- `base::test::RunUntil()` with a proper condition
- `TestFuture` for callback synchronization
- Observer patterns with explicit quit conditions
- MutationObserver for DOM changes (event-driven)

### Nested Run Loop Issues (AUTOMATIC FAIL for macOS)

- `EvalJs()` or `ExecJs()` inside `RunUntil()` lambdas
- This causes DCHECK failures on macOS arm64

### Test Disables

If the fix is disabling a test:

- Is there thorough documentation of why?
- Were other approaches tried first?
- Is this a Chromium test (upstream) or Brave test?
- If Chromium test, is it also disabled upstream?

**CRITICAL: Use the most specific filter file possible.**

Filter files follow the pattern: `{test_suite}-{platform}-{variant}.filter`

Available specificity levels (prefer most specific):

1. `browser_tests-windows-asan.filter` - Platform + sanitizer specific (MOST
   SPECIFIC)
2. `browser_tests-windows.filter` - Platform specific
3. `browser_tests.filter` - All platforms (LEAST SPECIFIC - avoid if possible)

**Before accepting a test disable, verify:**

1. **Which CI jobs reported the failure?** Check issue labels (bot/platform/_,
   bot/arch/_) and CI job names
2. **Is the root cause platform-specific?** (e.g., Windows-only APIs,
   macOS-specific behavior)
3. **Is the root cause build-type-specific?** (e.g., ASAN/MSAN/UBSAN, OFFICIAL
   vs non-OFFICIAL)
4. **Does a more specific filter file exist or should one be created?**

**Examples:**

- Test fails only on Windows ASAN → use `browser_tests-windows-asan.filter`
- Test fails only on Linux → use `browser_tests-linux.filter`
- Test fails on all platforms due to Brave-specific code → use
  `browser_tests.filter`

5. **Is the test flaky upstream? (Chromium tests only)** — This check only
   applies to upstream Chromium tests (defined in `src/` but NOT in
   `src/brave/`). Brave-specific tests won't appear in the Chromium database.
   For Chromium tests, check the LUCI Analysis database:

   ```bash
   python3 ./script/check-upstream-flake.py "<TestName>"
   ```

   The script queries `analysis.api.luci.app` and returns a verdict:

   - **Known upstream flake** (>=5% flake rate): Supports the disable. Verify
     the PR comment mentions upstream flakiness.
   - **Occasional upstream failures** (1-5%): Weakly supports the disable. PR
     should document this.
   - **Stable upstream** (<1%): The test is reliable in Chromium — the root
     cause is likely Brave-specific. The PR must explain what Brave-specific
     factors cause the failure. A disable without this explanation is a **red
     flag**.
   - **Not found / Insufficient data**: Cannot determine from upstream data.
     Manual analysis needed.

   Use `--days 60` for a wider lookback window if the default 30 days has
   insufficient data.

**Red flags (overly broad disables):**

- Adding to `browser_tests.filter` when failure is only reported on one platform
- Adding to general filter when failure is only on sanitizer builds
  (ASAN/MSAN/UBSAN)
- No investigation of which CI configurations actually fail

### Intermittent/Flaky Test Analysis

For flaky tests, the root cause analysis must explain **why the failure is
intermittent** - not just why it fails, but why it doesn't fail every time:

**Questions to answer:**

- What variable condition causes the test to sometimes pass and sometimes fail?
- Is it timing-dependent? (e.g., race between two async operations)
- Is it resource-dependent? (e.g., system load, memory pressure)
- Is it order-dependent? (e.g., test isolation issues, shared state)
- Is it platform-specific? (e.g., only flaky on certain OS/architecture)

**Examples of good intermittency explanations:**

- "The test is flaky because the viewport resize animation may or may not
  complete before the screenshot is captured, depending on system load"
- "The race window is small (~15ms) so the test only fails when thread
  scheduling happens to interleave the operations in a specific order"
- "On slower CI machines, the async callback completes before the size check; on
  faster machines, it doesn't"

**Red flags (incomplete analysis):**

- "The test is flaky" (without explaining the variable condition)
- "Sometimes passes, sometimes fails" (just restating the symptom)
- "Timing-dependent" (without explaining what timing varies)

---

## Step 9: Assess Fix Confidence

Rate confidence level:

### HIGH Confidence (likely to work)

- Clear root cause identified and explained
- Fix directly addresses the root cause
- Change is minimal and focused
- Similar patterns exist in codebase
- Tests verify the fix

### MEDIUM Confidence (may work, needs verification)

- Root cause identified but explanation has minor gaps
- Fix seems reasonable but relies on assumptions
- Could benefit from additional tests

### LOW Confidence (likely to fail or regress)

- Root cause not clearly identified
- Fix is a workaround, not a solution
- Uses timing-based approaches
- Overly complex for the problem
- Changes unrelated code
- Fix is not materially different from a previous failed attempt

---

## Step 10: Generate Review Report

**CRITICAL: Avoid Redundancy**

- Each piece of information should appear ONCE in the report
- Do NOT repeat the same issue in multiple sections
- The verdict reasoning should be a brief reference, not a restatement of
  everything above

**CRITICAL: Fill Informational Gaps Yourself**

- If the PR is missing context that you CAN research (e.g., "why does this flake
  in Brave but not upstream?"), DO THE RESEARCH and provide the answer in your
  analysis
- Only list something as an "issue requiring iteration" if it requires action
  from the PR author that you cannot provide
- The confidence level should reflect the state AFTER you've provided any
  missing context - if you filled the gaps, confidence should be higher

**CRITICAL: No Vague Language in YOUR Analysis**

- The same vague language rules (Step 7) apply to YOUR review output, not just
  the PR's analysis
- If you write "appears to", "seems to", "might be", etc. in your analysis, you
  have NOT completed the review
- You must either:
  1. **Investigate further** until you can make a definitive statement, OR
  2. **Flag it as requiring investigation** in the "Issues Requiring Author
     Action" section
- Example of what NOT to do: "The channel detection appears to return STABLE" -
  this is incomplete
- Example of what TO do: Either trace the exact code path to confirm what value
  is returned, OR list "Determine exact channel value returned in CI
  environment" as an issue requiring investigation

### PR Mode Report Format

```markdown
# PR Review: #<number> - <title>

## Summary

<2-3 sentences: what this PR does, the root cause, and whether the fix is
appropriate>

## Context

- **Issue**: #<number or "N/A">
- **Previous attempts**: <Brief list or "None found">
- **Differentiation**: <How this fix differs from previous attempts, or "N/A -
  no previous attempts">

## Analysis

### Root Cause

<Summarize the PR's explanation. If incomplete, research and provide the missing
context yourself rather than flagging it as an issue.>

### Brave-Specific Factors (if applicable)

<If this fails in Brave but not upstream, research and explain why. Provide this
context yourself.>

### Fix Evaluation

<Does the fix address the root cause? Any best practices violations?>

### Best Practices (Chunked Subagent Results)

<Summarize findings from the chunked best practices review. List any validated
violations with file, line, severity, and the specific rule violated. Include
rule links where available.>

## Issues Requiring Author Action

<ONLY list issues that genuinely require the PR author to take action. Do NOT
include:

- Informational gaps you filled in the Analysis section
- Context you researched and provided above
- Minor suggestions>

If no issues: "None - PR is ready for review."

## Verdict: PASS / FAIL (assessed AFTER accounting for any context you provided above)

**Confidence**: HIGH / MEDIUM / LOW

<1-2 sentence reasoning>
```

### Local Mode Report Format

```markdown
# Local Review: <branch-name>

## Summary

<2-3 sentences: what these changes do and whether the approach is sound>

## Changes Overview

- **Branch**: <branch-name>
- **Base branch**: <base-branch> (how it was detected: PR / tracking / default
  master)
- **Files changed**: <count>
- **Commits on branch**: <count> (+ uncommitted changes if any)

## Analysis

### Change Evaluation

<What do the changes accomplish? Is the approach correct? Any logic errors?>

### Best Practices (Chunked Subagent Results)

<Summarize findings from the chunked best practices review. List any validated
violations with file, line, severity, and the specific rule violated. Include
rule links where available.>

## Issues Found

<List significant issues that should be addressed before creating a PR. Do NOT
include:

- Style preferences
- Minor naming suggestions
- Optional refactoring ideas>

If no issues: "None - changes look ready for PR."

## Verdict: PASS / FAIL

**Confidence**: HIGH / MEDIUM / LOW

<1-2 sentence reasoning>
```

---

## Important Guidelines

### Only Report Significant Issues

**DO report:**

- Logic errors or bugs in the fix
- Missing synchronization or race conditions
- Violations of documented best practices
- Incomplete root cause analysis
- High-risk changes without adequate testing
- Potential regressions

**DO NOT report:**

- Style preferences
- Minor naming suggestions
- Optional refactoring ideas
- "While you're here..." improvements
- Anything that doesn't warrant a round-trip iteration

### Be Specific and Actionable

- BAD: "The root cause analysis is weak"
- GOOD: "The PR says 'This should fix the timing issue' but doesn't explain what
  timing issue exists or why this change fixes it. Specifically, what two
  operations are racing and how does the new wait prevent that race?"

### Read the Source Code

- **Always read the actual files** from the repo to understand context
- Don't just look at the diff in isolation
- Check related files, headers, and tests
- For chromium_src changes, also read the upstream Chromium file
- **Before each review comment, verify your claims by reading the relevant
  source code in `src/brave/` and `src/`.** Do not make assertions about APIs,
  patterns, deprecations, or behavior without first confirming them in the
  actual codebase. Look at how the API/pattern is used elsewhere, check header
  files for documentation, and read upstream Chromium code when relevant. Every
  comment you make should be grounded in what the code actually says, not
  assumptions.

### Local by Default

- **DO NOT** post comments, approve, or request changes on GitHub **unless the
  user explicitly asks**
- **DO NOT** merge or close the PR
- This is an analysis tool for the reviewer's eyes only

### Posting to GitHub

If the user asks you to post the review as a comment on GitHub, **always prefix
the review body** with:

I generated this review about the changes, sharing here. It should be used for
informational purposes only and not as proof of review.

This disclaimer must appear at the very beginning of the review body (as plain
text, not as a blockquote).

**Post as inline code comments when possible.** When the review identifies
specific issues tied to files and lines, post them as inline review comments on
the actual code rather than as a single general comment. Use the GitHub review
API to submit a single review with:

- **Review body**: The summary, verdict, and any general observations (with the
  disclaimer prefix above)
- **Inline comments**: Each specific issue placed on its file and line

```bash
gh api repos/brave/brave-core/pulls/{number}/reviews \
  --method POST \
  --input - <<'EOF'
{
  "event": "COMMENT",
  "body": "I generated this review about the changes, sharing here. It should be used for informational purposes only and not as proof of review.\n\n## Summary\n...\n\n## Verdict: PASS/FAIL\n...",
  "comments": [
    {
      "path": "path/to/file.cc",
      "line": 42,
      "side": "RIGHT",
      "body": "specific issue description for this line"
    }
  ]
}
EOF
```

**Key details:**

- `side: "RIGHT"` targets the new version of the file (changed lines)
- `line` is the line number in the new file
- All inline comments are batched into one review (one notification to the
  author)
- If an issue can't be tied to a specific line in the diff, include it in the
  review body instead
- If the inline API call fails for a comment (line outside diff range), fall
  back to including that issue in the review body

---

## Example Usage

Review local changes (default - no argument needed):

```
/review
/review local
```

Review a specific PR by URL:

```
/review https://github.com/brave/brave-core/pull/12345
```

Review a PR by number (assumes brave/brave-core):

```
/review 12345
```

---

## Checklist Before Completing Review

### Both Modes

- [ ] Confirmed running from brave-core directory
- [ ] Analyzed the diff (local changes or PR diff)
- [ ] Ran discovery script to find applicable best practices documents
- [ ] Ran chunking script on each discovered document
- [ ] Launched parallel subagents for all chunks
- [ ] Each subagent produced a full AUDIT trail (one verdict per ## heading)
- [ ] Aggregated and validated all violations from subagents
- [ ] Deep-dive validated each violation by reading actual source files
- [ ] **Read the actual source files in src/brave/** to understand context
- [ ] Validated root cause analysis quality (or assessed code quality for local)
- [ ] Checked timing-based fixes, nested run loops, and test disables (Step 8)
- [ ] Assessed fix confidence level
- [ ] Only reported important issues
- [ ] Provided clear pass/fail verdict with reasoning

### PR Mode Only

- [ ] Parsed PR URL and gathered context
- [ ] Extracted associated GitHub issue (if any)
- [ ] Researched previous fix attempts
- [ ] If previous attempts exist: proved current fix is materially different (or
      FAILED the review)
- [ ] Only posted to GitHub if user explicitly requested (with disclaimer
      prefix)
- [ ] For test disables: checked upstream flakiness via check-upstream-flake.py

### Local Mode Only

- [ ] Detected correct base branch (PR base > tracking branch > master)
- [ ] Gathered uncommitted changes (staged + unstaged)
- [ ] Gathered branch changes since diverging from base branch
- [ ] Checked branch name and commit messages for context
