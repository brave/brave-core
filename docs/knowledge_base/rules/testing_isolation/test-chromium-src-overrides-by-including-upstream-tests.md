---
name: brave/test-chromium-src-overrides-by-including-upstream-tests
description:
  A chromium_src override is tested by a chromium_src test file that includes
  the upstream test file and adds Brave tests after it.
rubric: testing
severity: should_fix
---

**Rule.** When a change overrides upstream code through `chromium_src`, test the
override in `chromium_src/<path>_unittest.cc` (or the matching test file) that
first includes the upstream test file, as
`#include <components/foo/foo_service_unittest.cc>`, and then adds the Brave
tests using the upstream fixtures and mocks. Flag a `chromium_src` test file
that copies or replaces upstream tests instead of including them, and an
override whose Brave behaviour has no test of this kind.

**Why.** Including the upstream file runs every upstream test against the
overridden code, which catches regressions the override causes, and the Brave
tests reuse fixtures that already exist rather than rebuilding them.

## Examples

Bad:

```cpp
// chromium_src/components/foo/foo_service_unittest.cc
// A Brave-only fixture: the upstream tests no longer run on the override.
class BraveFooServiceTest : public testing::Test { ... };
```

Good:

```cpp
// chromium_src/components/foo/foo_service_unittest.cc
#include <components/foo/foo_service_unittest.cc>

namespace foo {

TEST_F(FooServiceTest, BraveSpecificBehavior) {
  // Tests the Brave logic the chromium_src override adds.
}

}  // namespace foo
```
