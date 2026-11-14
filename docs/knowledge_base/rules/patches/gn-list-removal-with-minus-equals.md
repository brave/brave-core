---
name: brave/gn-list-removal-with-minus-equals
description:
  A GN patch removes an item from a list with an added -= line, never by
  modifying or deleting the line that adds it.
rubric: change_scope
severity: should_fix
---

**Rule.** To drop an item from a list in an upstream `BUILD.gn` or `.gni` file,
a hand-written patch adds a separate `list -= [ "item" ]` line after the
upstream assignment. NEVER remove or edit the upstream line that adds the item.

**Why.** An addition-only patch leaves the upstream line as context, so upstream
edits to the list do not conflict with it.

## Examples

Bad:

```gn
-  public_deps += [ ":chrome_framework_widevine_signature" ]
```

Good:

```gn
+  public_deps -= [ ":chrome_framework_widevine_signature" ]
```
