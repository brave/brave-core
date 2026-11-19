---
name: brave/extensible-patches-use-include
description:
  A patch that adds to a list or block adds a single include of a Brave-owned
  file instead, so further items are added in brave-core without changing the
  patch.
rubric: design
severity: should_fix
---

**Rule.** When a hand-written patch adds entries to an upstream list or block
(XML elements in a layout, entries of an array, items of a list), the patch adds
one include of a Brave-owned file that holds the entries instead, such as an XML
`<include layout="@layout/brave_..."/>`, a `#include` of a Brave file, or a GN
`import()` of a Brave `.gni`. NEVER patch the entries in one by one. NEVER apply
this rule to a plaster-generated patch, one whose upstream file has a matching
`rewrite/<path>.yaml`.

**Why.** With an include, Brave adds, removes and changes entries in its own
file and the patch never changes again. Entries patched in directly make the
patch grow with every addition, and each one can conflict on a rebase.

## Examples

Bad:

```diff
// patches/chrome-android-java-res-layout-toolbar_phone.xml.patch
+    <ImageButton android:id="@+id/brave_shields_button" android:layout_width="wrap_content" android:layout_height="match_parent" />
+    <ImageButton android:id="@+id/brave_rewards_button" android:layout_width="wrap_content" android:layout_height="match_parent" />
```

Good:

```diff
+    <include layout="@layout/brave_toolbar" android:layout_height="wrap_content" android:layout_width="match_parent" />
```
