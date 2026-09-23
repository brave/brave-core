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

- Execute: python3 find_rebase_commits.py --verbose
- Capture:
  - Commit range (first line)
  - Full verbose commit list (hash, author, subject)

2. Parse commits

- Ignore:
  - Pure refactors
  - Formatting changes
  - Test-only changes unless they imply behavior change
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

4. Convert commits to QA test items For each important/relevant commit:

- Rewrite subject into a test instruction:
  - Start with a verb: "Verify", "Test", "Ensure"
- Focus on observable behavior, not implementation
- Expand slightly if needed for clarity

Example:

- "Fix WebBluetooth permission prompt not showing" -> "Verify WebBluetooth
  permission prompt appears when a site requests access"

5. Deduplicate and merge

- Combine similar commits into a single QA item
- Keep wording concise

6. Output format (Markdown)

Output EXACTLY:

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

7. Constraints

- Do not include commit hashes in final output
- Do not include authors
- Do not include internal jargon
- Keep each bullet to one line when possible
- Prefer clarity over completeness

8. Optional enhancement

- If no obvious QA impact: Output:
  - "No significant user-facing changes detected"
