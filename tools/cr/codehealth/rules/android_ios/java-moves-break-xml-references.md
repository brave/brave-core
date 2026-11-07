---
name: android_ios/java-moves-break-xml-references
rubric: android_ios
severity: regression
---

**Rule.** Brave's XML references to upstream Java classes, and its imports of
them, follow the classes upstream moved or renamed in the range.

**Why.** `android:fragment=` attributes and custom view tags name classes as
plain strings that javac, `gn check` and R8 all ignore, so nothing fails until
the user opens the screen. Brave's Java imports break too, which only the
Android build catches.

**How to check.** Each candidate commit moves or removes a Java file. Grep each
old package and class in Brave's XML, e.g.
`git grep -n 'tab_management\.TabArchiveSettingsFragment' -- '*.xml'`, and its
Java. Also grep the bare class name: a same-package Brave test needs no import,
so a qualified grep misses it. `BravePreferenceFragmentNamesTest` covers the
`android:fragment` surface; custom view tags remain unchecked.

## Examples

- `TabArchiveSettingsFragment` moved into `…tab_management.archived_tabs`,
  leaving a dead tab settings row.
- M158, 8115.1: `GlicButtonDelegate` moved to `glic.toolbar_button`.
- M158, 8127.2: an orphaned `BraveRecentTabsCoordinatorTest` still in `ntp/`.
