---
name: android_ios/proguard-keep-rules
rubric: android_ios
severity: obsolete
---

**Rule.** Each member of every `-keep class X { … }` block in
`android/java/*.flags` still exists in the class at the new tag.

**Why.** A rule left behind when the rebase drops the reflection it served keeps
nothing, and R8 doesn't complain.

**How to check.** For each candidate commit that removes or re-signs a member,
grep Brave's `.flags` files for its class, and resolve the rule's members at the
new tag.

## Examples

- M158, 8113.1: `UndoBarController.showUndoBar`, after the plaster replaced the
  adapter, plus four rules dead since M115–M150.
