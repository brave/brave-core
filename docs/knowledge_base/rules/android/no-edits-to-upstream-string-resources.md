---
name: brave/no-edits-to-upstream-string-resources
description:
  Upstream Chromium string resource files (chrome_strings.grd, upstream
  strings.xml) are never modified; Brave strings go in android_brave_strings.grd
  or another Brave-owned file.
rubric: change_scope
severity: blocking
---

**Rule.** NEVER modify an upstream Chromium string resource file
(`chrome_strings.grd`, `android_chrome_strings.grd`, an upstream `strings.xml`
and the like), whether by a patch, a plaster or a copied override. Add
Brave-specific strings to `android_brave_strings.grd` or the appropriate
Brave-owned resource file instead.

**Why.** Upstream string files change in nearly every Chromium version, so
modifications to them create patch conflicts during Chromium upgrades.

## Examples

Bad:

```diff
# patches/chrome-browser-ui-android-strings-android_chrome_strings.grd.patch
+      <message name="IDS_BRAVE_FOO_TITLE" desc="...">Foo</message>
```

Good:

```xml
<!-- browser/ui/android/strings/android_brave_strings.grd -->
<message name="IDS_BRAVE_FOO_TITLE" desc="...">Foo</message>
```
