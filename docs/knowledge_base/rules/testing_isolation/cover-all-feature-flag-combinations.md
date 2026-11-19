---
name: brave/cover-all-feature-flag-combinations
description:
  A class whose behaviour depends on several base::Feature flags is tested under
  all 2^N combinations, as a bitmask-parameterized suite.
rubric: testing
severity: should_fix
---

**Rule.** When the class under test is affected by several `base::Feature`
flags, parameterize its tests over every combination of them: list the features
in an unnamed-namespace `kTestFeatures`, instantiate the suite over
`testing::Range<size_t>(0, 1 << kTestFeatures.size())`, and in the fixture
enable feature `i` when bit `i` of `GetParam()` is set and disable it otherwise.
Flag tests that cover only some of the combinations, such as each flag on its
own.

**Why.** Flags interact: a combination nobody tested is where a regression
hides, and testing each flag alone never exercises those interactions.

## Examples

Bad:

```cpp
// Only kFlagA alone and kFlagB alone are tested.
class MyFeatureFlagATest : public testing::Test {
  base::test::ScopedFeatureList scoped_feature_list_{features::kFlagA};
};
class MyFeatureFlagBTest : public testing::Test {
  base::test::ScopedFeatureList scoped_feature_list_{features::kFlagB};
};
```

Good:

```cpp
class MyFeatureTest : public testing::TestWithParam<size_t> {
 protected:
  void SetUp() override {
    std::vector<base::test::FeatureRef> enabled, disabled;
    for (size_t i = 0; i < kTestFeatures.size(); i++) {
      (GetParam() & (size_t{1} << i) ? enabled : disabled)
          .push_back(kTestFeatures[i]);
    }
    scoped_feature_list_.InitWithFeatures(enabled, disabled);
  }
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(All,
                         MyFeatureTest,
                         testing::Range<size_t>(0, 1 << kTestFeatures.size()));
```
