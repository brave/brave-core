---
name: brave/patches-only-functional-changes
description:
  A hand-written patch contains only the minimal functional change; it never
  adds comments, blank lines, whitespace changes or any other non-functional
  edit.
rubric: change_scope
severity: blocking
---

**Rule.** A hand-written patch MUST NOT add comments, empty lines,
re-indentation, reformatting or any other change that does not alter behaviour,
and MUST leave the whitespace of upstream lines exactly as it is. Explanations
belong in the `chromium_src/` override or Brave code the patch hooks into.
Commenting out XML elements, as [brave/comment-out-xml-in-patches] asks, is the
patch's functional change and does not count. NEVER apply this rule to a
plaster-generated patch, one whose upstream file has a matching
`rewrite/<path>.yaml`; review the plaster instead.

**Why.** Every line a patch touches is a line that can conflict on a Chromium
rebase, and non-functional lines make the patch larger and harder to review for
no gain.

## Examples

Bad:

```diff
+  // Brave: let Brave decide whether the bubble shows.
+  BRAVE_FOO_BUBBLE_VIEW_SHOW
+
```

Good:

```diff
+  BRAVE_FOO_BUBBLE_VIEW_SHOW
```
