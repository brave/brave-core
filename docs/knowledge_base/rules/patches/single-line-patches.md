---
name: brave/single-line-patches
description:
  A hand-written patch adds single lines, even past the line-length limit, never
  multiline hunks of new code; plaster-generated patches are exempt.
rubric: change_scope
severity: blocking
---

**Rule.** A hand-written patch MUST NOT add a multiline block of code to an
upstream file. Each change is one added line: a `BRAVE_*` hook whose body lives
in the `chromium_src/` override ([brave/patch-hooks-via-brave-macros]), or a
single statement. Keep that line unwrapped even past the 80-column limit, as
lint does not run on patched files. Move anything longer into a `chromium_src/`
override, NEVER into a new macro that edits upstream code
([brave/no-new-macros-editing-upstream-code]). NEVER apply this rule to a
plaster-generated patch (its upstream file has a matching
`rewrite/<path>.yaml`): those are multiline by design.

**Why.** A one-line addition is the smallest patch that can conflict on a
Chromium rebase and the easiest to resolve when it does. A block of logic in a
patch conflicts with any nearby upstream edit and has to be re-applied by hand.

## Examples

Bad:

```diff
+  if (brave::IsAutoplayBlocked(profile_)) {
+    return CONTENT_SETTING_BLOCK;
+  }
```

Good:

```diff
+  BRAVE_PERMISSION_CONTROLLER_IMPL_METHOD
```
