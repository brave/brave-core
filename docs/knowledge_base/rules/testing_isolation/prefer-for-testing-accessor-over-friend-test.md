---
name: brave/prefer-for-testing-accessor-over-friend-test
description:
  When tests need one private member, the class exposes a *_for_testing()
  accessor instead of a FRIEND_TEST per test.
rubric: design
severity: should_fix
---

**Rule.** When the tests of a class need a single private member, add one
accessor named `<member>_for_testing()` that returns a reference to it, and
leave the class free of `FRIEND_TEST`/`FRIEND_TEST_ALL_PREFIXES` declarations.
Flag a change that adds a further `FRIEND_TEST` (or a friend test class) just to
reach a member such an accessor would expose.

**Why.** Each `FRIEND_TEST` couples the production header to one test name,
grows with every new test, and opens all of the class's internals to it. One
accessor serves every test, and its `_for_testing` suffix marks it test-only,
which Chromium's presubmit checks production code does not call.

## Examples

Bad:

```cpp
class BraveTab {
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameBasic);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameCancel);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, RenameSubmit);
  raw_ptr<views::Textfield> rename_textfield_;
};
```

Good:

```cpp
class BraveTab {
 public:
  views::Textfield& rename_textfield_for_testing() {
    return *rename_textfield_;
  }

 private:
  raw_ptr<views::Textfield> rename_textfield_;
};
```
