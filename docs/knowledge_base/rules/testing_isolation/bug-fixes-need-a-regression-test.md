---
name: brave/bug-fixes-need-a-regression-test
description:
  A bug-fix change includes a test that reproduces the bug, failing without the
  fix and passing with it.
rubric: testing
severity: should_fix
---

**Rule.** When a change fixes a bug, it adds or changes a test that reproduces
the specific scenario of that bug, so that the test fails without the fix and
passes with it. Flag a bug fix whose diff changes production logic and no test
covers the case it fixes.

**Why.** Without a test of the exact scenario, nothing stops the bug from coming
back in a later change, and nothing shows that the fix addresses the reported
failure rather than a neighbouring one.

## Examples

Bad:

```cpp
// The fix, with no test of an empty title.
if (title.empty()) {
  return default_title_;
}
```

Good:

```cpp
// Fails before the fix: GetDisplayTitle() returned an empty string.
TEST_F(ConversationTest, GetDisplayTitle_UsesDefaultForEmptyTitle) {
  conversation_->SetTitle("");
  EXPECT_EQ(conversation_->GetDisplayTitle(), kDefaultTitle);
}
```
