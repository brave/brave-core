---
name: webui/allowlists-in-shared-webui-code
rubric: webui
severity: regression
---

**Rule.** Hard-coded lists of enum values that upstream adds to shared WebUI
infrastructure include the values Brave's patches add to that enum.

**Why.** A list written against upstream's enum leaves Brave's values out, and
code keyed on it throws or skips them.

**How to check.** For each list a candidate commit adds, find the enum it holds
values of, and compare with the values Brave's patches add to it
(`git grep -n '<Enum>' -- patches chromium_src`).

## Examples

- M158, 8113.1: `CATEGORY_DEFAULT_SYNTHETIC_PREFS` lacks Brave's site-settings
  categories, so their default radios throw.
