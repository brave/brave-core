---
name: android_ios/hide-lists-keyed-by-name
rubric: android_ios
severity: regression
---

**Rule.** Brave subclasses that remove upstream preferences or menu items by key
cover the keys upstream added or moved in the range.

**Why.** A hide list only knows the keys it names: a new row shows up in Brave,
and a key that moves to another screen leaves Brave's handling on the old one.
Brave's `SEARCH_INDEX_DATA_PROVIDER` replacements need the same removals.

**How to check.** For each candidate commit, take the keys it adds, removes or
moves, and grep Brave's Java for the screen's hide list and its search index
provider. Upstream strings that newly show up in Brave's rebranded `.grd` files,
often as mangled Google product names, are a cheap extra signal.

## Examples

- The new "Search AI Mode and connected apps" sync row.
- M157, 8094.1: a `bottom_bar_switch` that duplicates Brave's own toggle.
- M158, 8111.0: `homepage` moved from Main settings to Appearance.
