---
name: brave/no-change-detector-tests
description:
  'Tests check behaviour across common and edge cases; they never just mirror
  the production logic, calling a method and checking its obvious result, or
  check structure.'
rubric: testing
severity: should_fix
---

**Rule.** Do not add change detector tests: tests whose logic mirrors the
production code, calling a method and checking its obvious effect, or that only
assert structure (a view or button exists). Test behaviour instead, across the
common and edge cases of code that has many possible valid inputs.

**Why.** A change detector test breaks whenever the implementation changes, yet
catches no real bugs: it restates the code instead of checking what it is for.

## Examples

Bad:

```cpp
TEST(ShowButtonOnBrowserActivationTest, ShowButtonOnActivate) {
  ShowButtonOnBrowserActivation listener;
  listener.BrowserDidActivate();          // Just calls the method...
  EXPECT_TRUE(listener.DidShowButton());  // ...and checks the obvious result
}

TEST_F(MyFeatureTest, ButtonExists) {
  EXPECT_NE(nullptr, view->GetButtonByID(kMyButton));
}
```

Good:

```cpp
TEST(Math, CheckIsPrime) {
  EXPECT_TRUE(IsPrime(2));
  EXPECT_TRUE(IsPrime(3));
  EXPECT_FALSE(IsPrime(99));
  EXPECT_FALSE(IsPrime(-2));
}
```
