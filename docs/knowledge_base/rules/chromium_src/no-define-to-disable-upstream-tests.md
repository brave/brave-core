---
name: brave/no-define-to-disable-upstream-tests
description:
  'Upstream tests must never be disabled by a chromium_src #define renaming them
  to DISABLED_; they are filtered in test/filters/, with Brave replacements in
  Brave test targets.'
rubric: testing
severity: blocking
---

**Rule.** NEVER disable an upstream test from a `chromium_src/` override with
`#define TestName DISABLED_TestName` (or any other rename to `DISABLED_`).
Exclude it with a `-Suite.TestName` line in the matching filter file under
`test/filters/`, and put any Brave-specific replacement test in a Brave test
target (`brave_unit_tests`, `brave_browser_tests`,
`brave_components_unittests`).

**Why.** Filter files are where Brave keeps the upstream tests it disables, each
with its reason, so they can be reviewed and re-enabled. A `#define` in an
override hides the disablement in compiled code, renames every use of the name
in the translation unit, and keeps Brave tests inside upstream targets.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/tabs/tab_strip_model_unittest.cc
#define AddTabAtWithGroup DISABLED_AddTabAtWithGroup
#include <chrome/browser/ui/tabs/tab_strip_model_unittest.cc>
#undef AddTabAtWithGroup
```

Good:

```
# test/filters/unit_tests.filter
# Brave inserts tabs into groups differently; covered by
# BraveTabStripModelTest.AddTabAtWithGroup in brave_unit_tests.
-TabStripModelTest.AddTabAtWithGroup
```
