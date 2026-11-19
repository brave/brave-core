---
name: brave/comment-out-xml-in-patches
description:
  An XML patch removes upstream elements by wrapping them in an HTML comment,
  not by deleting their lines.
rubric: change_scope
severity: should_fix
---

**Rule.** To remove upstream elements from an XML file (Android layouts and
resources, `.grd` and `.grdp` files), a hand-written patch adds a `<!--` line
before them and a `-->` line after them. NEVER delete the elements' lines in the
patch.

**Why.** Two added lines are a much smaller diff than the deleted block, and a
smaller patch is easier to review and less likely to conflict on a rebase.

## Examples

Bad:

```diff
-<LinearLayout ...>
-  ...
-</LinearLayout>
```

Good:

```diff
+<!--
 <LinearLayout ...>
   ...
 </LinearLayout>
+-->
```
