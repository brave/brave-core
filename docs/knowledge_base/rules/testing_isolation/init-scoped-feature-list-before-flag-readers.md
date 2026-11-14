---
name: brave/init-scoped-feature-list-before-flag-readers
description:
  A fixture initializes its ScopedFeatureList in its constructor or member
  initializer, declared ahead of members that read the flag, not in SetUp().
rubric: testing
severity: blocking
---

**Rule.** A fixture's `base::test::ScopedFeatureList` MUST be initialized before
anything that reads the flag is constructed: in the fixture constructor (where
`GetParam()` is already available), or through the single-feature constructor in
the member initializer, with the `ScopedFeatureList` member declared ahead of
any member that reads the flag while being constructed. Initialize it in
`SetUp()` only when the flag state depends on something that does not exist
until then, and then at the top of `SetUp()`.

**Why.** An object that reads the flag before the override is in place sees the
default, so the test covers a state other than the one it pins. Members are
constructed in declaration order and `SetUp()` runs after the constructor, so
the constructor or member initializer removes ordering as a question.

## Examples

Bad:

```cpp
class MyCacheEnabledTest : public testing::Test {
 protected:
  void SetUp() override {
    // service_ already read kMyCache while being constructed.
    feature_list_.InitAndEnableFeature(features::kMyCache);
  }

  MyService service_;
  base::test::ScopedFeatureList feature_list_;
};
```

Good:

```cpp
class MyCacheEnabledTest : public testing::Test {
 private:
  base::test::ScopedFeatureList feature_list_{features::kMyCache};
  MyService service_;
};
```
