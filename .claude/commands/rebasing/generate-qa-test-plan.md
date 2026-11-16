---
name: generate-qa-test-plan
description: Generate QA test checklist from current Chromium rebase commits
---

Goal:

- Identify commits related to the current Chromium rebase
- Extract user-visible or behavior-impacting changes
- Convert them into actionable QA test items
- Group results into Desktop, iOS, and Android sections
- Output clean Markdown

Steps:

1. Run commit discovery script

- Execute: python3 tools/cr/find_rebase_commits.py --verbose
- Capture:
  - Commit range (first line)
  - Full verbose commit list (hash, author, subject)
- Verbose subjects are truncated to 80 chars, so re-read the full subjects with
  `git log --format='%h %s' <range>` before judging relevance

2. Parse commits

- Collapse `reassign!<sha>!`, `fixup!` and `squash!` commits first, if present:
  each one names the commit it amends and carries the same subject, so the range
  holds both. Keep one of each pair, or the whole list comes out doubled. A
  branch that is ready to merge usually has none of these left, so expect the
  plain case
- Every commit carries its platform tag by convention, so the tags drive step 3.
  As a cheap sanity check, list the untagged commits that still touch
  `android/`, `ios/` or `*.java` and confirm each is mechanical — a patch
  refresh, a strings import, a `DEPS` fix
- Ignore:
  - Pure refactors
  - Formatting changes
  - Build, GN dep and link fixes
  - Test-only changes, unless the upstream CL they follow changes behavior (see
    step 4) — a test that only compiles differently is not a QA item
- Prioritize:
  - UI changes
  - Networking / privacy / security
  - Permissions (Bluetooth, camera, etc.)
  - Platform-specific code paths (ios/, android/, chrome/, browser/)
  - Feature flags and toggles
  - Crash fixes
  - Performance changes

3. Infer platform relevance

- Desktop:
  - Default bucket for most changes
  - Anything in browser/, chrome/, components/, ui/
- iOS:
  - Commit mentions ios or has an "[ios]" tag (with our without brackets)
  - Paths containing ios/
  - Mentions of WKWebView, UIKit, iOS APIs
- Android:
  - Commit mentions Android or has an "[Android]" tag (with or without brackets)
  - Paths containing android/
  - Mentions of JNI, Java, Android permissions, SDK
- If unclear:
  - Include in Desktop
  - Optionally duplicate in all platforms if clearly cross-platform

4. Convert commits to QA test items

For each important/relevant commit:

- Read what the commit actually did before writing anything. The subject names
  the symbol that broke, never the UI path, and never why a user would notice:
  - `git show --stat <hash>` for the touched files, which name the surface
  - `git show <hash>` body — read it in full, both halves. Brave's own message
    says what the adaptation keeps or drops; the quoted upstream CL under
    `Chromium changes:` says what behavior changed and why
- The upstream CL is often the only thing with QA value. A Brave commit that
  just follows a rename or fixes a test can sit on top of a real behavior
  change, so judge the item on the CL, not on the size of Brave's diff:
  - A test-only Brave commit still earns an item when the CL changes a shipping
    Brave surface — test the surface the CL changed, not Brave's test
  - A Brave commit whose message says the fix restores something that had
    silently stopped working earns a high-priority item, since the behavior is
    new in this build even though no feature was added
  - Skip the commit entirely when the CL is also mechanical
- An upstream flag removed after launch needs no QA item if Brave never overrode
  it. Decide this from Brave's own override lists rather than from upstream
  history, which would take an investigation to read:
  `base/compile_overridden_features.inc`, `app/feature_defaults_unittest.cc`,
  `BraveCachedFlag.java`, and any `rewrite/` yaml naming the feature. A flag
  that appears only in a Brave test's `@EnableFeatures` was never overridden, so
  the removal cannot change what Brave ships. When Brave did pin it, the item is
  whatever the pin was holding off
- Follow the code when the entry point is still unclear: the string id in a
  `.grd`/`strings.xml`, the pref name, the `BASE_FEATURE` name, the
  `brave://flags` entry
- A Brave class with no caller under `brave/` is usually wired from the parent
  Chromium tree, not dead: `rewrite/**.yaml` and `patches/*.patch` inject the
  call into a Chromium source. Search `..` for the call site, excluding
  `../out`, and check `rewrite/` for a yaml named after the Chromium file before
  concluding the surface is unreachable
- Rewrite subject into a test instruction:
  - Start with a verb: "Verify", "Test", "Ensure"
- Focus on observable behavior, not implementation
- Expand slightly if needed for clarity

Then add steps to reproduce in a collapsed `<details>` block, but only when you
can name the real entry point. Guessed steps cost more than no steps: this
checklist is walked by hand, step by step, before it reaches QA.

Steps quality bar:

- 2 to 5 numbered actions, imperative, one observable action each
- Step 1 is the exact entry point: `Settings -> Appearance`, the overflow menu
  item, `brave://flags/#brave-foo`, a `--enable-features=` launch flag
- Add a `**Precondition:**` bullet whenever the item needs non-default state — a
  flag flipped, a fresh profile, a signed-in sync chain, tablet or foldable form
  factor, a specific ABI
- Last bullet is `**Expected:**` with the observable result
- Do not restate the summary as step 1
- Steps are what a tester does on the device and sees on screen. Keep host-side
  tooling out of them — no `adb logcat`, no `adb shell`, no database dump, no
  proxy — and state the expectation as the visible symptom: a crash, a hang, a
  blank screen, a wrong value. Verifying the mechanism is not the same as
  verifying the behavior; a malformed log call throws where it is called, so
  "the screen still works, with no crash" already covers it
- The exception is a defect with no on-screen symptom and a serious consequence
  — data sent that should not be, storage that should have been cleared, a field
  committed in plaintext. There the log or network check is the only possible
  test: name the exact string to look for, and say why the UI cannot show it
- Only use URLs that appear in the commit, in repo docs, or that are well-known
  test pages; prefer a local or `brave://` surface over an external site, which
  can disappear or change behavior
- Emit the one-line item with no `<details>` only when you cannot name the entry
  point, not as a way to keep a commit that step 2 says to skip

Markdown shape — the test instruction goes inside `<summary>`, and an item with
steps carries no `-` bullet. The collapsed row already reads as a list item,
with the disclosure triangle in place of the bullet:

```markdown
<details><summary>Verify WebBluetooth permission prompt appears when a site
requests access</summary>

- **Precondition:** Bluetooth enabled on the host.
- **Steps:**
  1. Open a page that requests Bluetooth, e.g. `https://permission.site`
  2. Activate the Bluetooth request
  3. Pick a device in the chooser
- **Expected:** the permission prompt appears and the choice is remembered for
  the site.

</details>

- Verify the shields panel still opens on sites with no filter list match
```

Rules that shape depends on:

- Never put the instruction in the item line and `Steps` in the summary. That
  repeats nothing useful, costs a row of vertical space per item, and a `-`
  bullet with an HTML block nested under it loses its bullet — GitHub stops
  parsing the line as a list item and prints the `-`
- The blank line after `</summary>`, and the one before `</details>`, are
  required, or the numbered list renders as plain text
- Blank line between consecutive `<details>` blocks
- The summary text may wrap across lines; HTML collapses the newline
- Items with no steps keep their `-` bullet, since nothing is nested under them
- Precondition, Steps and Expected are three bullets of one list, with the
  numbered steps nested under `**Steps:**`, so the whole body reads as one
  grouped unit
- GitHub styles a nested `<ol>` `lower-roman`, so the steps render `i. ii. iii.`
  even though the source says `1. 2. 3.` — that is wanted here, since it marks
  the steps as a sub-level. Do not try to force decimals: `<ol type="1">` is a
  presentational hint and loses to GitHub's author rule
- Do not promote the three to top-level siblings to get decimals back. The
  markers do line up that way, but the item loses its grouping and the uneven
  blank lines between the blocks read worse
- A bare paragraph cannot be lined up with the steps at all: GitHub strips
  `style`, so there is no way to indent one to the list's text column
- Drop the Precondition bullet when the item needs no setup

5. Deduplicate and merge

- Combine similar commits into a single QA item
- Keep wording concise
- Merged items get one `<details>` block covering the shared surface, not one
  per commit

6. Output format (Markdown)

Output this structure:

## Rebase QA Checklist

**Commit Range:** <range>

### Desktop

- ...
- ...

### iOS

- ...
- ...

### Android

- ...
- ...

Each item is either a `<details>` block from step 4 or, when it has no steps, a
bare `- ...` line. Omit a platform section that has no items.

7. Constraints

- Do not include commit hashes in final output
- Do not include authors
- Do not include internal jargon
- Keep the instruction to one sentence; the detail goes inside the `<details>`
  block
- Prefer clarity over completeness

8. Optional enhancement

- If no obvious QA impact: Output:
  - "No significant user-facing changes detected"
