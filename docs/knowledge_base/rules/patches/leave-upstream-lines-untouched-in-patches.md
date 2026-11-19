---
name: brave/leave-upstream-lines-untouched-in-patches
description:
  A hand-written patch puts its additions on separate lines and leaves existing
  upstream lines exactly as they are, rather than modifying them.
rubric: change_scope
severity: should_fix
---

**Rule.** A hand-written patch SHOULD NOT modify an existing upstream line
(remove it and add an edited copy) when the change can be an added line of its
own next to it. Leave the upstream code the patch touches exactly as it is,
character for character. NEVER apply this rule to a plaster-generated patch, one
whose upstream file has a matching `rewrite/<path>.yaml`.

**Why.** A modified line conflicts with any upstream edit to that line, while an
added line next to unchanged upstream code lets the patch apply, or resolve
automatically, after updates.

## Examples

Bad:

```diff
-  if (!profile->IsOffTheRecord()) {
+  if (!profile->IsOffTheRecord() && !profile->IsTor()) {
```

Good:

```diff
+  if (profile->IsTor()) return;
   if (!profile->IsOffTheRecord()) {
```
