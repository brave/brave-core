---
name: brave/android-patches-add-new-lines
description:
  A direct patch of an upstream Android file adds entirely new lines rather than
  modifying existing upstream lines.
rubric: change_scope
severity: should_fix
---

**Rule.** A direct `.patch` of an upstream file (as opposed to a `chromium_src`
override) SHOULD only add new lines. Do not edit an existing upstream line in
place when the same effect can be had by adding a line next to it, such as a
call to a Brave hook or an extra statement.

**Why.** A modified upstream line is an anchor the patch needs to stay
unchanged: whenever upstream touches that line, the patch conflicts at the next
Chromium upgrade. Added lines have no upstream anchor of their own, so they
conflict far less often.

## Examples

Bad:

```diff
-        mTitle = getString(R.string.settings_title);
+        mTitle = BraveSettingsUtils.getTitle(this);
```

Good:

```diff
         mTitle = getString(R.string.settings_title);
+        mTitle = BraveSettingsUtils.getTitle(this, mTitle);
```
