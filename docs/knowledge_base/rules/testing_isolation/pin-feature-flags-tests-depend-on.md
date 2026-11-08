---
name: brave/pin-feature-flags-tests-depend-on
description:
  A test never relies on a base::Feature's compiled-in default; it pins every
  flag it depends on or is parameterized over both states, including when a
  change flips the default.
rubric: testing
severity: blocking
---

**Rule.** Every `base::Feature` whose state a test's expectations depend on is
pinned by the test (`ScopedFeatureList`), or the test is parameterized over both
states and asserts only what differs between them. Where parameterizing is
impractical (a `TYPED_TEST` fixture), pin the flag explicitly. When a change
flips a default (`BASE_FEATURE`, `OVERRIDE_FEATURE_DEFAULT_STATES`, a plaster
`set_feature_flag_default_state`), it MUST NOT:

- delete a fixture's feature setup because the new default makes it redundant;
- add a bespoke helper that absorbs the new default's behaviour so the old
  expectations still hold.

Reuse the test file's parameterized fixture if it has one, and keep
flag-independent tests parameterized anyway.

**Why.** A test riding the default silently changes what it covers the day the
default flips, and passes either way. A helper that absorbs the new behaviour
models an ordering production never performs, so it introduces a bug rather than
hiding one.

## Examples

Bad:

```cpp
class MyCachingTest : public MyTestBase {
  // Removed when kMyCache became enabled by default:
  // MyCachingTest() { feature_list_.InitAndEnableFeature(kMyCache); }
};
```

Good:

```cpp
class MyCachingTest : public testing::TestWithParam<bool> {
 public:
  MyCachingTest() {
    feature_list_.InitWithFeatureState(features::kMyCache, GetParam());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

INSTANTIATE_TEST_SUITE_P(All, MyCachingTest, testing::Bool());
```
